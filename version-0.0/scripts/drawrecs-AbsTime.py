7#!/usr/bin/env python

import sys
import pylab as plt
import os
import math
#0              1          2         3          4
Mtd_COL      ,Nt_COL     ,Nb_COL   ,Np_COL    ,Nn_COL   , \
PureMPI_COL  ,T2_COL     ,Pc_COL   ,CompT_COL ,CommN_COL, \
CommT_COL    ,CommW_COL  ,Nc_COL   ,NpTot_COL ,NpPerNc_COL, \
NpPerNc2_COL ,GFLOPS_COL ,RelT_COL \
=range(0,18)
Color_COL,Mark_COL,Label_COL=0,1,2
SPLOT_YVAR,SPLOT_LS,SPLOT_YLBL,SPLOT_LEGEND=0,1,2,3
SeqParComp,NpPerCore,MtdComp,NtComp,NbComp,SpeedUp,PureMPI,Cholesky,FixedSize,NodeImpact,WeakScaling,ScaleUp=range(1,13)
col_names=dict(zip([Mtd_COL,NpTot_COL,Nb_COL,Nc_COL,Nt_COL,Nn_COL,NpPerNc_COL,PureMPI_COL,Np_COL,NpPerNc2_COL,T2_COL,RelT_COL,Pc_COL],
                   ['Method','# Points','#Block','#Core','Core Per Node','#Node', \
                    '#Points Per Core','PureMPI','Points Per Node','Data Size Per Core','Time(s)','Relative Time to One Core','Partition Per Node']))
mtd_names=dict(zip([3,2,1],
                   ['Auto','Pipe','Bcast']))
mark_list=('o','<','s','>','v','^','p','d')
ftitle = ''
draw_line=('-',':','-.','-',':','-.',',')
outfile=''    
lglist=list()
lblist=list()
gT1=0
fz=22
    
def readlist(fname):    
    out = list()
    fh = open(fname)
    line=fh.readline()
    while len(line) != 0:
        stline=line.strip().split(',')
        Mtd = int(stline[Mtd_COL])
        Nt = int(stline[Nt_COL])
        Np = int(stline[Np_COL])
        Nn = int(stline[Nn_COL])
        Nb = int(stline[Nb_COL])
        pm = int(stline[PureMPI_COL])

        CompT=float(stline[CompT_COL])
        CommT=float(stline[CommT_COL])
        CommW = int(stline[CommW_COL])
        CommN = int(stline[CommN_COL])
        t2=float(stline[T2_COL])
        Pc=1
        if Pc_COL<len(stline):
            Pc=int(stline[Pc_COL])
        Nc=Nt*Nn
        NpTot = (1-pm)*Nn*Np+pm*Nc*Np
        if group != WeakScaling and group != PureMPI:
            if abs(NpTot - 5040)<100: NpTot = 5040
        if group == PureMPI:
            if abs(NpTot - 4200)<100: NpTot = 4200
            if abs(NpTot - 8400)<100: NpTot = 8400
            if abs(NpTot - 8192)<100: NpTot = 8192
            if abs(NpTot - 1000)<100: NpTot = 1000
                
        NpPerNc = NpTot/Nc
        NpPerNc2 = Nn*(Np*Np)/Nt
        if 0 : 
            if NpPerNc%100<50:
                NpPerNc -= NpPerNc%100
            else:
                NpPerNc +=100 -NpPerNc%100
            if NpPerNc2%10<5:
                NpPerNc2 -= NpPerNc2%10
            else:
                NpPerNc2 +=10 -NpPerNc2%10

        if t2 ==0:
            gf = -1.0
        else:
            gf = int(math.ceil(float(NpTot * NpTot* 2000/t2/1e9)+0.5))
        out.append([Mtd,Nt,Nb,Np,Nn,pm,t2,Pc,CompT,CommN,CommT,CommW,Nc,NpTot,NpPerNc,NpPerNc2,gf,0])
        line = fh.readline()
    fh.close()
    return out

    
def groupMin(x,y):
    xr,yr=list(),list()
    d=dict(zip(x,y))
    dk = d.keys()
    dk.sort(lambda b,c:b-c)
    for v in dk:
        ym = max(y)
        for idx in range(len(x)):
            if x[idx]== v:
                if y[idx] < ym:ym=y[idx]
        xr.append(v)
        yr.append(ym)
    return [xr,yr]

def groupMax(x,y):
    xr,yr=list(),list()
    d=dict(zip(x,y))
    dk = d.keys()
    dk.sort(lambda b,c:b-c)
    for v in dk:
        ym = min(y)
        for idx in range(len(x)):
            if x[idx]== v:
                if y[idx] > ym:ym=y[idx]
        xr.append(v)
        yr.append(ym)
    return [xr,yr]

def perfect_line(ax,x,y,idx):
    y2=list()
    print "global T1",gT1,x[0],y,"Y0",x[0] * gT1 * y 
    for i in range(0,len(x)):
        y2.append(x[i] * gT1 * y )
    lg,=ax.plot(x,y2,'--b')
    ax.plot(x,y2,mark_list[idx]+'--b')
    return lg


def groupBydraw(rec,vlist,x_axis_var=Nc_COL,draw45=False,start=0,y_axis_var=T2_COL):
    global fig,ttlist,fcnt,group,lglist,lblist,fz,gsubplots,testno,hold_flag
    dbg=0
    flg=False
    if 1 or hold_flag==0:
        lglist=list()
        lblist=list()
    lv=''
    for i in range(start):
        lv= lv+'....'
    if dbg:print lv,'start of level' , start
    vidx=vlist[start]
    vval = [r[vidx] for r in rec ]
    gvdict=dict(zip(vval,range(len(vval) ) ) ) 
    keyslist = gvdict.keys()
    keyslist.sort(lambda x,y:x-y)
    if dbg:print "************",keyslist
    for v in keyslist:
       recflt= [r for r in rec if r[vidx] == v] 
       if vidx == Mtd_COL:
           lb=''# col_names[vidx]+'='+mtd_names[v]
       else:
           lb= col_names[vidx]+'='+str(v)
       ttlist[start] = lb
       if vidx == vlist[len(vlist)-2] :   
           if hold_flag == 0 :
               fig=plt.figure(None,figsize=figsz)
           plt.grid(True)
           t=''
           for idx  in range(len(ttlist)-1):
               t = t+ttlist[idx]+' , '
           t=t.strip(' ').strip(',')
           t='\n'+t.strip(' ').strip(',')+' '
           if vidx== Mtd_COL: t=''
           plt.title(ftitle+t,fontsize=fz)
       if start < len(vlist)-1:
           groupBydraw(recflt,vlist,x_axis_var,draw45,start+1,y_axis_var)
       if vidx == vlist[len(vlist)-2] :   
           fig.savefig(outfile+t.strip('\n')+'.png')
       if vidx == vlist[len(vlist)-1] :   
           idx = gvdict.keys().index(v) % len(mark_list)
           recflt.sort(lambda x,y: x[x_axis_var]-y[x_axis_var])
           
           x = [r[x_axis_var] for r in recflt ]
           y = [r[y_axis_var] for r in recflt ]
           if group in ( SpeedUp,ScaleUp,WeakScaling):
               x,y = groupMax(x,y)
           else:
               x,y = groupMin(x,y)
           ax1 = fig.add_subplot(111)
           for tl in ax1.get_xticklabels():
               tl.set_fontsize(fz) 
           for tl in ax1.get_yticklabels():
               tl.set_fontsize(fz) 
           ax2=ax1
           if 1:
               if testno == 24:
                   lg,=ax2.plot(x, y, color=colors[(idx+0)%len(colors)],marker=mark_list[idx+0],ls='',lw=2,ms=20)
               if testno == 23:
                   #print 23,idx,mark_list[idx],colors[idx%len(colors)],lb
                   lg,=ax2.plot  (x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='',lw=2,ms=20)
                   lg =plt.Line2D(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='',lw=2,ms=8 )
                   lglist.append(lg)
                   lblist.append(lb)
               
           ax1.set_xlabel(col_names[x_axis_var],fontsize=fz)
           if testno != 22 : 
               ax2.set_ylabel(col_names[y_axis_var],fontsize=fz)
    if len(lglist)>0 and ( vidx != Mtd_COL or group == MtdComp) :
        plt.legend(lglist,lblist,legendpos,shadow=True)
    if 1 or hold_flag==0 :
        lglist,lblist=list(),list()
    
def sortrecs(y,x,mag=1000):
    for idx in sort_fields:
        if x[idx] == y[idx]:
            idx = idx +1
        else:
            return int((x[idx] - y[idx])*mag)
    return 0
def writerecords(rec,fname,sep='\t'):
    rec.sort(sortrecs)
    for i in range(len(rec)-1):
        if len(rec[i]) >0 and len(rec[i+1]) >0: 
            if sortrecs(rec[i],rec[i+1],1000) == 0.0 :
                rec[i][NpTot_COL] =-1*rec[i][NpTot_COL]
                
    print len  ([r for r in rec if r[NpTot_COL] <0 ]),"records are filtered"
    
    rec = [r for r in rec if r[NpTot_COL] >0]
    fh = open(fname,'w')
    fh.write('Mtd{0}NpTot{0}Nc{0}Nn{0}Nt{0}T{0}{0}GF{0}Nb{0} Np {0}Pure{0}p/c{0}Mz/c{0}RelT_COL\n'.replace('{0}',sep))
    for r in rec:
        sv=''
#        print "####",r[NpTot_COL],r[Nn_COL],r[Nt_COL],r[RelT_COL]
        for i in sort_fields:
            v = r[i]
            sv=sv+ fields_fmt[i] % v + sep
        sv=sv.strip(',')+'\n'
        fh.write(sv)
    fh.close()
    return rec


fnameclean=''
group = 0
lblist,lglist=list(),list()

if len(sys.argv) <4:
    print "Error: Incorrect number of arguments."
    print "Usage: python drawrecs.py <input file-name> <output file-name> <group number>"
    sys.exit(1) 

fname      = sys.argv[1]
fnameclean = sys.argv[2]
group      = int(sys.argv[3])

rec        = readlist(fname)


sort_fields= (Mtd_COL    , NpTot_COL    , Nc_COL   , Nn_COL, Nt_COL, \
              T2_COL     , GFLOPS_COL   , Nb_COL   , Np_COL, PureMPI_COL, \
              NpPerNc_COL, NpPerNc2_COL , RelT_COL , Pc_COL, CompT_COL, \
              CommN_COL  , CommT_COL    , CommW_COL)
fields_fmt = dict ( zip( \
             [ Mtd_COL     , NpTot_COL    , Nc_COL   , Nn_COL , Nt_COL, \
               T2_COL      , GFLOPS_COL   , Nb_COL   , Np_COL , PureMPI_COL, \
               NpPerNc_COL , NpPerNc2_COL , RelT_COL , Pc_COL , CompT_COL, \
               CommN_COL   , CommT_COL    , CommW_COL],\
             ['%2d'        ,'%4d'         ,'%2d'     ,'%2d'   ,'%2d', \
              '%3.3f'      ,'%2d'         ,'%2d'     ,'%5d'   ,'%2d', \
              '%4d'        ,'%-9d'        ,'%3.3f'   ,'%2d'   ,'%3.3f', \
              '%3.3f'      ,'%3.3f'       ,'%3.3f']))
if len (fnameclean) >0:
    recf=[r for r in rec if r[T2_COL] >0.001 ]
    rec = writerecords(recf,fnameclean)
    
article = not True
rec     = [r for r in rec if r[T2_COL] >0 ]
ttlist  = ['','']
figsz   = (44,22)
fz=22
colors  = ['r','b']
if article:
    figsz=(12,8)
    colors=['k','k']
testno=0
hold_flag=0

   
if  group == PureMPI:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_'
    ftitle=' Pure MPI and Hybrid Methods Comparison'
    recf=rec
    dbg=0

    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_Nc'
    recpure = list()
    def hybrid_calc(ExtraThrd):
        recf=[r for r in rec if r[T2_COL] >0.001 ]
        for Np in [1000,3360,4200,5040,8400,8192]:
            recp = filter ( lambda x: x[NpTot_COL] == Np  , rec)
            if len(recp)<= 0 : continue
            if dbg:
                for r in recp :print "+",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
            for Nt in [1,3,5,2,4,6,7,8]:
                rect = [r for r in recp if r[Nt_COL] in ( Nt,Nt+ExtraThrd)] 
                if len(rect)<= 0 : continue
                if dbg: 
                    for r in rect :print "++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
                for Nn in [1,2,3,4,5,6,7,8]:
                    recn = [r for r in rect if r[Nn_COL] == Nn] 
                    if len(recn)<= 0 : continue
                    if dbg:
                        for r in recn :print "+++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
                    lpure = [r for r in recn if r[PureMPI_COL] ==1 and r[Nt_COL] == Nt+ExtraThrd ]
                    lhyb =  [r for r in recn if r[PureMPI_COL] ==0 and r[Nt_COL] == Nt   ]
                    if len(lpure) <=0 or len(lhyb) <= 0 : 
                        if dbg:
                            print len(lpure) , len(lhyb)
                        continue
                    tpure = sum ( [r[T2_COL] for r in lpure])/len(lpure)
                    thyb  = sum ( [r[T2_COL] for r in lhyb ])/len(lhyb)
                    wpure = sum ( [r[CommW_COL] for r in lpure])/len(lpure)
                    cpure = sum ( [r[CommN_COL] for r in lpure])/len(lpure)
                    whyb  = sum ( [r[CommW_COL] for r in lhyb]) /len(lhyb)
                    chyb  = sum ( [r[CommN_COL] for r in lhyb])/len(lhyb)
                    mW,mN=1,1
                    if whyb != 0 :
                        if wpure != 0 :
                            mW=wpure/whyb
                    if chyb != 0 :
                        mN=chyb*1.0/cpure
                    relT=(tpure/thyb)
                    tptp1=tpure *(Nn*(Nt+1))
                    thtp1=thyb  *(Nn*(Nt+1))
                    tht  =thyb  * Nt*Nn
                    tpt = tpure * Nt*Nn
                    mW = (Nn*Nt*(Nn-1)*Nt) *1.0/ (Nn*Nn*Nt) 
                    rtnwcomm = relT 
                    xNt=Nt+ExtraThrd
                                    #Mtd,Nt ,Nb    ,Np  ,Nn,PureMPI,T2  ,Pc  ,pT   ,mN   ,CommT ,mW,Nc    ,NpTot   ,NpPerNc    ,Np2,GF,RelT     
                    recpure.append([0   ,xNt,thtp1 ,tpt ,Nn,0      ,thyb,tht ,thyb ,tptp1,tpure ,mW,Nn*xNt,Np      ,Np/(Nn*xNt),0  ,0 ,relT])


    hybrid_calc(0)
    figsz=(12,8)
    fz=24
    colors=['g','r','#20E0F0','b','k']
    legendpos='best'
    if 0 : 
        col_names[RelT_COL] = r"Relative Execution Time($\frac{T_{Hybrid}}{T_{MPI}} $)"
        outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nc'
        groupBydraw(recpure,(NpTot_COL,Nt_COL),Nc_COL,False,0,T2_COL)
        recnode= [r for r in recpure if r[Nn_COL] in ( 1,2,4,8)]
        outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nn'
        groupBydraw(recnode,(NpTot_COL,Nn_COL),Nc_COL,False,0,T2_COL)

# Experiments
    col_names[Nc_COL]=r"#Cores"
    col_names[RelT_COL] = r"Relative Execution Time($\frac{T_{MPI}}{T_{Hybrid}} $)"
    fields_fmt[Pc_COL] = '%03.3f'
    fields_fmt[Nb_COL] = '%03.3f'
    fields_fmt[Np_COL] = '%03.3f'
    fields_fmt[CommN_COL] = '%03.3f'
    fields_fmt[CommW_COL] = '%03.3f'
    recc=writerecords(recpure,fnameclean)
    ftitle=''
    recnode= [r for r in recpure if r[Nn_COL] in ( 1,2,4,8)]
    legendpos='best'

    TPURE_THRD1_COL,TPURE_COL,THYB_COL,THYB_THRD_COL,THYB_THRD1_COL,NWCOMM_COL,TPURE_THRD_COL=CommN_COL,CommT_COL,CompT_COL,Pc_COL,Nb_COL,Np_COL,Np_COL

    #----------------------------------------------------  T * Proc  Cnt 
    outfname=fname.strip('rec.txt')+'NetworkComm_'
    col_names[THYB_THRD_COL]=r"$T_{Hybrid}\times p$"
    col_names[Nt_COL]='Cores/Node'
    legendpos='best'

    testno=23
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'RelTime_'

    outfile=outfname+'CoreBased_1357'
    recp = [r for r in recpure if r[Nt_COL] in (1,3,5,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    hold_flag = 1
    outfile=outfname+'CoreBased_24678'
    recp = [r for r in recpure if r[Nt_COL] in (2,4,6,7,8) ]
    fig=plt.figure(None,figsize=figsz)
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased_24678_both'
    recpure=list()
    testno=24
    col_names[Nt_COL]='p+1 vs. p'
    hybrid_calc(1)
    recp = [r for r in recpure if r[Nt_COL] in (2,4,6,7,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    hold_flag=0

    if 0 :
        for r in recp:
            print r[NpTot_COL],r[Nn_COL],r[Nt_COL],r[RelT_COL]

