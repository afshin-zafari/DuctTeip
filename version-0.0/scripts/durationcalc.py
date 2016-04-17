#!/usr/bin/env python

import sys
#import pylab
import os
import math


Tm_COL,Ev_COL,P1_COL,P2_COL,P3_COL=1,2,3,4,5

def readfile(filename,final):
    global pureMPI
    fh = open(filename, "r")
    header = fh.readline()
    while len(header) != 0:
        line=header.split(',')        
        if len(line)>1:
            line=header.split(',')
            if len(line[Tm_COL].strip())>0:
                tm = float(line[Tm_COL].strip(' '))
                ev=int(line[Ev_COL])
                p1=line[P1_COL].strip()
                p2=int(line[P2_COL])
                p3=int(line[P3_COL])
                final.append([0,tm,ev,p1,p2,p3] ) 
        header = fh.readline()    
    fh.close()
    return final

def writerecords(rec,fname):
    fh = open(fname,'a')
    for r in rec:
        sv=''
        for i in range(len(r)):
            v = r[i]
            if  i == Tm_COL:
                sv=sv+ '{0:05.3f}'.format(v/1000000)+','  
            else:
                sv=sv+ str(v)+','  
        sv=sv.strip(',')+'\n'
        fh.write(sv)
    fh.close()


M1='E1'
M2='E2'
tot_only = 0
if len(sys.argv) >1:
    fname = sys.argv[1]
if 0 : 
    if len(sys.argv) >2:
        E1 = int(sys.argv[2])
        E2 = int(sys.argv[3])
    if len(sys.argv) >3:
        M1= sys.argv[4]
        M2= sys.argv[5]
    if len(sys.argv) >5:
        tot_only= int(sys.argv[6])


def calc_two_events():
    readfile(fname,records)
    rec = [r for r in records if r[Ev_COL] in ( E1,E2) ]
    t=0
    s=0.0
    mn =1e9
    mx=0
    cnt = 0
    for r in rec:
        if r[Ev_COL] == E2 and t != 0 :
            s = s+ r[Tm_COL]-t
            mx = max ( mx , r[Tm_COL]-t)
            mn = min ( mn , r[Tm_COL]-t)
            if tot_only ==0 :
                print M1,"-",M2,r[Tm_COL]-t
            cnt = cnt + 1
            t=0
        if r[Ev_COL] == E1 and t == 0 :
            t= r[Tm_COL]
        
    if cnt !=0 :
        if E1 == E2 : cnt = cnt + 1
        print 'Total, E1,-,E2,sum,Min,Max,Avg,Cnt'
        print 'Total', M1,'-',M2,s,mn,mx,s/cnt,cnt
    else:
        print 'Total', M1,'-',M2,'NOTHING'

def calc_single(src,dest):
    dbg=0
    c,d,w,e=0,0,0,0
    while len(src)>0:
        e = src[0][Ev_COL]
        p = src[0][P1_COL]
        s=[sr for sr in src if sr[Ev_COL] == e and sr[P1_COL] == p]
        w+=s[0][P3_COL]
        c+=1
        if dbg:print 'S0',s[0]
        if len(s) >1:
            d+=s[1][Tm_COL] - s[0][Tm_COL]
            if dbg:print 'S1',s[1]
            del src[0:2]
        else:
            del src[0]
        if dbg:print 'SS',e,p,c,d,w
    dest.append([e,c,d,w])
    if dbg:print '\nD:',dest[-1]


def calc_couple(src,dest,e1,e2):
    dbg=1
    c,d,w,e=0,0,0,0
    for i in range(0,len(src)):
        if src[i][Ev_COL] != e1:
            continue
        p = src[i][P2_COL]
        s=[sr for sr in src if sr[Ev_COL] == e2 and sr[P2_COL] == p]
        if len(s) ==0:
            continue
        w+=s[0][P3_COL]
        c+=1
        d+=s[0][Tm_COL] - src[i][Tm_COL]
        if dbg:
            print 'S0',src[i]
            print 'S1',s[0]
        src[i][Ev_COL]=-e1
        src[i][P2_COL]=-p
        s[0][Ev_COL]=-e2

        if dbg:print 'SS',e1,c,d,w
#        dest.append([e1,c,d,w])
        dest.append([e1,p,d,0])
        if dbg:print '\nD:',dest[-1]
    dur_avg=[0,0,0,0,0]
    for i in range(1,6):
        dur = [r[2] for r in dest if r[1] == i]
        if len(dur) == 0 : continue
        dur_avg[i-1] = sum(dur) / len(dur)
        print 'avg',i,dur_avg[i-1]*1e6,len(dur)

def calc_couple_subtasks(org,dest,e1,e2):
    dbg=1
    d=0.0
    subt=list()
    dur_avg=[0,0,0,0,0]

    for p in range(1,6):
        src= [s for s in org if s[P2_COL] == p]
        for i in range(0,len(src)):
            if src[i][Ev_COL] != e1:
                continue
            b = src[i][P3_COL]            
            s=[sr for sr in src if sr[Ev_COL] == e2 and sr[P3_COL] == b]
            if len(s) ==0:
                continue
            for j in range(0,len(s)):
                d=s[j][Tm_COL] - src[i][Tm_COL]
                if d<0: 
                    d=0.0
                    continue
                if dbg:
                    print 'S1',src[i]
                    print 'S0',s[j]
                src[i][Ev_COL]=-e1
                src[i][P2_COL]=-p
                s[j][Ev_COL]=-e2
                break

            if dbg:print 'SS',e1,b,d*1e6
            dest.append([e1,p,d,0])
            if dbg:print '\nD:',dest[-1]
    for i in range(1,6):
        dur = [r[2] for r in dest if r[1] == i]
        if len(dur) == 0 : continue
        dur_avg[i-1] = sum(dur) / len(dur)
        if dbg and i == 4:
            for o in dur:
                print o*1e6
        print 'avg',i,dur_avg[i-1]*1e6,len(dur)
    dest = dur_avg

        
def calc_all_events_in_files(lFiles):
    dbg=1
    evDataSend=1011;evDataRecv=1019
    evTaskStart=300;evTaskFinish=500
    evSubTaskStart=1017;evSubTaskFinish=1018
    lFinal,lSummary=list(),list()
    def print_final():
        if dbg:
            print '-'*70
            for f in lFinal:
                print f
    def print_summary():
        print '*'*70
        for s in lSummary:
            print s
    def event_grand_total(e1,e2,cat):
        lTot=[t for t in lFinal if t[0] in (e1,e2 ) ]
        w = sum ( [t[3] for t in lTot] ) 
        d = sum ( [t[2] for t in lTot] ) 
        c = sum ( [t[1] for t in lTot] ) 
        lSummary.append([cat,c,d,w])
        
    def event_sub_total2(e1,e2):
        lData=[t for t in lTime if t[Ev_COL] in ( e1,e2)]
        if dbg:
            for d in lData:
                print d
        calc_couple_subtasks(lData,lFinal,e1,e2)

    def event_sub_total(ev):
        lData=[t for t in lTime if t[Ev_COL] ==ev]
        if dbg:
            for d in lData:
                print d
        calc_single(lData,lFinal)

    for fname in lFiles:
        lTime=list()
        if dbg: print fname
        readfile(fname,lTime)
#        event_sub_total(evDataSend)
#        event_sub_total(evDataRecv)
#        event_sub_total2(evTaskStart,evTaskFinish)
        event_sub_total2(evSubTaskStart,evSubTaskFinish)
    print_final()
#    event_grand_total(evDataSend,evDataRecv,'Comm')
#    event_grand_total(evTaskStart,evTaskFinish,'Comp')
    print_summary()
    if len(lSummary)>1: 
        return [lSummary[1][2],lSummary[0][1],lSummary[0][2],lSummary[0][3]]
    else:
        return [-1,-1,-1,-1]
                  
    
#print "<--",calc_all_events_in_files(sys.argv[1:])
