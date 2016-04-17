#ifndef __DAG_H__
#define __DAG_H__

#ifdef CREATE_DAG

#include <pthread.h>

#ifdef __cplusplus
#include <sstream>
template<typename T>
std::string DAG_toString(T t) { std::stringstream ss; ss << t; return ss.str(); }
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void DAG_init();
extern void DAG_addEdge(const void *source,
												const void *sink,
												const char *style);

extern void DAG_addNode(const void *handle,
												const char *name,
												const char *style);

extern void DAG_dump(const char *filename);

#ifdef __cplusplus
}
#endif

#else
#define DAG_init()
#define DAG_addEdge(a, b, c)
#define DAG_addNode(a, b, c)
#define DAG_dump(a)
#define DAG_toString(a)
#endif

#endif // __DAG_H__
