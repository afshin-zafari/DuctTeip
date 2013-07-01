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
        if group == PureMPI:
            if abs(NpTot - 4200)<100: NpTot = 4200
            if abs(NpTot - 8400)<100: NpTot = 8400
            if abs(NpTot - 8192)<100: NpTot = 8192
            if abs(NpTot - 1000)<100: NpTot = 1000
                
        NpPerNc = NpTot/Nc
        NpPerNc2 = Nn*(Np*Np)/Nt
        out.append([Mtd,Nt,Nb,Np,Nn,pm,t2,Pc,CompT,CommN,CommT,CommW,Nc,NpTot,NpPerNc,NpPerNc2,0,0])
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
    global fig,ttlist,fcnt,group,lglist,lblist,fz,gsubplots,testno
    dbg=0
    flg=False
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
       if dbg:print lv,"+++",lb,vidx,start,ttlist
       if vidx == vlist[len(vlist)-2] :   
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
           if dbg:print lv,lb,"idx=",idx,v
           if dbg:print lv,">>>-----------------"
           recflt.sort(lambda x,y: x[x_axis_var]-y[x_axis_var])
           
           if dbg:print "yidx",y_axis_var,x_axis_var
           x = [r[x_axis_var] for r in recflt ]
           y = [r[y_axis_var] for r in recflt ]
           if group in ( SpeedUp,ScaleUp,WeakScaling):
               x,y = groupMax(x,y)
           else:
               x,y = groupMin(x,y)
           if dbg:print lv,">>>-----------------"
           if dbg:
               for i in range(len(x)):
                   print lv,x[i],y[i]
           ax1 = fig.add_subplot(111)

           if draw45:
                   #ax1.plot(x, x, 'b-')
               if max(x) < 10 : 
                   ax1.plot(range(10),range(10),'b--')                 
               else:
                   mx = max([r[Nc_COL] for r in rec ])
                   ax1.plot(range(mx),range(mx),'b--')
               ax1.set_xlabel(col_names[x_axis_var],fontsize=fz)
               ax2 = ax1
           else:
               ax2=fig.add_subplot(111)
               ax2.set_xlabel(col_names[x_axis_var],fontsize=fz)
           for tl in ax1.get_xticklabels():
               tl.set_fontsize(fz) 
           for tl in ax1.get_yticklabels():
               tl.set_fontsize(fz) 
           if testno != 22 : 
               lg,=ax2.plot(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='-',lw=2)
           else:               
               lg  =plt.Line2D(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='-',lw=1)
           lglist.append(lg)
           lblist.append(lb)
           if  group == PureMPI:
               if testno == 22 or  len (gsubplots) <=0:
                   ax3=ax2
               else:
                   ax3=ax2.twinx()
                   for tl in ax3.get_xticklabels():
                       tl.set_visible(False) 
                   for tl in ax3.get_yticklabels():
                       tl.set_fontsize(fz) 
               for s in gsubplots:
                   y=[r[s[SPLOT_YVAR]] for r in recflt ]
                   ax3.plot(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls=s[SPLOT_LS],lw=1)
                   ax3.plot([0],[0] , color='w',marker='',lw=.2)
                   ax2.plot([0],[1] , color='w',marker='',lw=.2)
                   if len(s[SPLOT_YLBL]) >0:
                       ax3.set_ylabel(s[SPLOT_YLBL],fontsize=fz)
                   if dbg:print vidx,idx,Nt_COL,Nn_COL
                   if dbg:print gvdict.keys()
                   if gvdict.keys()[idx] == max(gvdict.keys()):
                       lg  =plt.Line2D(x[1:], y[1:], color='k',marker='',ls=s[SPLOT_LS],lw=1)
                       lglist.append(lg)
                       lblist.append(s[SPLOT_LEGEND])
               
           if testno != 22 : 
               ax2.set_ylabel(col_names[y_axis_var],fontsize=fz)
    if len(lglist)>0 and ( vidx != Mtd_COL or group == MtdComp) :
        if dbg:print lv,"legend add"
        plt.legend(lglist,lblist,legendpos,shadow=True)
    lglist,lblist=list(),list()
    if dbg:print lv,'end of level' , start
    

fnameclean=''
group = PureMPI
lblist,lglist=list(),list()

if len(sys.argv) <2:
    print "Error: Incorrect number of arguments."
    print "Usage: python draw_mpi_hybrid.py <input file-name> "
    sys.exit(1) 

fname   = sys.argv[1]
rec     = readlist(fname)
recf    = [r for r in rec if r[T2_COL] >0 ]
ttlist  = ['','']
figsz   = (44,22)
testno=0

if group == PureMPI:
    legendpos='upper left'
    dbg=0

    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_Nc'
    recf=[r for r in rec if r[T2_COL] >0.001 ]
    recpure = list()
    for Np in [1000,3360,4200,5040,8400,8192]:
        recp = filter ( lambda x: x[NpTot_COL] == Np  , rec)
        if len(recp)<= 0 : continue
        if dbg:
            for r in recp :print "+",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
        for Nt in [1,3,5,7,2,4,8,6]:
            rect = [r for r in recp if r[Nt_COL] in ( Nt,Nt+0)] 
            if len(rect)<= 0 : continue
            if dbg: 
                for r in rect :print "++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
            for Nn in [1,2,3,4,5,6,7,8]:
                recn = [r for r in rect if r[Nn_COL] == Nn] 
                if len(recn)<= 0 : continue
                if dbg:
                    for r in recn :print "+++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
                lpure = [r for r in recn if r[PureMPI_COL] ==1 and r[Nt_COL] == Nt+0 ]
                lhyb =  [r for r in recn if r[PureMPI_COL] ==0 and r[Nt_COL] == Nt   ]
                if len(lpure) <=0 or len(lhyb) <= 0 : 
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
                               #Mtd,Nt  ,Nb    ,Np  ,Nn,PureMPI,T2  ,Pc  ,pT   ,mN   ,CommT ,mW,Nc   ,NpTot   ,NpPerNc   ,Np2,GF,RelT                      
                recpure.append([0  ,Nt+0,thtp1 ,tpt ,Nn,0      ,thyb,tht ,thyb ,tptp1,tpure ,mW,Nn*Nt,Np      ,Np/(Nn*Nt),0  ,0 ,relT])



    figsz=(12,8)
    fz=24
    colors=['g','r','#20E0F0','b','k']
    legendpos='best'
    col_names[Nc_COL]=r"#Cores"
    col_names[RelT_COL] = r"Relative Execution Time($\frac{T_{MPI}}{T_{Hybrid}} $)"
    ftitle=''
    legendpos='best'

    TPURE_THRD1_COL,TPURE_COL,THYB_COL,THYB_THRD_COL,THYB_THRD1_COL,NWCOMM_COL,TPURE_THRD_COL=CommN_COL,CommT_COL,CompT_COL,Pc_COL,Nb_COL,Np_COL,Np_COL

    #----------------------------------------------------T * Thread Cnt 
    col_names[Nt_COL]='Cores/Node'
    legendpos='upper left'
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'AbsTime_x_ProcCnt_'
                      #SPLOT_YVAR     ,SPLOT_LS,SPLOT_YLBL                 ,SPLOT_LEGEND
    gsubplots.append([TPURE_THRD1_COL ,'--'    ,'Time * Processors'        ,r"$T_{MPI}\times p $"])
    gsubplots.append([THYB_THRD_COL   ,':'     ,''                         ,r"$T_{Hybrid}\times p$"])

    outfile=outfname+'CoreBased'
    if 0 : groupBydraw(recpure,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'NodeBased'
    if 0:groupBydraw(recnode,(NpTot_COL,Nn_COL),Nc_COL,False,0,RelT_COL)

    #----------------------------------------------------T * Thread Cnt +1
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'AbsTime_x_ThrdCnt+1_'
                      #SPLOT_YVAR     ,SPLOT_LS,SPLOT_YLBL                 ,SPLOT_LEGEND
    gsubplots.append([TPURE_THRD1_COL ,'--'    ,'Time * Cores/Node'        ,r"$T_{MPI}\times (c+1) $"])
    gsubplots.append([THYB_THRD1_COL  ,'-'     ,''                         ,r"$T_{Hybrid}\times (c+1)$"])

    outfile=outfname+'CoreBased'
    if 0 : groupBydraw(recpure,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)
    outfile=outfname+'NodeBased'
    if 0 : groupBydraw(recnode,(NpTot_COL,Nn_COL),Nc_COL,False,0,RelT_COL)


    #----------------------------------------------------Network Comm
    legendpos='lower center'
    col_names[Nt_COL]='Core/Node'
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'NetworkComm_'
                      #SPLOT_YVAR,SPLOT_LS,SPLOT_YLBL,SPLOT_LEGEND=0,1,2,3
    gsubplots.append([CommW_COL          ,'--'    ,r'Relative Comm.Work($\frac{Pure}{Hybrid}$)',r"$\frac{p((n-1)c)}{n^2}\frac{\frac{N}{p}}{\frac{N}{n}}$"])

    outfile=outfname+'CoreBased'
    if 0:groupBydraw(recpure,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'NodeBased'
    if 0:groupBydraw(recnode,(NpTot_COL,Nn_COL),Nc_COL,False,0,RelT_COL)


    #----------------------------------------------------  T * Proc  Cnt 
    col_names[THYB_THRD_COL]=r"$T_{Hybrid}\times p$"
    col_names[Nt_COL]='Cores/Node'
    legendpos='best'
    testno=22
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'AbsTime_x_P&P+1_'
                      #SPLOT_YVAR     ,SPLOT_LS,SPLOT_YLBL                ,SPLOT_LEGEND
    gsubplots.append([THYB_THRD1_COL  ,'-'    ,'Total CPU-time'           ,r"$T_{Hybrid}\times (p+1)$"])
    gsubplots.append([THYB_THRD_COL   ,'-'    ,''                         ,r"$T_{Hybrid}\times p$"])
#   gsubplots.append([TPURE_THRD1_COL ,'--'   ,''                         ,r"$T_{MPI}\times (p+1) $"])
    gsubplots.append([TPURE_THRD_COL  ,'--'   ,''                         ,r"$T_{MPI}\times p $"])
 

    outfile=outfname+'CoreBased_1357'
    recp = [r for r in recpure if r[Nt_COL] in (1,3,5,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased__357'
    recp = [r for r in recpure if r[Nt_COL] in (3,5,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)


    outfile=outfname+'CoreBased__3_7'
    recp = [r for r in recpure if r[Nt_COL] in (3,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased_1248'
    recp = [r for r in recpure if r[Nt_COL] in (1,2,4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased__248'
    recp = [r for r in recpure if r[Nt_COL] in (2,4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)


    outfile=outfname+'CoreBased__4_8'
    recp = [r for r in recpure if r[Nt_COL] in (4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)


    testno=23
    gsubplots=list()
    outfname=fname.strip('rec.txt')+'RelTime_'

    outfile=outfname+'CoreBased__357'
    recp = [r for r in recpure if r[Nt_COL] in (3,5,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased__3_7'
    recp = [r for r in recpure if r[Nt_COL] in (3,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased_1357'
    recp = [r for r in recpure if r[Nt_COL] in (1,3,5,7) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased__248'
    recp = [r for r in recpure if r[Nt_COL] in (2,4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased__4_8'
    recp = [r for r in recpure if r[Nt_COL] in (4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased_1248'
    recp = [r for r in recpure if r[Nt_COL] in (1,2,4,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    outfile=outfname+'CoreBased_24678'
    recp = [r for r in recpure if r[Nt_COL] in (2,4,6,7,8) ]
    groupBydraw(recp,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)


