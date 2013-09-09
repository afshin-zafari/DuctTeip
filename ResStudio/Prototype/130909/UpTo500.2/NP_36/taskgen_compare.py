 #!/usr/bin/env python

import sys
#import pylab as plt
from pylab import *
import os
import numpy as np

Np_COL,Nb_COL,CpAvg_COL,CcAvg_COL,CpTot_COL,CcTot_COL,Mtd_COL=range(0,7)

def readfile(filename,final):
    fh = open(filename, "r")
    lines = fh.readlines()
    for l in  lines:
        line = l.split('\t')        
        np = int(line[Np_COL])
        nb = int(line[Nb_COL])
        cpa= float(line[CpAvg_COL])
        cca= float(line[CcAvg_COL])
        cpt= float(line[CpTot_COL])
        cct= float(line[CcTot_COL])
        mtd= int  (line[Mtd_COL])
        final.append([np,nb,cpa,cca,cpt,cct,mtd])
    fh.close()
    return final

f= list()
readfile("c.txt",f)
for r in f:
    print r
np_par = f[0][Np_COL]
np_list  = [r for r in f if r[Np_COL] == np_par]
dt  = [r for r in np_list if r[Mtd_COL]==0]
sp  = [r for r in np_list if r[Mtd_COL]==1]
cs  = [r for r in np_list if r[Mtd_COL]==2]
nb_list = [d[Nb_COL]    for d in dt ]

dt_cp   = [d[CpAvg_COL] for d in dt ]
sp_cp   = [d[CpAvg_COL] for d in sp ]
cs_cp   = [d[CpAvg_COL] for d in cs ]

dt_cptot   = [d[CpTot_COL] for d in dt ]
sp_cptot   = [d[CpTot_COL] for d in sp ]

cp = list()
for i in range(0,len(dt_cptot)):
    cp.append( dt_cptot[i] / sp_cptot[i] ) 
ovh = int((sum(cp) / len(cp) - 1.00)*100)
print ovh,cp
nb_list.insert(0,0)
dt_cp.insert(0,-20)
sp_cp.insert(0,0)
cs_cp.insert(0,0)


fg = plt.figure(None,figsize=(22,16))

ax = fg.add_subplot(121)
ax.set_title('Average Computation Cost, ' + str(np_par)+' Processors \n Average Overhead: %'+str(ovh))
ax.set_xlabel('Problem Size (Nb x Nb)')

b1=plt.plot(nb_list,dt_cp,'go-')
b2=plt.plot(nb_list,sp_cp,'r-')
b3=plt.plot(nb_list,sp_cp,'bo')

plt.legend((b1[0],b2[0],b3[0]),('DuctTeip','StarPU','ClusterSs'),'upper left')


dt_cc   = [d[CcAvg_COL] for d in dt ]
sp_cc   = [d[CcAvg_COL] for d in sp ]
cs_cc   = [d[CcTot_COL] for d in cs ]


dt_cptot   = [d[CcTot_COL] for d in dt ]
sp_cptot   = [d[CcTot_COL] for d in cs ]

cp = list()
for i in range(0,len(dt_cptot)):
    cp.append( dt_cptot[i] / sp_cptot[i] ) 
ovh = int((sum(cp) / len(cp) - 1.00)*100)
print ovh,cp


dt_cc.insert(0,-20)
sp_cc.insert(0,0)
cs_cc.insert(0,0)

ax = fg.add_subplot(122)
ax.set_title('Average Communication Cost, ' + str(np_par)+' Processors \n Average Overhead: %'+str(ovh))
ax.set_xlabel('Problem Size (Nb x Nb)')

b1=plt.plot(nb_list,dt_cc,'go-')
b2=plt.plot(nb_list,sp_cc,'r-')
b3=plt.plot(nb_list,cs_cc,'bo-')

plt.legend((b1[0],b2[0],b3[0]),('DuctTeip','StarPU','ClusterSs'),'upper left')


dt_cp   = [d[CpTot_COL] for d in dt ]
sp_cp   = [d[CpTot_COL] for d in sp ]
cs_cp   = [d[CpTot_COL] for d in cs ]

plt.show()
fg.savefig('taskgen_compare_np-'+str(np_par)+'.png')
