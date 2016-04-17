#!/usr/bin/env python

import sys

def read_all_lists(filename):
    fh = open(filename, "r")
    aline = fh.readline()
    out = list()
    cnt = 0
    while len(aline) != 0 and cnt <= 10:
        tststr = aline.split()[1]
        tstamp = int(tststr[len(tststr)-7:len(tststr)])
        node   = int(aline.split()[2])
        event  = int(aline.split()[3])
        i1     = int(aline.split()[4])
#        i2     = int(aline.split()[5])
        i2 = 0 
        out.append([tstamp,node,event,i1,i2])
        aline = fh.readline()
        cnt = cnt +1 
    return out

filename = "viz.txt"
if len(sys.argv) > 1:
    filename = sys.argv[1]

tasks = read_all_lists(filename)
print tasks

numThreads = 3
w=1
h=1
yaxs=6;xaxs=6

wtot = xaxs
ncnt = 8
splot= ncnt/2*100+21
import pylab

def drawBox(node,oid,oit,sts):
    pylab.subplot(node+splot)
    box(0.01,0.02,0.01,0.02)
    box(xaxs+0.01,xaxs+0.02,yaxs+0.01,yaxs+0.02)
    box(oid*w+0.1,oid*w+w-0.1,oit*(h+0.1)+0.1,oit*(h+0.1)+h-0.1)

def drawDep(node,id1,id2,t1,t2,direction):
    pylab.subplot(node+splot)
    pylab.xticks([])
    pylab.yticks([])
    box(0.01,0.02,0.01,0.02)
    box(xaxs+0.01,xaxs+0.02,yaxs+0.01,yaxs+0.02)
#    pylab.plot([id1*w+w-0.1-w/2.0,id2*w + 0.1+w/2.0],[t2*(h+0.1)+0.1,t1*(h+0.1)-0.1+h],'-b')
#    pylab.fill([id2*w + 0.1+w/2.0, id2*w + 0.11+w/2.0, id2*w +0.1- 0.01+w/2.0], [t1*(h+0.1)-0.1+h, t1*(h+0.1)-0.1+h+0.01, t1*(h+0.1)-0.1+h-0.01])


def box(x0, x1, y0, y1):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fill=False)
def boxColor(x0, x1, y0, y1,clr):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fc=clr)
#def clearBox(node,oid,oit)

def drawText(x,y,text):
    pylab.text(x,y,text,horizontalalignment='center',verticalalignment='center')

height = 1.0
barHeight = height * 0.8

def drawTask(x0,x1,y0,y1,text):
    box(x0,x1,y0,y1)


for nd in range(0,ncnt):
    pylab.subplot(nd+splot)
    pylab.xticks([])
    pylab.yticks([])
    boxColor(.1,wtot-0.1,0.1,1.1,[1,1,0.48])
    for i in range(1,5):
        drawBox(nd,i,1,i)
        drawBox(nd,i,2,i)
        drawBox(nd,i,3,i)

pylab.show()

# clear box
# sorting the data
# animating during reading from file    

#  Task sts list = []
#  Data sts list = []
#  Lsnr sts list = []
#  Evnt list = []
# read from file
# split & convert
#  Task list = filter ( task_filter,list)
#  Data list = filter ( data_filter,list)
#  Lsnr list = filter ( lsnr_filter,list)
#  event_list = sorted ( all_list,key = lambda tstamp:tstamp[3] ) 
# plot area  = node_cnt sub-plots
# def drawBox(node,id,sts)
# def drawDep(node,id1,id2,direction)
# def drawComm(src_node,src_type,src_id,dest_node,dest_type,dest_id)
# def DrawEvent(e,node,i1,i2)
#            created     ,tasks          box,tid,sts
#            dep_checked ,tasks          box,tid,sts2
#            created     ,data           box,did,sts
#            created     ,lsnrs          box,lid,sts          arrow,did,data_id
#            sent        ,tasks  ,dest   arrow,tid,dest
#            received    ,tasks  ,src    arrow,src,tid
#            acked       ,tasks          box,tid,sts3
#            sent        ,syncs  ,dest   arrow,tid,dest
#            -------NonRoot Nodes
#            created     ,data           
#            created     ,lsnrs
#            sent        ,lsnrs  ,dest   arrow,lid,dest
#            received    ,lsnrs  ,src    arrow,src,lid
#            acked       ,lsnrs          box,lid,sts3
#            ------
#            populated   ,data           box,did,sts
#            started     ,task           box,tid,sts
#            finished    ,task           box,tid,sts
#            woken       ,lsnr           box,lid,sts
#            sent        ,data   ,dest   arrow,did,dest
#            received    ,data   ,src    arrow,src,did
#            acked       ,data           box,lid,sts
#            sent        ,termOK         box,tid,sts
#            acked       ,termOK         box,tid,sts
            
#  for each event in event_list: 
#       draw event
#       snapshot
