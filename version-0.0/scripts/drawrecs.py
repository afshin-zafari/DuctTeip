#!/usr/bin/env python

import sys
import pylab as plt
import os
import math
#0              1            2            3            4
Mtd_COL      ,Nt_COL       ,Nb_COL     ,Np_COL       ,Nn_COL   , \
PureMPI_COL  ,T2_COL       ,Pc_COL     ,CacheOpt_COL ,CompT_COL, \
CommN_COL    ,CommT_COL    ,CommW_COL  ,Nc_COL       ,NpTot_COL, \
NpPerNc_COL  ,NpPerNc2_COL ,GFLOPS_COL ,RelT_COL                 \
=range(0,19)
Seq_COL=0
Color_COL,Mark_COL,Label_COL=0,1,2
SeqParComp,NpPerCore,MtdComp,NtComp,NbComp,SpeedUp,PureMPI,Cholesky,FixedSize,NodeImpact,WeakScaling,ScaleUp,CholSpeedUp=range(1,14)
col_names=dict(zip([Mtd_COL,NpTot_COL,Nb_COL,Nc_COL,Nt_COL,Nn_COL,NpPerNc_COL,PureMPI_COL,Np_COL,NpPerNc2_COL, \
T2_COL,RelT_COL,Pc_COL,GFLOPS_COL,CacheOpt_COL], \
                   ['SeqMode','# Points','#Block','#Core','Core Per Node','#Node', \
                    '#Points Per Core','PureMPI','Points Per Node','Data Size Per Core', \
                    'Time(s)','Relative Time to One Core','Partition Per Node','GFLOPS','CacheOpt']))
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
def gigaflops(rec,outfile):

    gffig=plt.figure(None,figsize=(22,12)) 
    plt.grid(True)
    color=['b','r']
    for Nn in [3,5,7]:
        for mpi in [0,1]:
            recf=[r for r in rec  if r[PureMPI_COL] == mpi ]
            recf=[r for r in recf if r[Nn_COL] == Nn ]
            x,y= list(),list()
            for p in range(500,20000,500):
                r = [r[GFLOPS_COL] for r in recf if r[NpTot_COL] == p]
                if len(r) >0:
                    maxgf = sum (r) / len(r)
                    print maxgf,p
                    y.append(maxgf)
                    x.append(p)            
            plt.plot(x,y,color[mpi]+draw_line[Nn/3])
            gffig.savefig(outfile.strip('.png')+str(mpi)+str(Nn))

    
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
        CacheOpt = int(stline[CacheOpt_COL])
        Nc=Nt*Nn
        NpTot = (1-pm)*Nn*Np+pm*Nc*Np
        if group != WeakScaling and group != PureMPI:
            if abs(NpTot - 5040)<100: NpTot = 5040
        if group == PureMPI:
            if abs(NpTot - 4200)<100: NpTot = 4200
            if abs(NpTot - 8400)<100: NpTot = 8400
            if abs(NpTot - 8192)<100: NpTot = 8192
            if abs(NpTot - 1000)<100: NpTot = 1000
        if group == CholSpeedUp:
            if abs(NpTot - 512 )<100: NpTot = 512
            if abs(NpTot - 1024)<100: NpTot = 1024
            if abs(NpTot - 2048)<100: NpTot = 2048
            if abs(NpTot - 8192)<100: NpTot = 8192
                
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
            gf = 1/t2 #int(math.ceil(float(NpTot * NpTot/t2/1e9)+0.5))
        out.append([Mtd,Nt,Nb,Np,Nn,pm,t2,Pc,CacheOpt,CompT,CommN,CommT,CommW,Nc,NpTot,NpPerNc,NpPerNc2,gf,0])
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
    global fig,ttlist,fcnt,group,lglist,lblist,fz
    lglist=list()
    lblist=list()
    lv=''
    for i in range(start):
        lv= lv+'....'
    print lv,'start of level' , start
    vidx=vlist[start]
    vval = [r[vidx] for r in rec ]
    gvdict=dict(zip(vval,range(len(vval) ) ) ) 
    keyslist = gvdict.keys()
    keyslist.sort(lambda x,y:x-y)
    print "************",keyslist
    for v in keyslist:
       recflt= [r for r in rec if r[vidx] == v] 
       if vidx == Mtd_COL:
           lb= col_names[vidx]+'='+mtd_names[v]
           print v,vidx,lb
       else:
           lb= col_names[vidx]+'='+str(v)
       ttlist[start] = lb
       print lv,"+++",lb,vidx,start,ttlist
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
           print lv,lb,"idx=",idx,v
           print lv,">>>-----------------"
           recflt.sort(lambda x,y: x[x_axis_var]-y[x_axis_var])
           
           print "yidx",y_axis_var,x_axis_var
           x = [r[x_axis_var] for r in recflt ]
           y = [r[y_axis_var] for r in recflt ]
           if group in ( SpeedUp,ScaleUp,WeakScaling,CholSpeedUp):
               x,y = groupMax(x,y)
           else:
               x,y = groupMin(x,y)
           print lv,">>>-----------------"
           for i in range(len(x)):
               print lv,x[i],y[i]
           ax1 = fig.add_subplot(111)

           if 0 and group == WeakScaling:
               if x_axis_var == Nc_COL:
                   for pidx in [-1,0]:
                       mnc= min ([r[Nt_COL] for r in recflt ]) 
                       if mnc ==1 and pidx == 0: continue
                       xp=x[pidx]
                       yp=y[pidx]
                       y1=yp-0.7
                       x1=xp +4
                       x2=x1+6.8
                       y2= y1+0.5
                       pcnt=min((y[pidx]/mnc)*100,1000)
                       if pcnt>=100: x2+=0.9
                       plt.plot([x[pidx],x1],[y[pidx],y2],'k-')
                       plt.fill([x1,x1,x2,x2],[y1,y2,y2,y1],fill=False)
                       plt.text(x1+1,(y1+y2)/2,'%2.1f' % pcnt+'%',horizontalalignment='left',verticalalignment='center',fontsize=16)
                   
                   
               
           if draw45:
               if group == SpeedUp:
                   for pidx in [5,8,10,13,17,22,len(y)-3]:
                       if pidx>len(y):break
                       cfg=[r for r in recflt if r[y_axis_var]==y[pidx]][0]
                       print cfg
                       Nt=cfg[Nt_COL];Nn=cfg[Nn_COL]
                       tag='%1d' % Nn + ' Node'
                       if Nn >1: tag +='s'
                       tag += ' x ' + '%1d' % Nt+ ' Core' 
                       if Nt >1: tag +='s'
                       xp=x[pidx]
                       yp=y[pidx]
                       x1=x[pidx]+4
                       x2=x1+22
                       y1=yp-1-3
                       if x2 >70: x2=69;x1=x2-22;y1 -=5
                       y2=y1+2.7
                       if x2==xp:
                           x2 += 0.3
                           plt.plot([xp,x1],[yp,y2],'k-')
                       else:
                           plt.plot([xp,x1],[yp,(y2+y1)/2],'k-')
                       plt.fill([x1,x1,x2,x2],[y1,y2,y2,y1],fill=False)
                       plt.text(x1+0.5,(y1+y2)/2,tag,horizontalalignment='left',verticalalignment='center',fontsize=fz)
                   
               if group ==  ScaleUp :
                   mnc= min ([r[Nt_COL] for r in recflt ]) 
                   lg_pline=perfect_line(ax1,x,mnc,idx)
                   print "Yact[0]",y[0]
                   if mnc >=5:
                       print ":::::::::::::::::",y[-1]/(x[-1]*gT1*mnc),((y[-1]/(x[-1]*gT1*mnc))*100)
                       
                       y1=y[-1]-5
                       pcnt=((y[-1]/(x[-1]*gT1*mnc))*100)
                       x1=x[-1]-1.2
                       if pcnt>90:y1-=12;x1 +=.05
                       if mnc == 7: x1 -=2 ;y1 +=5
                       if mnc == 5: y1 -=25;x1 -= 0.5
                       y2=y1+8
                       x2=x1+1.2
                       plt.plot([x[-1],x2],[y[-1],y2],'k-')
                       plt.fill([x1,x1,x2,x2],[y1,y2,y2,y1],fill=False)
                       plt.text(x1+.15,(y1+y2)/2,'%2.1f' % pcnt+'%',horizontalalignment='left',verticalalignment='center',fontsize=16)
                   if mnc ==1:
                       lglist.append(lg_pline)
                       lblist.append('Theoretical Speed')
               else:
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
#           lg,=ax2.plot(x, y, color=colors[0],marker=mark_list[idx],ls=draw_line[idx%len(draw_line)],lw=2)
           for tl in ax1.get_xticklabels():
               tl.set_fontsize(fz) 
           for tl in ax1.get_yticklabels():
               tl.set_fontsize(fz) 
           lg,=ax2.plot(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='-',lw=2)
           lglist.append(lg)
           lblist.append(lb)
           if 1 and group == PureMPI:
               ax3=ax2.twinx()
               y=[r[CommW_COL] for r in recflt ]
               ax3.plot(x, y, color=colors[idx%len(colors)],marker=mark_list[idx],ls='--',lw=1)
               ax3.plot([0],[0] , color='w',marker='',lw=.2)
               ax2.plot([0],[0] , color='w',marker='',lw=.2)
               for tl in ax3.get_xticklabels():
                   tl.set_visible(False) 
               for tl in ax3.get_yticklabels():
                   tl.set_fontsize(fz) 
               ax3.set_ylabel('Relative Comm. Volume',fontsize=fz)
               if idx ==0 :
                   lg  =plt.Line2D(x[1:], y[1:], color='k',marker='',ls='--',lw=1)
                   lglist.append(lg)
                   lblist.append(r"Network Comm. ($\frac {Hybrid}{MPI})$")
               
           if 0 and group ==  WeakScaling: 
               plt.plot([1.1],[0.1],'w.',lw=0)
           ax2.set_ylabel(col_names[y_axis_var],fontsize=fz)
           if group != PureMPI:
               for r in recflt:
                   print lv,r[Mtd_COL],r[Nb_COL],r[NpTot_COL],r[Nc_COL],r[T2_COL]
               print lv,"-----------------"
    if len(lglist)>0 : #and ( vidx != Mtd_COL or group == MtdComp) :
        print lv,"legend add",lblist
        plt.legend(lglist,lblist,legendpos,shadow=True)
    lglist,lblist=list(),list()
    print lv,'end of level' , start
    
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
    fh.write('Seq{0}NpTot{0}Nc{0}Nn{0}Nt{0}T{0}{0}GF{0}Nb{0} Np {0}Pure{0}p/c{0}Mz/c{0}RelT_COL\n'.replace('{0}',sep))
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

if group == NpPerCore:
    legendpos='lower right'
    outfile=fname.strip('.txt')+'_g2_'
    ftitle=' Time of execution for multi cores'
    recf=[r for r in rec if r[Mtd_COL] == 3 ]
    recf=[r for r in recf if r[Nb_COL] == 2 ]
    recf=[r for r in recf if r[NpPerNc_COL] >= 125 ]
    groupBydraw(recf,(Mtd_COL,NpPerNc_COL),Nc_COL,True)
   
if group == SeqParComp  :
    legendpos='upper left'
    ftitle=' Sequential and Parallel comparison'
    outfile=fname.strip('.txt')+'_seqparcomp_'
    recf=[r for r in rec if r[Mtd_COL] == 3 ]
    recf=[r for r in recf if r[Nb_COL] == 2 or (r[Nb_COL] == 1 and r[Nc_COL] == 1)]
    recf=[r for r in recf if r[Nc_COL] in [1,8,16,24,32,40,48,56,64] ]
    recf=[r for r in recf if r[NpTot_COL] < 10000 ]
    groupBydraw(recf,(Mtd_COL,Nc_COL),NpTot_COL)

    legendpos='upper left'
    ftitle=' Sequential and Parallel comparison'
    outfile=fname.strip('.txt')+'_parcomp_'
    recf=[r for r in rec if r[Mtd_COL] == 3 ]
    recf=[r for r in recf if r[Nb_COL] == 2 ]
    recf=[r for r in recf if r[Nc_COL] in [8,32,40,48,56,64] ]
    recf=[r for r in recf if r[NpTot_COL] < 10000 ]
    groupBydraw(recf,(Mtd_COL,Nc_COL),NpTot_COL)

if group == NbComp  :
    legendpos='upper right'
    outfile=fname.strip('.txt')+'_bxcomp_20K+_'
    ftitle=' Block Count Comparison'
    recf=[r for r in rec if r[Mtd_COL] == 3 ]
    groupBydraw(recf,(Np_COL,Nt_COL),Nb_COL)

if group == MtdComp:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_mxcompu10K_'
    ftitle=' Distribution Method Comparison'
    figsz=(10,8)
    fz=24
    colors=['g','r','k','m','#20E0F0']
    recf=[r for r in rec if r[Nb_COL] == 4 ]
    recf=[r for r in recf if r[NpTot_COL] in range( 1000,10000+1) ]
    print "Records Length",len(recf)
    groupBydraw(recf,(Mtd_COL,Mtd_COL),Nn_COL)

    legendpos='upper left'
    outfile=fname.strip('.txt')+'_mxcomp10K+_'
    ftitle=' Distribution Method Comparison'
    recf=[r for r in rec if r[Nb_COL] == 2 ]
    recf=[r for r in recf if r[NpTot_COL] in range( 10000,20000+1)   ]
    groupBydraw(recf,(Nn_COL,Mtd_COL),NpTot_COL)



if group == FixedSize:
    legendpos='upper left'

    outfile=fname.strip('.txt')+'_g9_fixedsize_per_core'
    ftitle=' Fixed Size Per-Core Diagram '
    recf=[r for r in rec if r[NpPerNc_COL] in range(100,800,100) ]
    recf=[r for r in rec if r[Nc_COL] >0 ]
    groupBydraw(recf,(Nt_COL,NpPerNc2_COL),Nc_COL)
    groupBydraw(recf,(Mtd_COL,NpPerNc2_COL),Nc_COL)


if group == NodeImpact:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g10_Nodecnt_10K_'
    ftitle=' Node Count Impact Diagram'
    recf=[r for r in rec if r[Mtd_COL] == 3 ]
    groupBydraw(recf,(Nt_COL,Np_COL),Nn_COL)

#-------------------------------------------------------------------------------------------------------
if 0 and group == PureMPI:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_'
    ftitle=' Pure MPI and Hybrid Methods Comparison'
    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_Nc'
    recf=rec
    recpure = list()
    for Np in [500,1000,3360,4200,5040,8400]:
        recp = filter ( lambda x: x[NpPerNc_COL] == Np  , recf)
        if len(recp)<= 0 : continue
        for r in recp :print "+",r[NpPerNc_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
        for Nt in [1,3,5,7]:
            rect = [r for r in recp if r[Nt_COL] == Nt] 
            if len(rect)<= 0 : continue
            for r in rect :print "++",r[NpPerNc_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
            for Nn in [1,2,3,4,5,6,7,8]:
                recn = [r for r in rect if r[Nn_COL] == Nn] 
                if len(recn)<= 0 : continue
                for r in recn :print "+++",r[NpPerNc_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
                lpure = [r[T2_COL] for r in recn if r[PureMPI_COL] ==1]
                lhyb =  [r[T2_COL] for r in recn if r[PureMPI_COL] ==0]
                if len(lpure) <=0 or len(lhyb) <= 0 : continue
#                tpure = min ( lpure)
#                thyb  = min ( lhyb) 
                tpure = sum ( lpure)/len(lpure)
                thyb  = sum ( lhyb) /len(lhyb)
                               #Mtd,Nt,Nb,Np,Nn,PureMPI,T2  ,Pc,pT,mN,mT,mW,Nc   ,NpTot   ,NpPerNc   ,NpPerNc2,GFLOPS,RelT                      
                recpure.append([0  ,Nt,0 ,0 ,Nn,0      ,thyb,0 ,0 ,0 ,0 ,0 ,Nn*Nt,Np*Nt*Nn,Np        ,0       ,0     ,(thyb*1.0)/(tpure) ])



    figsz=(10,8)
    fz=24
    colors=['k','k']
    colors=['g','r','k','m','#20E0F0']
    legendpos='best'
    col_names[RelT_COL] = r"Relative Execution Time ($T_{Hybrid}/T_{MPI} $)"
    outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nc'
    ftitle=''# Pure MPI and Hybrid Methods Comparison-Core Based'
    groupBydraw(recpure,(NpPerNc_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    legendpos='upper right'
    outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nn'
    ftitle=''# Pure MPI and Hybrid Methods Comparison-Node Based'
    rechp= [r for r in recpure if r[Nn_COL] in ( 1,2,3,4) ]
    groupBydraw(recpure,(NpPerNc_COL,Nn_COL),Nc_COL,False,0,RelT_COL)
    
    for r in recpure:
        print r[Np_COL],r[Nn_COL],r[Nt_COL],r[RelT_COL]
    recc=writerecords(recpure,fnameclean)



    val=list()
    fh=open(fname.strip('.txt')+'_g7_stats.txt','w')
    for Np in range(500,25000,500):
        rpure = [r for r in rec if r[NpTot_COL] == Np and r[PureMPI_COL] == 1]
        rhyb  = [r for r in rec if r[NpTot_COL] == Np and r[PureMPI_COL] == 0]
        if len(rpure) >0 and len(rhyb) >0 :
            tpure = min ( [r[T2_COL] for r in rpure])
            thyb  = min ( [r[T2_COL] for r in rhyb ])
            ratio = thyb/tpure
            s= "Improvement for Np = "+str(Np)+' ' + str(ratio)+ '\n'
            print s
            fh.write(s)
            val.append(ratio)
    if len(val)>0:
        s=  "Average = " + str(sum(val)/len(val))+'\n'
        print s
        fh.write(s)
    fh.close()

    val=list()
    fh=open(fname.strip('.txt')+'_g7_stats.txt','a')
    for Nc in range(1,64):
        rpure = [r for r in rec if r[Nc_COL] == Nc and r[PureMPI_COL] == 1]
        rhyb  = [r for r in rec if r[Nc_COL] == Nc and r[PureMPI_COL] == 0]
        if len(rpure) >0 and len(rhyb) >0 :
            tpure = min ( [r[T2_COL] for r in rpure])
            thyb  = min ( [r[T2_COL] for r in rhyb ])
            ratio = thyb/tpure
            s= "Improvement for Nc = "+str(Nc)+' ' + str(ratio)+ '\n'
            print s
            fh.write(s)
            val.append(ratio)
    if len(val)>0:
        s=  "Average = " + str(sum(val)/len(val))+'\n'
        print s
        fh.write(s)
    fh.close()
    fig3d=plt.figure(None,figsize=(22,12)) 
    plt.grid(True)



#-------------------------------------------------------------------------------------------------------
if 1 and group == PureMPI:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_'
    ftitle=' Pure MPI and Hybrid Methods Comparison'
    recf=rec
    print outfile,fname

    outfile=fname.strip('.txt')+'_g7_PureMPI_u10K_Nc'
    recf=[r for r in rec if r[T2_COL] >0.001 ]
    recpure = list()
    for Np in [1000,3360,4200,5040,8400,8192]:
        recp = filter ( lambda x: x[NpTot_COL] == Np  , rec)
        if len(recp)<= 0 : continue
        for r in recp :print "+",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
        for Nt in [1,3,5,7]:
            rect = [r for r in recp if r[Nt_COL] in ( Nt,Nt+1)] 
            if len(rect)<= 0 : continue
            for r in rect :print "++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
            for Nn in [1,2,3,4,5,6,7,8]:
                recn = [r for r in rect if r[Nn_COL] == Nn] 
                if len(recn)<= 0 : continue
                for r in recn :print "+++",r[NpTot_COL],r[Nt_COL],r[Nn_COL],r[PureMPI_COL]
                lpure = [r for r in recn if r[PureMPI_COL] ==1 and r[Nt_COL] == Nt+1 ]
                lhyb =  [r for r in recn if r[PureMPI_COL] ==0 and r[Nt_COL] == Nt   ]
                if len(lpure) <=0 or len(lhyb) <= 0 : 
                    print "?"*40
                    continue
                tpure = sum ( [r[T2_COL] for r in lpure])/len(lpure)
                thyb  = sum ( [r[T2_COL] for r in lhyb ])/len(lhyb)
                wpure = sum ( [r[CommW_COL] for r in lpure])/len(lpure)
                cpure = sum ( [r[CommN_COL] for r in lpure])/len(lpure)
                whyb  = sum ( [r[CommW_COL] for r in lhyb]) /len(lhyb)
                chyb  = sum ( [r[CommN_COL] for r in lhyb])/len(lhyb)
                print "="*20
                print tpure,wpure,cpure
                print thyb,whyb,chyb
                mW,mN=1,1
                if whyb != 0 :
                    if wpure != 0 :
                        mW=wpure/whyb
                    #thyb /= whyb
                    #tpure /= wpure
                xpure=Nn*Nt*(Nn*Nt -  Nt+1)
                xhyb=Nn*Nn
                if chyb != 0 :
                    mN=chyb*1.0/cpure
                mN = xhyb*1.0/xpure
                #mW = 1.0/(Nt+1)
                
                print mW,mN
                print "="*20
                relT=(thyb/tpure)
                mW = mN
                               #Mtd,Nt  ,Nb,Np,Nn,PureMPI,T2  ,Pc,pT,mN,CommT ,mW,Nc   ,NpTot   ,NpPerNc   ,NpPerNc2,GFLOPS,RelT                      
                recpure.append([0  ,Nt+1,0 ,0 ,Nn,0      ,thyb,0 ,0 ,mN,tpure ,mW,Nn*Nt,Np      ,Np/(Nn*Nt),0       ,0     ,relT])



    figsz=(12,8)
    fz=24
    colors=['k','k']
    colors=['g','r','k','m','#20E0F0']
    legendpos='upper right'
    col_names[RelT_COL] = r"Relative Execution Time($\frac{T_{Hybrid}}{T_{MPI}} $)"
    outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nc'
    print outfile,fname
    ftitle=''
    groupBydraw(recpure,(NpTot_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

    recnode= [r for r in recpure if r[Nn_COL] in ( 1,2,4,6,8)]
    outfile=fname.strip('.txt')+'Article_g7_PureMPI_Nn'
    ftitle=''
    groupBydraw(recnode,(NpTot_COL,Nn_COL),Nc_COL,False,0,RelT_COL)


    if 0 :
        for r in recnode:
            print r[NpTot_COL],r[Nn_COL],r[Nt_COL],r[RelT_COL]
    recc=writerecords(recpure,fnameclean)

#-------------------------------------------------------------------------------------------------------

if group == SpeedUp:
    lglist=list()
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g6_speedup_u10K_'
    ftitle=''#' Speed Up Diagram'
    recf=[r for r in rec if r[Mtd_COL] == 3 and r[Nc_COL] != 64]
    recc= list()
    for np in [3360,4200,5040,6720,7560,8400]:
        temp=[r[T2_COL] for r in recf if r[NpTot_COL] == np and r[Nc_COL] == 1 and r[Nb_COL] == 1 ]
        if len (temp ) ==0 : 
            print "No data for Np=",np
            continue
        ts = min(temp )
        rect = [[r[0] , r[1] , r[2] , r[3] , r[4] , r[5] , \
                     r[6] , r[7] , r[8] , r[9],r[10],r[11],r[12],ts/r[T2_COL] ]for r in recf if r[NpTot_COL] == np ] #and r[Nt_COL] in ( 1,3,5,7)]
        for r in rect:
#            print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[NpPerNc_COL],r[RelT_COL]
            recc.append(r)
    col_names[RelT_COL]='Speedup'
    figsz=(10,8)
    fz=18
    colors=['k','k']
    colors=['g','r','k','m','#20E0F0']
    outfile=outfile+'_art'
    groupBydraw(recc,(NpTot_COL,Mtd_COL),Nc_COL,True,0,RelT_COL)
    recf=writerecords(recc,fnameclean)
#-------------------------------------------------------------------------------------------------------

def round_to(a,b):
    print a,b,'-',a-b,0.01*b
    if abs(a-b) <0.01*b:
        return a
    return a
if group == WeakScaling:
    
    legendpos='best'
    ftitle='' #Constant Work Load [ 1 million elements/core]'# Weak Scaling Diagram'
    outfile=fname.strip('.txt')+'_g11_WeakScaleUp_'
    plist=[1,3,5,7]
    for np in [1000]: 
        recf=[r for r in rec if  r[Nt_COL] in plist]
        if len(recf)>0:
            Ts =min([r[T2_COL]       for r in rec if r[Nc_COL]  == 1])
            Ns =min([r[Np_COL]    for r in rec if r[Nc_COL]  == 1])
            Ws=Ns*Ns
            recc=list()
            rect = [[r[0] , r[1] , r[2] , r[3] , r[4] , r[5] , \
                         r[6] , r[7] , r[8] , r[9],r[10],r[11],r[12],\
                         r[13] , r[14] , r[15] , r[16],\
                          Ts/r[T2_COL]   ] for r in recf if r[Nt_COL] in plist]
#                         round_to ( (r[Np_COL]**2*r[Nn_COL]*1.0)*(Ts/r[T2_COL])/(Ws*r[Nt_COL]),1)   ] for r in recf if r[Nt_COL] in plist]
#                         (1)*((Ws*1.0)/(r[NpTot_COL]**2*1.0))*(r[6]/Ts )   ] for r in recf if r[Nt_COL] in ( 1,3,5,7)]
#                         (r[6]*np*np/(Ts*(r[NpTot_COL]**2.) ))   ] for r in recf if r[Nc_COL] != 1 and r[Nt_COL] in ( 1,2,4,8)]
            for r in rect:
                recc.append(r)
            recc=writerecords(recc,fnameclean+str(np))
            figsz=(12,8)
            fz=24
            colors=['k','k']
            colors=['g','r','k','m','#20E0F0']
#            col_names[RelT_COL]=r"Relative Execution Time/core ($\frac{T_1/Work_1}{c.T_p/Work_p}$)"
#            col_names[RelT_COL]=r"Relative Execution Time/core ($\frac{T_1}{T_p}$)"
            col_names[RelT_COL]=r"Relative Speed ($\frac{T_1}{T_p}$)"
            outfile=outfile+'_art'
            plt.ylim(0,8)
            groupBydraw(recc,(Mtd_COL,Nt_COL),Nn_COL,False,0,RelT_COL)
            legendpos='best'
            outfile=outfile+'_Nc'
            plt.ylim(0,8)
            groupBydraw(recc,(Mtd_COL,Nt_COL),Nc_COL,False,0,RelT_COL)

#-------------------------------------------------------------------------------------------------------
if group == ScaleUp:
    legendpos='upper left'
    outfile=fname.strip('.txt')+'_g12_ScaleUp_'
    ftitle=''# Strong ScaleUp Diagram'
    recf=rec #([r for r in rec if r[Nc_COL]  != 1] )
    recz=([r[NpTot_COL] for r in rec if r[Nc_COL]  != 1])
    npm = min (recz)
    print npm
    for np in [ 3360,4200,5040,6720,7560,8400 ]:
        l = [r[T2_COL] for r in rec if r[Nc_COL]  == 1 and r[NpTot_COL] == np]
        print len(l),np,len(rec)
        if len(l)<=0: continue
        Ts=min(l)
        gT1=(np**2)/1e6/Ts
        recc=list()
        rect = [[r[0] , r[1] , r[2] , r[3] , r[4] , r[5] , \
                     r[6]  , r[7]  , r[8]  , r[9] ,r[10],r[11],r[12], \
                     r[13] , r[14] , r[15] , r[16],\
                     (r[NpTot_COL]**2)/1e6/r[T2_COL] ]for r in recf if r[NpTot_COL] == np and r[Nt_COL] in ( 1,3,5,7)]
        for r in rect:
#            print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[NpPerNc_COL],r[RelT_COL]
            recc.append(r)
        print "T 1x1",Ts
        recc=writerecords(recc,fnameclean+str(np))
        col_names[RelT_COL]=r"Speed [million elements/s]"
        figsz=(10,8)
        fz=24
        colors=['#F20624','#06F2AF','#05AA50','#2020F0','#20E0FF']
        colors=['g','r','k','m','#20E0F0']
        outfile=outfile+'_art'
        plt.ylim(0,2)
        groupBydraw(recc,(NpTot_COL,Nt_COL),Nn_COL,True,0,RelT_COL)
        outfile=outfile.strip('_art')


#-------------------------------------------------------------------------------------------------------

if group == CholSpeedUp:
    lglist=list()
    legendpos='upper left'
    ftitle='No Cache Optimization'#' Speed Up Diagram'
    recf=[r for r in rec if  r[T2_COL]>0.000]
    recc= list()
    for np in [512,1024,2048,4096,8192] : # [512,1024,2048,4096,8192]:
        for nb in [8,16 ,32 ,64,128,256,512,1024,2048]:
            temp=[r[T2_COL] for r in recf if r[NpTot_COL] == np and r[Nc_COL] == 1 and r[Seq_COL] == 2 and r[Nb_COL] == nb]
            if len (temp ) ==0 : 
                print "No data for Np=",np,"Nb=",nb
                continue
            ts = min(temp )
            for r in recf:
                if r[NpTot_COL] == np and r[Nc_COL] == 1 and r[Seq_COL] != 0 and r[Nb_COL] == nb:
                    print r
            for r in recf:
                if r[NpTot_COL] == np and r[Seq_COL] == 0 and r[Nb_COL] == nb: 
                    print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[RelT_COL]

            print ts,len(recf[0])
            rect = [[r[0] , r[1] , np/nb , r[3] , r[4] , r[5] , \
                     r[6] , r[7] , r[8] , r[9],r[10],r[11],r[12], \
                     r[13] , r[14] , r[15] , r[16],r[17],\
                     ts/r[T2_COL] ]for r in recf if r[NpTot_COL] == np and r[CacheOpt_COL] == 0 and r[Nb_COL] == nb]
            for r in rect:            
                print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[RelT_COL]
                recc.append(r)

    col_names[RelT_COL]='Speedup'
    col_names[Nb_COL]='Block Size'
    figsz=(10,8)
    fz=18
    colors=['k','k']
    colors=['g','r','k','m','#20E0F0']
    ofile=fname.strip('.txt')+'_chol_speedup_'
    outfile=ofile+'_Core'
    groupBydraw(recc,(NpTot_COL,Nb_COL), Nc_COL,True,0,RelT_COL)
    reca=writerecords(recc,fnameclean)



    ftitle='With Cache Optimization'#' Speed Up Diagram'
    recc= list()
    for np in [512,1024,2048,4096,8192] : # [512,1024,2048,4096,8192]:
        for nb in [8,16 ,32 ,64,128,256,512,1024,2048]:
            temp=[r[T2_COL] for r in recf if r[NpTot_COL] == np and r[Nc_COL] == 1 and r[Seq_COL] == 3 and r[Nb_COL] == nb]
            if len (temp ) ==0 : 
                print "No data for Np=",np,"Nb=",nb
                continue
            ts = min(temp )
            for r in recf:
                if r[NpTot_COL] == np and r[Nc_COL] == 1 and r[Seq_COL] != 0 and r[Nb_COL] == nb:
                    print r
            for r in recf:
                if r[NpTot_COL] == np and r[Seq_COL] == 0 and r[Nb_COL] == nb: 
                    print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[RelT_COL]

            print ts,len(recf[0])
            rect = [[r[0] , r[1] , np/nb , r[3] , r[4] , r[5] , \
                     r[6] , r[7] , r[8] , r[9],r[10],r[11],r[12], \
                     r[13] , r[14] , r[15] , r[16],r[17],\
                     ts/r[T2_COL] ]for r in recf if r[NpTot_COL] == np and r[CacheOpt_COL] == 1 and r[Nb_COL] == nb]
            for r in rect:            
                print r[NpTot_COL],r[Nc_COL],r[T2_COL],r[Nt_COL],r[Nn_COL],r[RelT_COL]
                recc.append(r)

    col_names[RelT_COL]='Speedup'
    col_names[Nb_COL]='Block Size'
    figsz=(10,8)
    fz=18
    colors=['k','k']
    colors=['g','r','k','m','#20E0F0']
    ofile=fname.strip('.txt')+'_chol_speedup_cacheopt'
    outfile=ofile+'_Core'
    groupBydraw(recc,(NpTot_COL,Nb_COL), Nc_COL,True,0,RelT_COL)
    reca=writerecords(recc,fnameclean)



    legendpos='upper left'
    rect= list()
    rect = [[r[Seq_COL] , r[1] , r[NpTot_COL]/r[Nb_COL] , r[3] , r[4] , r[5] , \
                 r[6] , r[7] , r[8] , r[9],r[10],r[11],r[12], \
                 r[13] , r[14] , r[15] , r[16],r[17],r[18] ]for r in recf if r[Seq_COL] in(2,3)]


    for r in rect:
        print r[NpTot_COL], r[Mtd_COL], r[Nb_COL], r[T2_COL]
    ofile=fname.strip('.txt')+'_chol_exp'
    outfile=ofile
    mtd_names=dict(zip([3,2,1,0],
                   ['CacheOpt','Block','Element','NotSeq']))
    groupBydraw(rect,(NpTot_COL,Mtd_COL), Nb_COL,False,0,T2_COL)
