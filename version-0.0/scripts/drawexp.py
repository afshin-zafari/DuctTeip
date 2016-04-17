#!/usr/bin/env python

import sys
#import pylab
import os
import math
import durationcalc as dc

def eventfilter(x):
    global gEvent
    return x[0] == gEvent
    
def readlist(fh):    
    out = list()
    line=fh.readline()
    while len(line) != 0:
        stline=line.strip().split(',')
        if (stline[Mtd_COL] == 'Mtd'): 
            return out
        if len(stline) != 6: 
            line = fh.readline()
            continue 
        event = int(stline[Ev_COL])
        time=float(stline[Tm_COL])
        out.append([event,time])
        line = fh.readline()
    return out

def list_summary(inlist,event):
    global gEvent
    gEvent = event
    inlist = filter(eventfilter,inlist)
    ls = list()
    for i in range(0,len(inlist)-1,2):
        #print "+++",inlist[i+1][1]-inlist[i][1],inlist[i+1][1],inlist[i][1]
        ls.insert(-1,inlist[i+1][1]-inlist[i][1])
    if len(ls)<1 :
        return 0
    return max(  min(ls) , 0 ) 


def readfile(filename):    
    global pureMPI,smry,cacheOpt
    dbg=1
    final = list()
    fh = open(filename, "r")
    header = fh.readline()
    while len(header) != 0:
        line=header.split(' ')
        if dbg:print '->',line
        if ( line[0] == 'Seq') : 
            header = fh.readline()
        line=header.split(' ')
        if dbg:print '-->',line
        final.append([int(line[Mtd_COL]),int(line[Nt_COL]),int(line[Nb_COL]),int(line[Np_COL]),int(line[Nn_COL]),pureMPI,0.0,partCnt,\
                      cacheOpt,smry[0],smry[1],smry[2],smry[3]])
        tmlist=readlist(fh)
        if dbg:print filename
        if dbg:
            for t in tmlist:
                print t
        final[len(final)-1][T2_COL] = list_summary(tmlist,EVENT2)
        if dbg: print final[len(final)-1][T2_COL] 
        header = fh.readline()    
    fh.close()
    return final

def extract(inlist,idx):
    exlist=list()
    for i in range(len(inlist)):
        exlist.append(inlist[i][idx])
    return exlist

def writerecords(rec,fname):
    fh = open(fname,'a')
    for r in rec:
        sv=''
        for i in range(len(r)):
            v = r[i]
            if  i in ( T2_COL,CompT_COL,CommT_COL):
                sv=sv+ '{:05.3f}'.format(v/1.0)+','  
            else:
                sv=sv+ str(v)+','  
        sv=sv.strip(',')+'\n'
        #print sv
        fh.write(sv)
    fh.close()

global gEvent,EVENT1,EVENT2,smry
global Mtd_COL, Nn_COL,Nb_COL,Np_COL,Nt_COL,PureMPI_COL,T2_COL,Ev_COL,Tm_COL,cacheOpt

gEvent = 0
EVENT1,EVENT2=1020,1021
Mtd_COL,Nt_COL,Nb_COL,Np_COL,Nn_COL,PureMPI_COL,T2_COL,Pc_COL,CacheOpt_COL,CompT_COL,CommN_COL,CommT_COL,CommW_COL=range(0,13)
Tm_COL,Ev_COL=1,2

smry=[0,0,0,0]
if len(sys.argv) >1:
    fname = sys.argv[1]
    outfname = sys.argv[2]
    pureMPI = int(sys.argv[3])
    partCnt=int(sys.argv[4])
    cacheOpt=int(sys.argv[5])
    if len(sys.argv)>6:
        print sys.argv[6:]
        smry=dc.calc_all_events_in_files(sys.argv[6:])
records=readfile(fname)
writerecords(records,outfname)
Mtd = extract(records,Mtd_COL)
Nt  = extract(records,Nt_COL)
Nb  = extract(records,Nb_COL)
Np  = extract(records,Np_COL)
Nn  = extract(records,Nn_COL)
T1  = extract(records,PureMPI_COL)
T2  = extract(records,T2_COL)

#print "Mtd",Mtd,Nt,Np,Nb,Nn,T1,T2,smry

