7#!/usr/bin/env python

import sys
import pylab
import os
import time

##################################################
# CONFIG
##################################################

# which tasks to show (return true for desired tasks)
def myfilter(x):
    return True
#    return x[0] == "evalWithin"

# which color to use for each task
def getColor(text):
    if text.startswith("evalBetween"): return '#ff8080'
    if text.startswith("evalWithin"):  return '#80ff80'
    if text.startswith("step"):        return '#8080ff'
    if text.startswith("p2p"):         return '#8080ff'
    if text.startswith("m2ps"):        return '#80ff80'
    if text.find('rbf') >=0:          return '#ff8080'
    if text.find('dist')>=0:          return '#80ff80'

    if text.find('DIAG')>=0:          return '#ffC0C0'
    if text.find('PNLU')>=0:          return '#C0C0ff'
    if text.find('MMUL')>=0:          return '#80ff80'
    if text.find('XADD')>=0:          return '#80ff80'
    if text.find('MADD')>=0:          return '#80ff80'
    if text.find('TGEN')>=0:          return '#800080'
    return '#808080'

# if any text is to be attached to the task
def getText(text):
#    return []
    f=text.find('dist')
    if f>=0:
        return [text[f+4:]]
    if text.find('rbf')>=0:
        return [text[0:4],text[7:]]
    if text.startswith("evalBetween"):
        w = text.split(" ")
        return [w[1], w[2]]
    if text.startswith("evalWithin"):
        w = text.split(" ")
        return [w[1]]
    if text.startswith("step"):
        w = text.split(" ")
        return [w[1]]
    if text.startswith("p2p"):
        w = text.split(" ")
        return [w[1]]
    if text.startswith("m2ps"):
        w = text.split(" ")
        return [w[1]]
    return []

#    if text == "evalBetween": return []
#    if text == "evalWithin":  return []
#    if text == "step":        return []
#    return text.split(" ")

##################################################

def load_file(filename):
    fh = open(filename, "r")
    name = fh.readline()
    out = list()
    while len(name) != 0:
        threadid = int(fh.readline())
        start = int(fh.readline())
        length = int(fh.readline())
        out.append([name.strip(), threadid, start, length])
        name = fh.readline()
    return out

def load_data_ready_file(filename,i):
    fh = open(filename, "r")
    line = fh.readline()
    out = list()
    did = -1
    while len(line) != 0:
        line = line.strip(' ')
        alist = line.split(' ')
        t = float(alist[1])
        did  = int(alist[3])
        nodeid = int(alist[4])
        out.append([t,did,i])
        line = fh.readline()
    return out



def drawsched(fname,i,n):
    global numThreads

    tasks = load_file(fname)


    # filter out uninteresting tasks

    tasks = filter(myfilter, tasks)


    # normalize start time
    tmintime = min ([x[2] for x in tasks ]) 
    tasks = map(lambda x: [x[0], x[1], x[2]-tmintime, x[3]], tasks)

    # scale time to ms

    timeScale = 1.0/1000.0; # ns -> ms
    mt = min ([x[2] for x in tasks ]) 
#    print "Tasks",i,tmintime,tmintime,mt-tmintime
    if mt>=1000:timeScale /= 1000.0
    tasks = map(lambda x: [x[0], x[1], x[2]*timeScale, x[3]*timeScale], tasks)


    # true threadids -> logical threadids

    threadids = dict(zip([x[1] for x in tasks], [x[1] for x in tasks]))
    numThreads = len(threadids)
    threadidmap = dict(zip(threadids.keys(), range(0, len(threadids))))
    tasks = map(lambda x: [x[0], threadidmap[x[1]], x[2], x[3]], tasks)

    # plot

    ax=pylab.subplot( (n/2+1)*100+21+i)
#    ax.xaxis.grid(color='#ffa0a0',linestyle=':',linewidth=1)
    def box(x0, x1, y0, y1):
        pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fill=False)

    def drawBox(x0,x1,y0,y1,col):
        pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fc=col)

    def drawText(x,y,text):
        pylab.text(x,y,text,horizontalalignment='center',verticalalignment='center',fontsize=8)

    def drawTask(x0,x1,y0,y1,text):
        global maxtime
        if  x1 > maxtime: maxtime = x1
        if text.find("sync")>=0:
            y0=0;y1=height*(numThreads)
            drawBox(x0,x0,y0,y1,'#ff7070')
        else:
            drawBox(x0,x1,y0,y1,getColor(text))
        text = getText(text)
        if 0 : 
            if len(text) == 2:
                drawText((x0+x1)/2,(y0+y1)/2 + barHeight/4,text[0])
                drawText((x0+x1)/2,(y0+y1)/2 - barHeight/4,text[1])
            elif len(text) == 1:
                drawText((x0+x1)/2,(y0+y1)/2,text[0])

        
    for task in tasks:
        name = task[0]
        threadid = task[1]
        start = task[2]
        length = task[3]
        x0 = start;
        x1 = x0 + length;
        y0 = threadid * height
        y1 = y0 + barHeight;
        drawTask(x0, x1, y0, y1, name)

    #pylab.title(filename);
    pylab.yticks([x + barHeight/2 for x in range(0, numThreads)], range(0, numThreads));
    pylab.xticks(fontsize='large')
    pylab.yticks(fontsize='large')
    xlbl='Time (ms)'
    if mt>=1000.0:xlbl='Time (s)'
    if i == (n-1) or i== (n-2):
        pylab.xlabel(xlbl,fontsize='large')
    pylab.ylabel('Node '+str(i),fontsize='large')

#    fh= open('output/Drawing12/tasks.txt','a')
#    t = [ts[3] for ts in tasks if ts[0].find('rbf')>0]
#    av1=sum(t)/len(t)    
#    t = [ts[3] for ts in tasks if ts[0].find('dist')>0]
#    av2=sum(t)/len(t)    
#    fh.write(str(numThreads)+' rbf  ' +'%2.2f' % av1+' dist ' +'%2.2f' % av2+'\n')
#    fh.close()
#+++++++++++++++++++++++++++++++++++++
def drawDataReady(n,node_cnt):
    return
    fname="devents"+str(n)+".txt"
    drdylist=load_data_ready_file(fname,n)
    if len(drdylist) != 0:
        dmintime=min([x[0] for x in drdylist])
#        print "data",n,dmintime
        scl=1.0
        if dmintime>1000: scl=1000.0
        drdylist = map(lambda x: [(x[0]-dmintime )/1000.0/scl,x[1],x[2]],drdylist)            
        for d in drdylist:
            x0=d[0]
            y0=0;y1=height*(numThreads)
 #           print d,(node_cnt/2+1)*100 +21+d[2]
            pylab.subplot( (node_cnt/2+1)*100 +21+d[2])
            #pylab.plot([x0,x0],[y0,y1],color='b')

height = 1.0
barHeight = height * 0.8
maxtime=0
filename = "schedule"

if len(sys.argv) > 1:
    node_cnt = int(sys.argv[1])
    numThreads = int(sys.argv[2])
    outfname=sys.argv[3]
    infolder=sys.argv[4]
    filename=infolder+'/'+filename

pylab.figure(1,figsize=(22,12));



for i in range(0,node_cnt):
    fname = filename + str(i)+".dat"
    #drawDataReady(i,node_cnt)
    drawsched(fname,i,node_cnt)

for i in range(0,node_cnt):
    x0=maxtime
    y0=0;y1=height*(numThreads)
    pylab.subplot( (node_cnt/2+1)*100 +21+i)
    pylab.plot([x0,x0],[y0,y1],color='w')

pylab.savefig(outfname.strip('.txt')+'.png')
#pylab.show()
