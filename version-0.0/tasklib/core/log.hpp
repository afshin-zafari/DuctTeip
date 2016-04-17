#ifndef __LOG_HPP__
#define __LOG_HPP__

namespace LogTag {
    typedef struct {} Ignore;
    typedef struct {} Barrier;
    typedef struct {} TaskQueue;
    typedef struct {} LockListeners;
    typedef struct {} VersionListeners;
    typedef struct {} Stealing;
}

#include "platform/spinlock.hpp"
#include "platform/threads.hpp"
#include "platform/gettime.hpp"
#include "platform/perfcount.hpp"
#include "core/accessutil.hpp"
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <string>
#include <sstream>
#include <signal.h>

template<typename Options> class Log;
template<typename Options> class TaskBase;
template<typename Options> class Handle;
template<typename Options> class TaskExecutor;
template<typename Options> class ThreadManager;
template<typename Options> class TaskQueue;
template<typename Options> class WorkerThread;

namespace detail {

// ===========================================================================
// GetName
// ===========================================================================
template<typename O, typename T = void>
struct GetName {
    static std::string getName(TaskBase<O> *task) {
        char name[30];
        sprintf(name, "%p", task);
        return name;
    }
};
template<typename O>
struct GetName<O, typename O::TaskName> {
    static std::string getName(TaskBase<O> *task) {
        return task->getName();
    }
};

template<typename Options>
std::ostream& operator<<(std::ostream& os, TaskBase<Options> *task) {
    os << GetName<Options>::getName(task);
    return os;
}

template<typename O, typename T = void>
struct GetHandleName { static void *getName(Handle<O> *handle) { return handle; } };
template<typename O>
struct GetHandleName<O, typename O::HandleName> { static std::string getName(Handle<O> *handle) { return handle->getName(); } };

template<typename Options>
std::ostream& operator<<(std::ostream& os, Handle<Options> *handle) {
    os << GetHandleName<Options>::getName(handle);
    return os;
}


// ===========================================================================
// Option Logging_DAG
// ===========================================================================
template<typename Options, typename T = void>
class Log_DAG {
public:
    static void dump_dag(const char *) {}
    static void dump_data_dag(const char *) {}
    static void addDependency(TaskBase<Options> *, Access<Options> *, int) {}
};

template<typename Options>
class Log_DAG<Options, typename Options::Logging_DAG> {
private:

    typedef typename Options::TaskId requires_TaskId;
    typedef typename Options::CurrentTask requires_CurrentTask;

    typedef std::pair<typename Options::version_t, Handle<Options> *> datapair;
    typedef size_t node_t;

    struct Node {
        std::string name;
        std::string style;
        int type;
        time_t time_stamp;
        Node(const std::string &name_,
             const std::string &style_,
             size_t type_)
        :   name(name_), style(style_), type(type_) {
            time_stamp = Time::getTime();
        }
    };

    struct Edge {
        node_t source;
        node_t sink;
        std::string style;
        time_t time_stamp;
        size_t type;
        Edge(node_t source_, node_t sink_, const char *style_, size_t type_)
        : source(source_), sink(sink_), style(style_), type(type_) {
            time_stamp = Time::getTime();
        }
    };

    struct LogDagData {
        std::map<Handle<Options> *, std::vector<typename Options::version_t> > handles;
        std::map<datapair, std::vector<node_t> > dep;

        SpinLock spinlock;
        std::vector<Node> nodes;
        std::vector<Edge> edges;
        std::map<TaskBase<Options> *, node_t> tasknodes;
        std::map<datapair, node_t> datanodes;

        void clear() {
            handles.clear();
            dep.clear();
            nodes.clear();
            edges.clear();
            tasknodes.clear();
            datanodes.clear();
        }
    };

    static LogDagData &getDagData() {
        // this is very convenient and does not require a variable definition outside of the class,
        // but the C++ standard does not require initialization to be thread safe until C++2011:
        // 6.7 [stmt.dcl]
        //   "If control enters the declaration concurrently while the variable is being initialized,
        //    the concurrent execution shall wait for completion of the initialization."
        // Also, this is apparently often implemented with a flag that will be checked each
        // time this method if entered, adding some overhead.
        static LogDagData data;
        return data;
    }

    static node_t addNode(LogDagData &data, std::string name, std::string style, size_t type) {
        data.nodes.push_back(Node(name, style, type));
        return data.nodes.size()-1;
    }

    static node_t getTaskNode(LogDagData &data, TaskBase<Options> *task) {
        if (data.tasknodes.find(task) != data.tasknodes.end())
            return data.tasknodes[task];

        std::stringstream ss;
        ss << task;
        node_t newNode = addNode(data, ss.str(), "", 0);
        data.tasknodes[task] = newNode;
        return newNode;
    }

    static size_t getDataNode(LogDagData &data, node_t taskNode, Access<Options> *access) {
        datapair p(access->getRequiredVersion(), access->getHandle());

        // add task dependency on this access
        data.dep[p].push_back(taskNode);

        // return existing access node, if such exist
        if (data.datanodes.find(p) != data.datanodes.end())
            return data.datanodes[p];

        Handle<Options> *handle = access->getHandle();

        const size_t num = data.handles[handle].size();
        // store version in the handle's version queue
        data.handles[handle].push_back(access->getRequiredVersion());

        // create the access node
        std::stringstream ss;
        ss << handle << "(" << p.first << ")";
        node_t newNode = addNode(data, ss.str(), "", 1);
        // update map from access to node
        data.datanodes[p] = newNode;

        if (num > 0) {
            // dependencies on old version of this data exists
            // add dependencies from the task to all tasks that
            // needed the old version

            datapair prevPair(data.handles[handle][num-1], handle);

            std::vector<node_t> &depNodes(data.dep[prevPair]);

            for (size_t i = 0; i < depNodes.size(); ++i) {
                data.edges.push_back(Edge(depNodes[i], newNode, "", 0));
            }
        }
        return newNode;
    }

public:

    static void addDependency(TaskBase<Options> *task, Access<Options> *access, int type) {
        LogDagData &data(getDagData());
        SpinLockScoped lock(data.spinlock);

        node_t taskNode = getTaskNode(data, task);
        node_t dataNode = getDataNode(data, taskNode, access);
        typedef typename Options::AccessInfoType AccessInfo;
        if (AccessUtil<Options>::readsData(type))
            data.edges.push_back(Edge(dataNode, taskNode, "", type));// ### TODO: Assumes type 0 == read!
    }

    static void remove_node_type(int id) {
        //
        // still leaves unnecessary edges. see allpairs example.
        // when data nodes are removed, some edges are already implied.

        LogDagData &data(getDagData());
        SpinLockScoped lock(data.spinlock);

        // create new node list without data nodes
        // create map from old to new task node number
        std::vector<Node> newNodes;
        std::map<node_t, node_t> nodeMap;
        for (size_t i = 0; i < data.nodes.size(); ++i) {
            const Node &n(data.nodes[i]);
            if (n.type == id)
                continue;
            nodeMap[i] = newNodes.size();
            newNodes.push_back(n);
        }

        // create map from edges, store edge type
        std::map<node_t, std::vector<node_t> > dag;
        for (size_t i = 0; i < data.edges.size(); ++i) {
            dag[data.edges[i].source].push_back(data.edges[i].sink);
        }

        std::set<std::pair<node_t, node_t> > added;
        std::vector<Edge> newEdges;

        for (size_t i = 0; i < data.edges.size(); ++i) {
            const Edge &e(data.edges[i]);
            const Node &source(data.nodes[e.source]);
            const Node &sink(data.nodes[e.sink]);

            // skip edges from data nodes
            if (source.type == id)
                continue;

            node_t newSource = nodeMap[e.source];

            // handle edges to data nodes
            if (sink.type == id) {
                // follow all successors of the data node
                std::vector<node_t> &sinks(dag[e.sink]);
                for (size_t j = 0; j < sinks.size(); ++j) {
                    if (added.find(std::make_pair(newSource, nodeMap[ sinks[j] ])) == added.end()) {
                        newEdges.push_back(Edge(newSource, nodeMap[ sinks[j] ], "", 0));
                        added.insert(std::make_pair(newSource, nodeMap[ sinks[j] ]));
                    }
                }
            }
            else {
                // include task - task edge
                if (added.find(std::make_pair(newSource, nodeMap[ e.sink ])) == added.end()) {
                    newEdges.push_back(Edge(newSource, nodeMap[e.sink], e.style.c_str(), e.type));
                    added.insert(std::make_pair(newSource, nodeMap[ e.sink ]));
                }
            }
        }

        // remove unnecessary edges that are implied now that data nodes are removed
        std::vector<Edge> newEdges2;
        {
            // this can be done much faster.
            // use shortest path bredth-first search and assign distance from root
            // to all nodes. if a node gets two distances, find and remove the shortest path.
            std::map<node_t, std::vector<node_t> > map;
            for (size_t i = 0; i < newEdges.size(); ++i) {
                const Edge &e(newEdges[i]);
                map[e.source].push_back(e.sink);
                std::cout << e.source << "-" << e.sink << std::endl;
            }
            for (size_t i = 0; i < newEdges.size(); ++i) {
                const Edge &e(newEdges[i]);
                std::stack<node_t> workqueue;
                // follow all edges except for the currently evaluated direct edge
                for (size_t i = 0; i < map[e.source].size(); ++i) {
                    if (map[e.source][i] != e.sink)
                        workqueue.push(map[e.source][i]);
                }

                bool goodedge = true;
                while (!workqueue.empty()) {
                    node_t curr = workqueue.top();
                    if (curr == e.sink) {
                        goodedge = false;
                        break;
                    }
                    workqueue.pop();
                    for (size_t i = 0; i < map[curr].size(); ++i)
                        workqueue.push(map[curr][i]);
                }
                if (goodedge)
                    newEdges2.push_back(e);
            }
        }

        std::swap(data.nodes, newNodes);
        std::swap(data.edges, newEdges2);
    }

    static void add_mutex_edges() {
        // add mutex edges. Data nodes must be present
        // BROKEN
        LogDagData &data(getDagData());
        SpinLockScoped lock(data.spinlock);

        // create map from edges, store edge type
        std::map<node_t, std::vector<node_t> > dag;
        for (size_t i = 0; i < data.edges.size(); ++i) {
            if (data.edges[i].type != 0) {
                bool dupe = false;
                for (size_t j = 0; j < dag[data.edges[i].source].size(); ++j)
                    if (dag[data.edges[i].source][j] == data.edges[i].sink) {
                        dupe = true;
                        break;
                    }
                if (!dupe)
                    dag[data.edges[i].source].push_back(data.edges[i].sink);
            }
        }

        std::vector<Edge> extraEdges;
        std::set<node_t> handled;
        for (size_t i = 0; i < data.nodes.size(); ++i) {
            if (handled.find(data.edges[i].source) != handled.end())
                break;
            handled.insert(data.edges[i].source);

            std::set<node_t> clique;

            clique.insert(data.edges[i].source);
            std::stack<node_t> workqueue;
            workqueue.push(data.edges[i].source);
            while (!workqueue.empty()) {
                node_t curr = workqueue.top();
                workqueue.pop();

                std::vector<size_t> &nodes(dag[curr]);
                for (size_t j = 0; j < nodes.size(); ++j) {
                    clique.insert(dag[curr][j]);
                    workqueue.push(dag[curr][j]);
                }
            }

            std::vector<node_t> cvec(clique.begin(), clique.end());
            for (size_t i = 0; i < cvec.size(); ++i)
                for (size_t j = i+1; j < cvec.size(); ++j)
                    extraEdges.push_back(Edge(cvec[i], cvec[j], "[dir=none,penwidth=2]", 0));
        }
        data.edges.insert(data.edges.end(), extraEdges.begin(), extraEdges.end());
    }


    static void dump_dag(const char *filename) {
        std::ofstream out(filename);
        LogDagData &data(getDagData());
        SpinLockScoped lock(data.spinlock);

        out << "digraph {" << std::endl;
        out << "  overlap=false;" << std::endl;

        bool dupename = false;
        {
            std::set<std::string> names;
            for (size_t i=0; i <data.nodes.size(); ++i)
                names.insert(data.nodes[i].name);
            if (names.size() != data.nodes.size())
                dupename = true;
        }
        for (size_t i = 0; i < data.nodes.size(); ++i) {
            const Node &n(data.nodes[i]);
            if (n.name.empty())
                continue;
            if (n.type == 1)
                continue;
            if (dupename)
                out << "  \"" << n.name << "_" << i << "\" [label=\""<<n.name<<"\"]" << " " << n.style << ";" << std::endl;
            else
                out << "  \"" << n.name << "\" [label=\""<<n.name<<"\"]" << " " << n.style << ";" << std::endl;
        }

        out << std::endl;

        for (size_t i = 0; i < data.edges.size(); ++i) {
            const Edge &e(data.edges[i]);
            const Node &source(data.nodes[e.source]);
            const Node &sink(data.nodes[e.sink]);
            if (dupename) {
                out << "  \""
                    << source.name << "_" << e.source
                    << "\" -> \""
                    << sink.name << "_" << e.sink
                    << "\" " << e.style << ";" << std::endl;
            }
            else {
                out << "  \""
                    << source.name
                    << "\" -> \""
                    << sink.name
                    << "\" " << e.style << ";" << std::endl;
            }
        }

        out << "}" << std::endl;
        out.close();
        data.clear();
    }

    static void invert_edges() {
        // assumes only mutex edges (these must be added first)
        LogDagData &data(getDagData());
        SpinLockScoped lock(data.spinlock);
        std::set<std::pair<node_t, node_t> > edges;
        for (size_t i = 0; i < data.edges.size(); ++i) {
            if (data.edges[i].type == 1)
                continue;
            edges.insert(std::make_pair(data.edges[i].source, data.edges[i].sink));
            edges.insert(std::make_pair(data.edges[i].sink, data.edges[i].source));
        }

        std::vector<Edge> newEdges;
        for (size_t i = 0; i < data.nodes.size(); ++i)
            for (size_t j = i+1; j < data.nodes.size(); ++j)
                if (edges.find(std::make_pair(i, j)) == edges.end())
                    newEdges.push_back(Edge(i, j, "[dir=none,penwidth=2]", 0));
        std::swap(newEdges, data.edges);
    }
};


// ===========================================================================
// Option Logging_Perf
// ===========================================================================
template<typename Options, typename T = void>
struct Log_Perf {
    struct Data {};
    template<typename T2> static void initPerf(T2 &) {}
    template<typename T2> static void startPerf(T2 &) {}
    template<typename T2> static void stopPerf(T2 &) {}
    template<typename T2> static void appendName(T2 &, std::string &) {}
};

template<typename Options>
struct Log_Perf<Options, typename Options::Logging_Perf> {
    struct Data {
        PerformanceCounter perf;
        unsigned long long counter;
    };

    static PerformanceCounter &getPerf() {
        static PerformanceCounter perf;
        return perf;
    }

    static void initPerf() {
        getPerf().init(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    }
    static void startPerf() { getPerf().start(); }
    static void stopPerf() { getPerf().stop(); }
    unsigned long long readPerf() { return getPerf().readCounter(); }


    template<typename T>
    static void initPerf(T &logdata) {
        logdata.perf.init(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    }
    template<typename T>
    static void startPerf(T &logdata) {
        logdata.perf.counter = logdata.perf.readCounter();
        logdata.perf.start();
    }
    template<typename T>
    static void stopPerf(T &logdata) {
        logdata.perf.stop();
    }
    template<typename T>
    static void appendName(T &logdata, std::string &name) {
        unsigned long long perfcount = logdata.perf.readCounter() - logdata.perf.counter;
        std::stringstream ss;
        ss << name << " (perfcount=" << perfcount << ")";
        name = ss.str();
    }
};

// ===========================================================================
// Option Logging_Timing
// ===========================================================================
template<typename Options, typename T = void>
class Log_Timing {
public:
    struct Instrumentation { Instrumentation(const char *) {} };
    static void dump_times() {}
};

template<typename Options>
class Log_Timing<Options, typename Options::Logging_Timing> {
private:
    typedef std::map<std::string, std::pair<double, Time::TimeUnit> > maptype;
    static SpinLock &getSpinlock() {
        static SpinLock spinlock;
        return spinlock;
    }

    static maptype &getMap() {
        static maptype timemap;
        return timemap;
    }
    static void addTime(const char *name, Time::TimeUnit time, unsigned long long m) {
        maptype &timemap(getMap());
        SpinLockScoped lock(getSpinlock());
        if (timemap.find(name) == timemap.end())
            timemap[name] = std::make_pair(time/1000.0, m);
        else {
            getMap()[name].first += time/1000.0;
            getMap()[name].second += m;
        }
    }

public:
    struct Instrumentation {
        const char *name_;
        Time::TimeUnit start;
        unsigned long long startM;

        Instrumentation(const char *name)
         : name_(name), start(Time::getTime()), startM(0/*getPerf().readCounter()*/) {}
        ~Instrumentation() {
            const Time::TimeUnit stop(Time::getTime());
            const unsigned long long stopM(0/*getPerf().readCounter()*/);
            addTime(name_, stop - start, stopM - startM);
        }
    };

    static void dump_times() {
        typedef maptype::iterator itr;
        SpinLockScoped lock(getSpinlock());
        maptype &timemap(getMap());
        for (itr i = timemap.begin(); i != timemap.end(); ++i)
            std::cout << i->first << ": " << i->second.first << " " << i->second.second << std::endl;
        timemap.clear();
    }
};

// ===========================================================================
// Option Logging_DumpState
// ===========================================================================
template<typename Options, typename T = void>
struct Log_DumpState {
    static void dumpState(ThreadManager<Options> &) {}
    static void installSignalHandler(ThreadManager<Options> &) {}
};

template<typename Options>
struct Log_DumpState<Options, typename Options::Logging> {
    static ThreadManager<Options> **getTm() {
        static ThreadManager<Options> *tm;
        return &tm;
    }

    static void signal_handler(int param) {
        Log<Options>::dumpState(**getTm());
        exit(1);
    }

    static void installSignalHandler(ThreadManager<Options> &tm) {
        *(getTm()) = &tm;
        signal(SIGINT, signal_handler);
    }

    static void dumpState(ThreadManager<Options> &tm) {

        typename Log<Options>::LogData &data(Log<Options>::getLogData());
        const size_t num = data.threadmap.size();

        const size_t numWorkers = tm.getNumThreads();
        std::cerr << "Num threads = " << numWorkers << std::endl;
        const size_t numQueues = tm.getNumQueues();
        std::cerr << "Num queues = " << numQueues << std::endl;

        const size_t N = 5;
        // copy last N events
        std::vector< std::vector< typename Log<Options>::Event> > evs(num);
        for (size_t i = 0; i < num; ++i) {
            SpinLockScoped lock(data.threaddata[i].lock);
            if (data.threaddata[i].events.size() < N)
                evs[i] = data.threaddata[i].events;
            else
                evs[i].insert(evs[i].begin(), data.threaddata[i].events.end()-N, data.threaddata[i].events.end());
        }

        // normalize times
        // keep a pointer per thread.
        // In each iteration:
        //   find minimum time among the events currently pointed to
        //   replace all events sharing this minimum time with a logical time, and increase pointers for these
        std::vector<size_t> ptr(num, 0);
        size_t time = 0;
        for (;;) {
            time_t minTime = 0;
            bool firstTime = true;

            // find minimum time among events currently pointed to
            for (size_t i = 0; i < num; ++i) {
                if (ptr[i] < evs[i].size()) {
                    if (firstTime) {
                        firstTime = false;
                        minTime = evs[i][ptr[i]].time_start;
                    }
                    else
                        minTime = std::min(minTime, evs[i][ptr[i]].time_start);
                }
            }

            // if no time at all was found, we are done
            if (firstTime)
                break;

            // replace all events sharing this minimum time with a logical time, and increase pointers for these
            for (size_t i = 0; i < num; ++i) {
                if (ptr[i] < evs[i].size() && evs[i][ptr[i]].time_start == minTime)
                    evs[i][ptr[i]++].time_start = time;
            }

            // next logical time
            ++time;
        }

        std::cerr<<"Last events:" << std::endl;
        for (size_t i = 0; i < num; ++i) {
            std::cerr<<"Thread " << i << ": ";
            for (size_t j = 0; j < evs[i].size(); ++j) {
                std::cerr << evs[i][j].time_start << "[" << evs[i][j].name << "] ";
            }
            std::cerr << std::endl;
            if (i == 0) {
                TaskQueue<Options> &queue(tm.barrierProtocol.getTaskQueue());

                std::cerr<< "Main queued=: ";
                TaskBase<Options> *list = queue.root;
                while (list->next != 0) {
                    std::cerr << list << " ";
                    list = list->next;
                }


//                std::cerr<< "Main queued=" << queue.size() << ": ";
//                for (size_t j = 0; j < queue.size(); ++j) {
//                    std::cerr << queue.buffer[j] << " ";
//                }
                std::cerr<<std::endl;
            }
            else {
                const size_t threadid = i-1;
                WorkerThread<Options> *worker(tm.getWorker(threadid));
                assert(data.threadmap[worker->getThread()->getThreadId()] == (int) i);
                TaskQueue<Options> &queue(worker->getTaskQueue());


                std::cerr<< "Worker " << i << ": ";
                TaskBase<Options> *list = queue.root;
                while (list->next != 0) {
                    std::cerr << list << " ";
                    list = list->next;
                }

//                std::cerr<<"Worker " << i << " queued="<<queue.size()<<": ";
//                for (size_t j = 0; j < queue.size(); ++j)
//                    std::cerr << queue.buffer[j] << " ";
                std::cerr<<std::endl;
            }
        }
    }
};

// ===========================================================================
// Option TaskExecution
// ===========================================================================
template<typename Options, typename T = void>
struct Log_TaskExecution {
    struct RunTaskData {};
    static void runTaskBefore(TaskBase<Options> *) {}
    static void runTaskAfter() {}
};

template<typename Options>
struct Log_TaskExecution<Options, typename Options::Logging> {
    struct RunTaskData : public Log_Perf<Options>::Data {
        Time::TimeUnit start;
        std::string taskname;
    };

    static void runTaskBefore(TaskBase<Options> *task) {
        typename Log<Options>::ThreadData &data(Log<Options>::getThreadData());
        data.taskname = detail::GetName<Options>::getName(task);
        Log_Perf<Options>::initPerf(data);
        Log_Perf<Options>::startPerf(data);
        data.start = Time::getTime();
    }

    static void runTaskAfter() {
        Time::TimeUnit stop = Time::getTime();
        typename Log<Options>::ThreadData &data(Log<Options>::getThreadData());
        Log_Perf<Options>::stopPerf(data);
        Log_Perf<Options>::appendName(data, data.taskname);
        Log<Options>::add(typename Log<Options>::Event(data.taskname, data.start, stop));
    }
};

// ===========================================================================
// Helpers for logging tags
// ===========================================================================

// Class with logging enabled, inherited by enabling specializations
template<typename Options>
struct addByTagEnabled {
    static void add(const char *text) { Log<Options>::add(text); }
    template <typename T> static void add(const char *text, T param) { Log<Options>::add(text, param); }
    static void push() { Log<Options>::push(); }
    static void pop(const char *text) { Log<Options>::pop(text); }
    template <typename ParamT>
    static void pop(const char *text, ParamT param) { Log<Options>::pop(text, param); }
};

// Base class with logging disabled
template<typename Options, typename Tag, typename T = void>
struct addByTag {
    static void add(const char *) {}
    template <typename ParamT> static void add(const char *, ParamT) {}
    static void push() {}
    static void pop(const char *) {}
    template <typename ParamT> static void pop(const char *, ParamT) {}
};

// Specialization of addByTag for Logging_Barrier enabling LogTag_Barrier
template<typename Options>
struct addByTag<Options, typename LogTag::Barrier, typename Options::Logging_Barrier>
: public addByTagEnabled<Options> {};

// ===========================================================================
// Option Logging
// ===========================================================================
template<typename Options, typename T = void>
class Log_Impl {
public:
    static void add(const char *) {}
    static void addEvent(const char *, Time::TimeUnit start, Time::TimeUnit stop) {}
    template<typename ParamT>
    static void add(const char *, ParamT) {}

    template<typename Tag>
    static void add(Tag, const char *) {}
    template<typename Tag, typename ParamT>
    static void add(Tag, const char *, ParamT) {}

    static void push() {}
    static void pop(const char *) {}
    template<typename ParamT>
    static void pop(const char *, ParamT) {}

    template<typename Tag>
    static void push(Tag) {}
    template<typename Tag>
    static void pop(Tag, const char *) {}
    template<typename Tag, typename ParamT>
    static void pop(Tag, const char *, ParamT) {}

    static void dump(const char *, int = 0) {}
    static void stat(Time::TimeUnit &a, Time::TimeUnit &b) { a = b = 0; }

    static void clear() {}

    static void registerThread(int id) {}
    static void init(size_t) {}
};

template<typename Options>
class Log_Impl<Options, typename Options::Logging> {
public:
    typedef Time::TimeUnit time_t;
    struct Event {
        std::string name;
        time_t time_start;
        time_t time_total;
        Event() {}
        Event(const std::string &name_, time_t start, time_t stop)
        : name(name_), time_start(start), time_total(stop-start) 
        {
	  //printf("Event Constructor,%s,%s\n",name_,name.c_str());
	}
        int operator<(const Event &rhs) const {
            return time_start < rhs.time_start;
        }
    };

    struct ThreadData
    : public Log_TaskExecution<Options>::RunTaskData {
        std::vector<Event> events;
        std::stack<Time::TimeUnit> timestack;
        ThreadData() {
            events.reserve(4096);
        }
    };
    struct LogData {
        SpinLock initspinlock;
        std::map<ThreadIDType, int> threadmap;
        ThreadData *threaddata;
        LogData() : threaddata(0) {}
    };
public:

    static LogData &getLogData() {
        // this is very convenient and does not require a variable definition outside of the class,
        // but the C++ standard does not require initialization to be thread safe until C++11:
        // 6.7 [stmt.dcl]
        //   "If control enters the declaration concurrently while the variable is being initialized,
        //    the concurrent execution shall wait for completion of the initialization."
        // Also, this is apparently often implemented with a flag that will be checked each
        // time this method if entered, adding some overhead.
        static LogData data;
        return data;
    }

    static ThreadData &getThreadData() {
        LogData &data(getLogData());
        const int id = data.threadmap[ThreadUtil::getCurrentThreadId()];
        return data.threaddata[id];
    }

    static void add(const Event &event) {
        ThreadData &data(getThreadData());
        data.events.push_back(event);
    }

    static void addEvent(const char *text, Time::TimeUnit start, Time::TimeUnit stop) {
      //printf("add Event Text:>%s<\n",text);
        add(Event(text, start, stop));
    }

    static void add(const char *text) {
        Time::TimeUnit time = Time::getTime();
        add(Event(text, time, time));
    }

    template<typename ParamT>
    static void add(const char *text, ParamT param) {
        Time::TimeUnit time = Time::getTime();
        std::stringstream ss;
        ss << text << "(" << param << ")";
        add(Event(ss.str(), time, time));
    }

    template<typename Tag>
    static void add(Tag, const char *text) {
        addByTag<Options, Tag>::add(text);
    }

    template<typename Tag, typename ParamT>
    static void add(Tag, const char *text, ParamT param) {
        addByTag<Options, Tag>::add(text, param);
    }

    static void push() {
        Time::TimeUnit startTime = Time::getTime();
        typename Log<Options>::ThreadData &data(Log<Options>::getThreadData());
        data.timestack.push(startTime);
    }

    static void pop(const char *text) {
        Time::TimeUnit endTime = Time::getTime();
        typename Log<Options>::ThreadData &data(Log<Options>::getThreadData());
        Time::TimeUnit startTime = data.timestack.top();
        data.timestack.pop();
        Log<Options>::add(typename Log<Options>::Event(text, startTime, endTime));
    }

    template <typename ParamT>
    static void pop(const char *text, ParamT param) {
        Time::TimeUnit endTime = Time::getTime();
        typename Log<Options>::ThreadData &data(Log<Options>::getThreadData());
        Time::TimeUnit startTime = data.timestack.top();
        data.timestack.pop();
        std::stringstream ss;
        ss << text << "(" << param << ")";
        Log<Options>::add(typename Log<Options>::Event(ss.str(), startTime, endTime));
    }

    template<typename Tag>
    static void push(Tag) {
        addByTag<Options, Tag>::push();
    }

    template<typename Tag>
    static void pop(Tag, const char *text) {
        addByTag<Options, Tag>::pop(text);
    }

    template<typename Tag, typename ParamT>
    static void pop(Tag, const char *text, ParamT param) {
        addByTag<Options, Tag>::pop(text, param);
    }

    static void dump(const char *filename, int node_id = 0) {
        std::ofstream out(filename);
        LogData &data(getLogData());

        const size_t num = data.threadmap.size();
        std::vector<std::pair<Event, size_t> > merged;

        for (size_t i = 0; i < num; ++i) {
            for (size_t j = 0; j < data.threaddata[i].events.size(); ++j)
                merged.push_back(std::make_pair(data.threaddata[i].events[j], i));
        }

        std::sort(merged.begin(), merged.end());

        for (size_t i = 0; i < merged.size(); ++i) {
            out << node_id << " "
                << merged[i].second << ": "
                << merged[i].first.time_start-merged[0].first.time_start << " "
                << merged[i].first.time_total << " "
                << merged[i].first.name << std::endl;
        }

        out.close();
    }

    static void stat(unsigned long long &end, unsigned long long &total) {

        LogData &logdata(getLogData());
        end = 0;
        total = 0;
        unsigned long long minimum = 0;
        bool gotMinimum = false;

        const size_t numt = logdata.threadmap.size();
        for (size_t j = 0; j < numt; ++j) {
            ThreadData &data(logdata.threaddata[j]);
            const size_t num = data.events.size();

            if (num == 0)
                continue;

            if (!gotMinimum) {
                minimum = data.events[0].time_start;
                gotMinimum = true;
            }
            for (size_t i = 0; i < num; ++i) {
                if (data.events[i].time_start < minimum)
                    minimum = data.events[i].time_start;
                if (data.events[i].time_start + data.events[i].time_total > end)
                    end = data.events[i].time_start + data.events[i].time_total;
                total += data.events[i].time_total;
            }
        }
        end -= minimum;
    }

    static void clear() {
        LogData &data(getLogData());
        const size_t num = data.threadmap.size();
        for (size_t i = 0; i < num; ++i)
            data.threaddata[i].events.clear();
    }

    static void registerThread(int id) {
        LogData &data(getLogData());
        SpinLockScoped lock(data.initspinlock);
        data.threadmap[ThreadUtil::getCurrentThreadId()] = id;
    }

    static void init(const size_t numCores) {
        LogData &data(getLogData());
        if (data.threaddata != 0)
            delete [] data.threaddata;
        data.threaddata = new ThreadData[numCores];
        data.threadmap.clear();
        registerThread(0);
    }
};

} // namespace detail

// ===========================================================================
// Log
// ===========================================================================
template<typename Options>
class Log
  : public detail::Log_DAG<Options>,
    public detail::Log_Impl<Options>,
    public detail::Log_DumpState<Options>,
    public detail::Log_TaskExecution<Options>,
    public detail::Log_Timing<Options>
{
public:
};

#endif // __LOG_HPP__
