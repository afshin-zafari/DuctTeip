#!/usr/bin/env python

import sys
import pylab as plt
import os
import math

def create_hash(s,n,p,g):
    n1,n2,p1,p2=0,0,0,0
    #0123456789-1234
    #D000_01--002_02
    w1,w2=3,2
    if s[0] =='P':
        n1=int(s[1      : w1+1      ])
        p1=int(s[w1+1+1 : w1+1+1+w2 ])
    if s[0] =='D':
        n1=int(s[1                  : w1+1                    ])
        p1=int(s[w1+1+1             : w1+1+1+w2-1             ])
        n2=int(s[w1+2+w2-1+3        : w1+2+w2-1+3+w1-1        ])
        p2=int(s[w1+2+w2-1+3+w1-1+2 : w1+2+w2-1+3+w1-1+2+w2-1 ])

    print s,n1,p1,n2,p2,s[1    : w1+1      ],s[w1+2 : w1+2+w2 ]
    h=n1*p+p1
    return h

def get_hash(s,n,p,g,nid):
    n1,p1,n2,p2=0,0,0,0
    offset1,offset2=0,n*p
    if s[0] =='P':
        s1=s[1:].split('_')
        n1=int(s1[0])
        p1=int(s1[1])-1
        h=n1*p+p1+offset1
    if s[0] =='D':
        s1=s[1:].split('--')
        s2=s1[0].split('_')
        n1=int(s2[0])
        p1=int(s2[1])-1
        s2=s1[1].split('_')
        n2=int(s2[0])
        p2=int(s2[1])-1
        h=(n1*p+p1)*n*p+n2*p+p2+offset2

    if g!=0:
        if s[0]!='P':
            #print g,h,s[0]!='P',nid,nid*n*p*p
            h = h- nid*n*p*p
        if nid!= n1:
            print n1
            return 0
    return h


prg=4
nrg=100
prn=0
grp=1
hl,sl = list(),list()
for i in range(0,nrg):
    for j in range(1,prg+1):
        s='P'+('%03d' % i)+'_'+('%02d' % j)
        h=get_hash(s,nrg,prg,grp,i)
        hl.append(h)
        sl.append(s)
        if prn:print s,h
        for k in range(0,nrg):
            for l in range(1,prg+1):
                s='D'+('%03d' % i)+'_'+('%02d' % j)
                s=s+'--'+('%03d' % k)+'_'+('%02d' % l)
                h=get_hash(s,nrg,prg,grp,i)
                hl.append(h)
                sl.append(s)
                if prn:print s, h,i

dc = dict ( zip ( hl,sl) ) 
print len (dc), (nrg*prg)**2+(nrg*prg),nrg*prg*prg+prg*nrg
