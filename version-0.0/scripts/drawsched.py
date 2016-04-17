#!/usr/bin/env python

import sys

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
    return '#808080'

# if any text is to be attached to the task
def getText(text):
#    return []
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

filename = "schedule.dat"
if len(sys.argv) > 1:
    filename = sys.argv[1]

tasks = load_file(filename)


# filter out uninteresting tasks

tasks = filter(myfilter, tasks)


# normalize start time

mintime = min([x[2] for x in tasks])
tasks = map(lambda x: [x[0], x[1], x[2]-mintime, x[3]], tasks)

# scale time to ms

timeScale = 1.0/1000.0; # ns -> ms
tasks = map(lambda x: [x[0], x[1], x[2]*timeScale, x[3]*timeScale], tasks)


# true threadids -> logical threadids

threadids = dict(zip([x[1] for x in tasks], [x[1] for x in tasks]))
numThreads = len(threadids)
threadidmap = dict(zip(threadids.keys(), range(0, len(threadids))))
tasks = map(lambda x: [x[0], threadidmap[x[1]], x[2], x[3]], tasks)


# plot

import pylab
def box(x0, x1, y0, y1):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fill=False)

def drawBox(x0,x1,y0,y1,col):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fc=col)

def drawText(x,y,text):
    pylab.text(x,y,text,horizontalalignment='center',verticalalignment='center')

height = 1.0
barHeight = height * 0.8

def drawTask(x0,x1,y0,y1,text):
    drawBox(x0,x1,y0,y1,getColor(text))
    text = getText(text)
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
pylab.xlabel('Time (ms)',fontsize='large')
pylab.ylabel('Thread',fontsize='large')
pylab.show()
