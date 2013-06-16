 #!/usr/bin/env python

import sys
#import pylab as plt
from pylab import *
import os
import numpy as np


Node_COL,CtxIn_COL,CtxSkip_COL,TaskRead_COL,TaskIns_COL,TaskProp_COL,CommCost_COL,TotTask_COL,Nb_COL,Proc_COL=range(0,10)

def readfile(filename,final):
    fh = open(filename, "r")
    header = fh.readline()
    while len(header) != 0:
        if "Node" in header:
            header = fh.readline()    
            continue
        line=header.split('\t')        
        if len(line)>1:
            line=header.split('\t')
            node_str=line[0].split(':')
            node=int(node_str[1])
            ci= int(line[CtxIn_COL])
            cs= int(line[CtxSkip_COL])
            tr= int(line[TaskRead_COL])
            ti= int(line[TaskIns_COL])
            tp= int(line[TaskProp_COL])
            cc= int(line[CommCost_COL])
            tt= int(line[TotTask_COL])
            nb= int(line[Nb_COL])
            pr= int(line[Proc_COL])            
	    final.append([node,ci,cs,tr,ti,tp,cc,tt,nb,pr])
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

f=list()
readfile("b.txt",f)
stat=np.array(f)
tt= stat[0,TotTask_COL]*1.0
nb= stat[0,Nb_COL]
np= stat[0,Proc_COL]

tpt = sum(stat[:,TaskProp_COL])*1.0
tr = stat[:,TaskRead_COL]/tt
ti = stat[:,TaskIns_COL]/tt
tp = stat[:,TaskProp_COL]/tpt

fg = plt.figure()
ax = fg.add_subplot(111)
ax.set_title('Nb = ' + str(nb)+ ', Total Tasks = '+ str(int(tt)))
axis ([0,np,0,1.0])

b1=plt.bar(stat[:,Node_COL]    ,tr,.3)
b2=plt.bar(stat[:,Node_COL]+0.3,ti,.3,color='r')
b3=plt.bar(stat[:,Node_COL]+0.6,tp,.3,color='g')
plt.legend((b1[0],b2[0],b3[0]),('Task Read','Task Insert','Task Propagate'))

fg.savefig('taskstat_nb-'+str(nb)+'_np-'+str(np)+'.png')
cp_tot = tt                         # Comput. Cost Total
cp_avg = cp_tot / len(f)            #              Avg
cc_tot = sum(stat[:,CommCost_COL])  # Commun. Cost Total
cc_avg = cc_tot / len(f)            #              Avg

a = [np,nb,cp_tot,0     ,cp_tot   ,0        ,1]  # StarPU
s = ''
for e in a:
    s = s +str(e)+'\t' 
print s
a=[ np,nb,cp_avg,cc_avg,cp_tot   ,cc_tot   ,2]  # ClusterSs
s = ''
for e in a:
    s = s +str(e)+'\t' 
print s

cp_tot = cp_tot + sum (stat[:,CtxIn_COL])   + sum (stat[:,CtxSkip_COL])
cc_tot = cc_tot + sum (stat[:,TaskProp_COL])  
cp_avg = cp_tot / len(f) 
cc_avg = cc_tot / len(f) 
a=[ np,nb,cp_avg,cc_avg,cp_tot,cc_tot,0]  # DuctTeip
s = ''
for e in a:
    s = s +str(e)+'\t' 
print s
