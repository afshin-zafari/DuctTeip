#!/usr/bin/env python

import sys
#import pylab as plt
import os
import math
import random as rnd

gr=list()
dbg=0

def dest_node(r,g):
    return (r/g)*g
def pack_dest_node(r,g):
    if r%g==0:
        return dest_node(r/g,g)
    return -1

def graph(r,g,n):
    up=(r/g)    
    if r%g==0:up-=1
    low=[r*g+1,(r+1)*g]
    low[1] =min(n-1, low[1])
    if low[0] >=n: low=[]
    return [up,low]

def coll(i,gr):
    r = [g[0] for g in gr if g[1] == i]
    if len(r)<=0:
        print i,"Is Not Up"
        return False
    return  gr[i][2] != [min(r),max(r)]

def chain(i,gr,in_r=list()):
    if gr[i][1]==0:
        return 0
    for g in gr:
        if len(g[2])>0:
            if i >=g[2][0] and i <=g[2][1]:
                return [g[0],chain(g[0],gr)]

def chain_down(i,gr):
    if len(gr[i][2])<=0:
        return
    if dbg:print i,'-->',gr[i][2]
    for r in range(gr[i][2][0],gr[i][2][1]+1):
        if r < len(gr):
            chain_down( r,gr)



def add_mat(a,b,n):
    mat=n*n*[0]
    for i in range(0,n):
        for j in range(0,n):
            mat[i*n+j]=  a[i*n+j]+b[i*n+j]
    return mat

def print_mat(mat,n):
    for i in range(0,n):
        s=''
        for j in range(0,n):
            s+= ( mat[i*n+j]==0)*'.'+( mat[i*n+j]!=0)*'*'
        print s


def trans(mat,n):
    trs=n*n*[0]
    for i in range(0,n):
        for j in range(0,n):
            trs[j*n+i] =  mat[i*n+j]
    return trs
    
def adj_mat(gr):
    n=len(gr)
    mat=n*n*[0]
    for g in gr:
        mat[g[0]*n+g[1]]=1
    return mat

n=8
g=2

        
for i in range(0,n):
    upp,low=graph(i,g,n)
    gr.append([i,upp,low])

for i in range(0,11):
    if coll(i,gr):
        print "Coll" , i


for r in gr:
    if len(r[2])>=0:
        if dbg:print r

if dbg:
    if (n-1)%g != 0: 
        print "Node ",(n-1)/g, "wait for ", (n-1)%g , "messages:"
        print gr[(n-1)/g][2]

if dbg:
    for i in range(2,n,2*g):
        print "----------------"
        print "Chain Up Of",i,':',
        print chain(i,gr)
        if i<n/g:
            print "Chain Down Of",i,':'
            chain_down(i,gr)

m=adj_mat(gr)
t=trans(m,n)
#print_mat (m,n)
#print_mat (t,n)
t[0]=0
#print_mat (add_mat(m,t,n),n)


Cap=10
tw=n*[0]
twb=n*[0]
bal=[]
pcklist=[]
pckcntz=n*[0]
for i in range(0,n):
    bal.append ([[i,0]])
    pcklist.append ([[i,0]])

def xfer_work(f,t,w,up=0):
    if up==0:
        bal[f][0][1] -=w
        bal[t][0][1] +=w
    tw[f] -= w
    tw[t] += w

def local_clr(bl,up=0):
    mni,mxi=0,0
    mn,mx=0,0
    if dbg:print "locl clr of ",
    for i in range(0,len(bl)):
        if bl[i][1]>mx:mx=bl[i][1];mxi=i
        if bl[i][1]<mn:mn=bl[i][1];mni=i
        if dbg:print bl[i],
    if dbg:print ' '
    if mx*mn==0: return 0
    if dbg:print bl[mni],'-',bl[mxi]
    w= min (abs(bl[mni][1]),mx)
    bl[mni][1] += w
    bl[mxi][1] -= w
    xfer_work(bl[mxi][0],bl[mni][0],w,up)
    if dbg:print bl[mni],'-',bl[mxi]

    
def send_pack(i,b):
    if i<0:return 0
    up=gr[i][1]
    if up<0:up=0
    #if i==up:return 0
    pckcntz[up] +=1
    if dbg:print '\t',pckcntz[up],i,"->",up,b
    for j in range(1,len(b)):
        bb=b[j]
        if bb[1]!=0:
            pcklist[up].append ( bb) 

def balance_Ln(nn):
    for up in range(nn-1,-1,-1):
        if pckcntz[up] >0:
            if dbg:print "###",up
            while local_clr(pcklist[up],1) !=0 :
                continue
            send_pack( gr[up][1] , pcklist[up] ) 
            pcklist[up] =[]
            pckcntz[up] =0
            return 1
    return 0
    
def send(i,w):
    bal[i][0][1]=w
    if gr[i][1]>=0:
        bal[ gr[i][1] ].append([i,w])

def balance_L1(i):
    up = gr[i][1]
    l =len(bal[up]) 
    if dbg:print "===",i,up,l,bal[up]
    if dbg:print l>g ,  ((n-1)%g != 0) , ((n-1)%g==l-1)

    if l>g or ( ((n-1)%g != 0) and ((n-1)%g==l-1)):
        if dbg:print i,"**",bal[ gr[i][1] ]
        while local_clr(bal[gr[i][1]]) !=0:
            continue
        send_pack(gr[i][1],bal[gr[i][1]])
        bal[gr[i][1]]=[]
# (r,w)
rnd.seed()
for i in range(0,n):
    tw[i]=rnd.randint(0,1.5*Cap)
    twb[i]=tw[i]

for i in range(0,n):
    send(i,tw[i]-Cap)

if dbg:
    for i in range(0,n):
        print "::",bal[i]

for i in range(0,n):
    balance_L1(i)

if dbg:
    for i in range(0,n):
        print pcklist[i],pckcntz[i]

while  balance_Ln(n) !=0:
    if dbg:
        for i in range(0,n):
            print pcklist[i],pckcntz[i]
    continue
if dbg:
    for i in range(0,n):
        if len(bal[i])>1:
            print "++++++"
            print bal[i]
        if len(pcklist[i])>1:
            print pcklist[i]
            print "++++++"

if 1 or dbg:
    for i in range(0,n):
        print i,'\t', twb[i],'\t',tw[i],'\t',twb[i]-Cap,'\t',tw[i]-Cap
    
print "-------------"
lb= len ([t for t in twb if t-Cap>0]) 
print lb
l= len ([t for t in tw  if t-Cap>0]) 
print l
if l<10:
    print [t for t in tw  if t-Cap>0]
