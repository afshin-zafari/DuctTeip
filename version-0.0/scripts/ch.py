#!/usr/bin/env python

import sys
#import pylab 
import math 

# Record structure
# R: T(type,r,c,h,w) d0(type,r,c,h,w,v,c1) d1(type,r,c,h,w,v.c1) d2(type,r,c,h,w,v,c1) glob_v  cw0 cw1 cw2
TASK,D0,D1,D2,GV,CW0,CW1,CW2=range(0,8)
TYPE,ROW,COL,HOST,WGHT,VER,DYRC=range(0,7)
DX,DM,DY = 0,1,2
XAdd,MAdd,Diag,MMul,PnlU = 1,2,3,4,5

# stat rec
# N P Part GV T# T% TW TWP CW CW%
NCOL,PCOL,P1COL,GVCOL,TCNT,TPER,TWGT,TWPER=range(0,8)
gtask_list=list()
dax_list = list()
block_col = 0

def colupd(n,j,br,bc):
    if j>=n: return
    if br == 1 : 
        print "ColUpd:A[",j+1,'-',n,"]",j
        return
    print "ColUpd:B[",(j+1)/br+1,'-',n/br+1,"]",j
             
             
def chol(n,br,bc):
    print "-------------------Element Tasks---------------"
    for j in range(1,n+1):
        istr = "["+str(j)+'-'+str(n)+"]"
        if j > 1:
            kstr = "[1-"+str(j-1)+"]"
            print "PanelUpd",istr,j,"-=A"+istr+kstr,"*A"+str(j)+kstr
        colupd(n,j,br,bc)
            
            
def gen_tasks(n,br,bc):
    print "-------------------Block Tasks---------------"
    for j in range(1,(n+1)/bc+1):
        istr = "["+str(j)+'-'+str(n/br)+"]"
        if j > 1:
            kstr = "[1-"+str(j-1)+"]"
            print "PanelUpd",istr,j,"-=B"+istr+kstr,"*B"+str(j)+kstr
        colupd(n,j,br,bc)

def gen_tasks_proc(n,pr):
    p = int(math.sqrt(pr))
    bc =br = int (n / p)
    print "-------------------Processor Tasks---------------",br,bc
    for j in range(1,(n+1)/bc+1):
        for i in range( j, n/br+1):
            istr = str(i)
            if j > 1:
                for k in range ( 1,j ):
                    kstr = str(k)
                    print "P:",istr,"PanelUpd:B"+istr+str(j),"-=B"+istr+kstr,"*B"+str(j)+kstr
            else:
                kstr="1"
                print "P:",istr,"PanelUpd:B"+istr+str(j),"-=B"+istr+kstr,"*B"+str(j)+kstr
        colupd(n,j,br,bc)
    
def versioning():
    global gtask_list,dax_list
    dbg=0
    dl=list()
    for r in dax_list:
        dl.append(r[2])
        dl.append(r[4])
        dl.append(r[6])
    ddict= dict(zip (dl,range(len(dl))))                
    ver=[0 for r in range(max(ddict.values()) +1)]
    if dbg:print len ( ver),len(ddict),len(dl)
    for r in dax_list:
        if dbg:print r[2],r[4],r[6]
        if dbg:print ddict[r[2]],ddict[r[4]],ddict[r[6]]
        r[3]= ver[ ddict[r[2]] ]
        if ddict[r[4]]<len(ver):
            r[5]= ver[ ddict[r[4]] ]
        ver[ ddict[r[6]] ]= ver[ ddict[r[6]] ]+1
        r[7]= ver[ ddict[r[6]] ]
    for i in range(0,len(dax_list)):
        gtask_list[i][D0][VER]=dax_list[i][3]
        gtask_list[i][D1][VER]=dax_list[i][5]
        gtask_list[i][D2][VER]=dax_list[i][7]
        gtask_list[i][GV]=max ( dax_list[i][3] , dax_list[i][5])
        if dbg:
            print  gtask_list[i][D0][VER],  gtask_list[i][D1][VER], gtask_list[i][D2][VER],  gtask_list[i][GV]

def add_task(s0):
    global dax_list
    dbg=0
    if dbg:print s0
    s1=s0.split('\n')
    for s in s1:
        el=s.split('  ')
        if len(el)>4:
            tsk=el[0]
            r = int(tsk[2:4])
            c = int(tsk[5:7])
            dax_list.append ([el[0],el[1],el[2],0,el[3],0,el[4],0,int(el[5]),r,c,-1] ) 
    return s0

def create_task(t,host,r,c):
    global gtask_list
    dbg=0
    # R: T(type,r,c,h,w) d0(type,r,c,h,w,v,c2) d1(type,r,c,h,w,v,c2) d2(type,r,c,h,w,v,c2) glob_v  cw0 cw1 cw2
    s="T({0:02},{1:02}):  ".format(r,c)
    if t == "Diag":
        s1=""
        #for c1 in range(c-1,-1,-1):
        #    gtask_list.append([ [MAdd,r,c,host,4]  , [DX,r,c1,-1,-1,0,-1], [DM,r,r,-1,-1,0,-1], [DM,r,r,-1,-1,0,-1], -1,-1,-1,-1] )
        #    s1+=s+"MAdd@{1}  X({2},{3})  M({2},{2})  M({2},{2})  4".format ( t,host,r,c1)+'\n'
            #print r,c,c1,'\n',s1
        gtask_list.append([ [Diag,r,c,host,3]  , [DM,r,c,-1,-1,0,-1], [DM,r,r,-1,-1,0,-1], [DM,r,r,-1,-1,0,-1], -1,-1,-1,-1] )
        s1+= s+"{0}@{1}  M({2},{3})  M({2},{2})  M({2},{2})  3".format ( t,host,r,c)+'\n'
        if dbg:print s1
        return s1
    if t == "MAdd":
        s1=""
        gtask_list.append([ [MAdd,r,c,host,2]  , [DX,r,c,-1,-1,0,-1], [DM,r,c,-1,-1,0,-1], [DM,r,c,-1,-1,0,-1], -1,-1,-1,-1] )
        s1+= s+"{0}@{1}  X({2},{3})  M({2},{4})  M({2},{4})  2".format ( t,host,r,c,c)+'\n'        
        if dbg:print s1
        return s1
    if t == "PnlU":
        gtask_list.append([ [PnlU,r,c,host,5]  , [DM,c,c,-1,-1,0,-1], [DM,r,c,-1,-1,0,-1], [DM,r,c,-1,-1,0,-1], -1,-1,-1,-1] )
        s1= s+"{0}@{1}  M({3},{3})  M({4},{3})  M({4},{3})  5".format ( t,host,r-1,c,r,r+1)+'\n'
        if dbg:print s1
        return s1
    if t == "MMul":
        s1=""
        r1=c+1
        gtask_list.append([ [MMul,r,c,host,4]  , [DM,r,c,-1,-1,0,-1], [DM,r1,c,-1,-1,0,-1], [DY,r,c,-1,-1,0,r1], -1,-1,-1,-1] )
        s1+= s+"{0}@{1}  M({2},{3})  M({4},{3})  X({2},{3},{4},{3})  4".format ( t,host,r,c,r1)+'\n'            
        for c1 in range(c,r+1):
            gtask_list.append([ [XAdd,r,c,host,1]  , [DY,r,c,-1,-1,0,r1], [-1,r,c,-1,-1,-1,-1], [DX,r,c1,-1,-1,0,-1], -1,-1,-1,-1] )
            s1+= s+"X -=@{1}  X({2},{3},{4},{3})  --  X({2},{5})  1".format ( t,host,r,c,r1,c1)+'\n'
        if dbg:print s1
        return s1

    
def gen_gtasks(n,part):
    for c in range(0,part):
        for r in  range(c,part):
            if c>0:
                add_task(create_task("MAdd",0,r ,c))                    
            if r>0:
                if r != c:
                    add_task(create_task("PnlU",0,r,c))
                    add_task(create_task("MMul",0,r,c))
            if r == c:
                add_task(create_task("Diag",0,r,r))
            



def print_task(task,lbl=''):

    tn = dict({XAdd: 'XAdd',MAdd:'MAdd',Diag:'Diag',MMul:'MMul',PnlU:'PnlU'})
    dn = dict({DM: 'M',DX:'X',DY:'X',-1:'--'})
    tstr="{0}({1},{2})@{3}:".format(tn[task[TASK][TYPE]],task[TASK][ROW],task[TASK][COL],task[TASK][HOST])
    if task[D0][TYPE] ==DY:
        d0str="X({0},{1},{2},{1}){4}@{3}".format(task[D0][ROW],task[D0][COL],task[D0][DYRC],task[D0][HOST],task[D0][VER])
    else:
        d0str="{0}({1},{2}){4}@{3}".format(dn[task[D0][TYPE]],task[D0][ROW],task[D0][COL],task[D0][HOST],task[D0][VER])
    if task[D1][TYPE] <0:
        d1str=8*'-'
    else:
        if task[D1][TYPE] ==DY:
            d1str="X({0},{1},{2},{1}){4}@{3}".format(task[D1][ROW],task[D1][COL],task[D1][DYRC],task[D1][HOST],task[D1][VER])
        else:
            d1str="{0}({1},{2}){4}@{3}".format(dn[task[D1][TYPE]],task[D1][ROW],task[D1][COL],task[D1][HOST],task[D1][VER])
    if task[D2][TYPE] ==DY:
        d2str="X({0},{1},{2},{1}){4}@{3}".format(task[D2][ROW],task[D2][COL],task[D2][DYRC],task[D2][HOST],task[D2][VER])
    else:
        d2str="{0}({1},{2}){4}@{3}".format(dn[task[D2][TYPE]],task[D2][ROW],task[D2][COL],task[D2][HOST],task[D2][VER])
    print tstr +'\t'+d0str +'\t'+d1str +'\t'+d2str +'\t' +str(task[CW0]) +'\t'+str(task[CW1]) +'\t'+str(task[CW2]) +'\t'+lbl

def write_task(task,fn):
    #Data: Name, version, Host , nRow, nCol
    #Axs: d_name , acc_type
    #Task: Name , Host , Axs
    
    tn = dict({XAdd: 'XAdd',MAdd:'MAdd',Diag:'Diag',MMul:'MMul',PnlU:'PnlU'})
    dn = dict({DM: 'M',DX:'X',DY:'X',-1:'--'})

    tname="{0:13s}".format(tn[task[TASK][TYPE]])
    thost="{0}".format(task[TASK][HOST])

    d0name="{0}_{1:02d}_{2:02d}_{3:02d}_{4:02d}".format(dn[task[D0][TYPE]],task[D0][ROW],task[D0][COL],task[D0][DYRC],task[D0][VER])
    d0host="{0}".format(task[D0][HOST])

    if task[D1][TYPE] <0:
        d1name =''
        d1host= -1
    else:
        d1name="{0}_{1:02d}_{2:02d}_{3:02d}_{4:02d}".format(dn[task[D1][TYPE]],task[D1][ROW],task[D1][COL],task[D1][DYRC],task[D1][VER])
        d1host="{0}".format(task[D1][HOST])

    d2name="{0}_{1:02d}_{2:02d}_{3:02d}_{4:02d}".format(dn[task[D2][TYPE]],task[D2][ROW],task[D2][COL],task[D2][DYRC],task[D2][VER])
    d2host="{0}".format(task[D2][HOST])
    
    # d0
    # d1
    # d2
    fn.write( "D {0} {1}\n".format( d0name ,d0host))
    if len(d1name)>0:
        fn.write( "D {0} {1}\n".format( d1name ,d1host))
    fn.write( "D {0} {1}\n".format( d2name ,d2host))
    # Axs 1
    # Axs 2
    # Axs 3
    READ , WRITE = 1,10
    fn.write( "A {0} {1}\n".format( d0name ,READ))
    if len(d1name)>0:
        fn.write( "A {0} {1}\n".format( d1name ,READ))
    fn.write( "A {0} {1}\n".format( d2name ,WRITE))
    # Task
    fn.write( "T {0} {1}\n".format( tname, thost))

def export_tasks(tasks,fname,p):
    fn = open (fname,'w')
    for t in tasks:
        write_task(t,fn)
    for r in range(0,p):
        for c in range(0,r+1):
            fn.write("I M_{0:02d}_{1:02d}_-1_00 {0}\n".format(r,c))
    fn.close()




def print_stat(st,caption):
    print "\n"+15*'-',caption,(65-len(caption))*'-'
    print "Pnt#\tPr#\tPart#\tRow\tCol\tT#\tT%\tTW\tTW%\tCW\tCW%"
    print "----\t---\t-----\t---\t---\t--\t--\t--\t---\t--\t---"
    for s in st:
        for i in range(0,len(s)):
            print s[i],'\t',
        print

def calc_stat(rec,n,p,part):
    dbg=0
    stat=list()
    totw=sum ( [r[TASK][WGHT] for r in rec] )
    if dbg:print len(rec),totw
    for v in range(0,40):
        rv = [r for r in rec if r[GV]==v]
        tcnt=len(rv)
        if tcnt==0:break
        tws = sum ( [r[TASK][WGHT] for r in rv] )
        tcp = 100*float(tcnt)/len(rec)
        twp = 100*float(tws)/totw
        stat.append([n,p,part,v,tcnt,int(tcp*100)/100.0,tws,int(twp*100)/100.0])
    return stat

            
def calc_stat_part(rec,n,p,part,base=COL):
    dbg=0
    stat = list()
    recf=[r for r in rec if r[GV] in ( 0,1,2) ]
    vtcnt=len(recf)
    vtws=sum (r[TASK][WGHT] for r in recf) 
    if dbg:print "VerT#,VerTWSum", vtcnt, vtws
    for c in range(0,part):
        rect = [rt for rt in recf if rt[TASK][base] == c  ]
        tcnt = len(rect)
        if tcnt==0:continue
        tws = sum (rr[TASK][WGHT] for rr in rect) 
        tcp = 100*float(tcnt)/vtcnt
        twp = 100*float(tws)/vtws
        stat.append ( [n,p,part,c,0,tcnt,int(tcp*100)/100.0,tws,int(twp*100)/100.0] )
    return stat

def calc_stat_part_host(rec,n,p,part,v):
    dbg=0
    stat = list()
    recf=[r for r in rec if r[GV] in v ]
    vtcnt=len(recf)
    vtws=sum (r[TASK][WGHT] for r in recf) 
    if dbg:print "VerT#,VerTWSum", vtcnt, vtws
    for h in range(0,p):
        rect = [rt for rt in recf if rt[TASK][HOST] == h  ]
        tcnt = len(rect)
        if tcnt==0:continue
        tws = sum (rr[TASK][WGHT] for rr in rect) 
        tcp = 100*float(tcnt)/vtcnt
        twp = 100*float(tws)/vtws
        stat.append ( [n,p,part,h,0,tcnt,int(tcp*100)/100.0,tws,int(twp*100)/100.0] )
    return stat

def calc_stat_comm_cost(rec,n,p,part,v):
    dbg=0
    stat = list()
    recf=[r for r in rec if r[GV] in v ]
    vtcnt=len(recf)
    vtws=sum (r[TASK][WGHT] for r in recf) 
    vcws=sum (r[CW0]+r[CW1]+r[CW2] for r in recf) 
    vcwavg=100.0/p
    err1,err2=0,0
    if dbg:print "VerT#,VerTWSum", vtcnt, vtws
    for h in range(0,p):
        rect = [rt for rt in recf if rt[TASK][HOST] == h  ]
        tcnt = len(rect)
        if tcnt==0:continue
        tws = sum (rr[TASK][WGHT] for rr in rect) 
        tcp = 100*float(tcnt)/vtcnt
        twp = 100*float(tws)/vtws
        cws = sum (rr[CW0]+rr[CW1]+rr[CW2] for rr in rect) 
        cwp = 0
        if vcws != 0 :
            cwp = 100*float(cws)/vcws
        err1+= (twp-vcwavg)**2
        err2+= (cwp-vcwavg)**2
        if dbg: print "CommCost@",h,'=',cws,cwp
        stat.append ( [n,p,part,h,0,tcnt,int(tcp*100)/100.0,tws,int(twp*100)/100.0,cws,int(cwp*100)/100.0  ] )
    stat.append ( ['-','-','-','-','-',vtcnt,'-',vtws,'{0:2.2f}'.format(math.sqrt(err1/p)) ,vcws,'SD={0:2.2f}'.format(math.sqrt(err2/p))  ] )
    return stat
    
    
def comm_add(rec, dt,dr,dc,dh,amnt):
    for r in rec:
        if (r[D2][TYPE],r[D2][ROW],r[D2][COL],r[D2][HOST]) == (dt,dr,dc,dh):
            r[CW2] +=amnt
    return rec

def comm_cost(rec,n,p,part):
    for r in rec:
        r[CW0],r[CW1],r[CW2]=0,0,0        
    for r in rec:
        if r[TASK][HOST] !=r[D0][HOST]: 
            r[CW0]=(r[D0][TYPE]!=-1)*(n/part)*(n/part)/1000
            rec=comm_add(rec,r[D0][TYPE],r[D0][ROW],r[D0][COL],r[D0][HOST],r[CW0])
        if r[TASK][HOST] !=r[D1][HOST]: 
            if (r[D0][TYPE],r[D0][ROW],r[D0][COL],r[D0][HOST],r[D0][VER]) != (r[D1][TYPE],r[D1][ROW],r[D1][COL],r[D1][HOST],r[D1][VER]):
                r[CW1]=(r[D1][TYPE]!=-1)*(n/part)*(n/part)/1000
                rec=comm_add(rec,r[D1][TYPE],r[D1][ROW],r[D1][COL],r[D1][HOST],r[CW1])
        

def alloc_tasks_para(rec,n,p,part,v):
    dbg=0
    h=0
    c=0
    for r in rec:
        if r[TASK][TYPE] == PnlU :
            c += 1 
            if c>=(part/p):
                print "c,p,part,h",c,p,part,h
                h =(h+1) % p
                c=0
        r[TASK][HOST]=h 
        print_task(r)        

    for r in rec:
        r[D2][HOST] = r[TASK][HOST] 
        rec = change_receivers_host(rec,r[D2][TYPE],r[D2][ROW],r[D2][COL],r[D2][DYRC],r[D2][VER],r[TASK][HOST])

    h = part / p 
    for r in rec:
        if r[D0][HOST] == -1 :
            if dbg:print_task(r)
            r[D0][HOST] = r[D0][ROW] / h
            if dbg:print_task(r);print
        if r[D1][TYPE] >= 0 :
            if r[D1][HOST] == -1 :
                if dbg:print_task(r)
                r[D1][HOST] = r[D1][ROW] / h 
                if dbg:print_task(r);print
            


    return rec



def alloc_tasks_seq(rec,n,p,part,v):
    for r in rec:
        r[TASK][HOST]=r[TASK][ROW]
        r[D0][HOST]=r[D0][ROW]
        r[D1][HOST]=r[D1][ROW]
        r[D2][HOST]=r[D2][ROW]
        
def get_max_extra(stat,avg):
    mx =0
    idx=-1
    for s in stat:
        if s[7]-avg>mx:
            mx=s[7]-avg
            idx= s[3]
    return mx,idx

def get_max_slack(stat,avg):
    mx =0
    idx=-1
    for s in stat:
        if avg-s[7]>mx:
            mx=avg-s[7]
            idx= s[3]
    return mx,idx

def get_tw(rec,v):
    return sum ([r[TASK][WGHT] for r in rec if r[GV] in v] ) 


def change_receivers_host(rec, dt,dr,dc,dc2,dv,newhost):
    dbg=0
    if dbg:print "NewHost", dt,dr,dc,dc2,'->',newhost
    for r in rec:
        if (r[D0][TYPE],r[D0][ROW],r[D0][COL],r[D0][DYRC],r[D0][VER]) == (dt,dr,dc,dc2,dv):
            if dbg:print_task(r)
            r[D0][HOST] = newhost
            if dbg:print_task(r)
        if (r[D1][TYPE],r[D1][ROW],r[D1][COL],r[D1][DYRC],r[D1][VER]) == (dt,dr,dc,dc2,dv):
            if dbg:print_task(r)
            r[D1][HOST] = newhost
            if dbg:print_task(r)
    return rec

def balance (rec,sidx,samnt,didx,damnt):
    dbg=0
    if dbg:print "Balance ....",sidx,samnt
    ts=0
    for row in range(0,len(rec)):          
        if damnt <= 0 or samnt <= 0 : break
        if rec[row][TASK][HOST] == sidx:
            if rec[row][D1][TYPE]<0:continue
            rec[row][TASK][HOST] =didx             
            rec[row][D2][HOST]=didx
            if dbg:
                print "Task Host changed"
                print_task(rec[row])
            rec = change_receivers_host(rec,rec[row][D2][TYPE],rec[row][D2][ROW],rec[row][D2][COL],rec[row][D2][DYRC],rec[row][D2][VER],didx)
            samnt -=rec[row][TASK][WGHT] 
            damnt -=rec[row][TASK][WGHT] 
            ts+=rec[row][TASK][WGHT] 
            if dbg:print "Row.host",rec[row][TASK][HOST],samnt,damnt,ts
    return rec
    
def alloc_tasks_weighted(rec,n,p,part,v):
    dbg=0
    avgo=(get_tw(rec,v)/p)
    if dbg:print "avg_tw",avgo
    damnt,samnt=0,5
    for c in range(0,2*p):
        avg = avgo #* ( 1.7- c*0.3)
        st= calc_stat_part_host(rec,n,p,part,v)
        if dbg:print_stat(st,"balancing...")
        samnt,sup=get_max_extra(st,avg)
        damnt,dem=get_max_slack(st,avg)
        if dbg:print ">SupDem:",sup,samnt,dem, damnt
        if abs(damnt)<float(avg)*0.01: break
        if abs(samnt)<float(avg)*0.01: break
        balance(rec,sup,samnt,dem,damnt)
    st= calc_stat_part_host(rec,n,p,part,v)
    if dbg:print "Final Dist"
    avg=get_tw(rec,v)/p
    if dbg:print "avg_tw",avg
    if dbg:print_stat(st,"... balanced")
    return rec

def balance_comm(rec,n,part):
    dbg,dbg2=0,0
    for row in range(0,len(rec)):
        fr=rec[row][D2][HOST]
        s=0
        if dbg: 
            print "--Broadcast for :"
            print_task(rec[row])
            print "--Broadcast for row."
        for r in rec:
            if (r[D0][TYPE],r[D0][ROW],r[D0][COL],r[D0][DYRC],r[D0][HOST],r[D0][VER] ) == \
               (rec[row][D2][TYPE],rec[row][D2][ROW],rec[row][D2][COL],rec[row][D2][DYRC],rec[row][D2][HOST],rec[row][D2][VER]) :
                if r[D0][HOST]!=fr: 
                    if dbg: print_task(r,'current '+str(fr))
                    r[D0][HOST]=fr
                    amnt=(r[D0][TYPE]!=-1)*(n/part)*(n/part)/1000
                    rec[row][CW2] -= amnt
                    r[CW2] +=amnt 
                    s+=amnt
                    if dbg: print_task(r,'next'+str(amnt)+'\n')
                fr = r[TASK][HOST]
            if (r[D1][TYPE],r[D1][ROW],r[D1][COL],r[D1][DYRC],r[D1][HOST],r[D1][VER] ) == \
               (rec[row][D2][TYPE],rec[row][D2][ROW],rec[row][D2][COL],rec[row][D2][DYRC],rec[row][D2][HOST],rec[row][D2][VER]) :
                if r[D1][HOST]!=fr: 
                    if dbg: print_task(r,'1current '+str(fr))
                    r[D1][HOST]=fr
                    amnt=(r[D1][TYPE]!=-1)*(n/part)*(n/part)/1000
                    rec[row][CW2] -= amnt
                    r[CW2] +=amnt 
                    s+=amnt
                    if dbg: print_task(r,'1next'+str(amnt)+'\n')
                fr = r[TASK][HOST]
        if dbg: 
            print "--After Balanced :"
            print_task(rec[row],str(fr)+'\n')
        if dbg2:
            if s: 
                print "--rec updated  :"
                print_task(rec[row],str(s)+'\n')
    return rec

def experiment(print_stat_all,print_stat_part,print_comm_cost,print_comm_bal,print_final,comp_load_balance,comm_load_balance,n,p,part):
    global gtask_list,dax_list    
    v = range(0,10)
    for i in [p]:
        #p = part
        print "Point#,Proc#,Part#", n,p,part
        gtask_list=list()
        dax_list=list()

        gen_gtasks(n,part)
        versioning()

        st= calc_stat(gtask_list,n,p,part)
        if print_stat_all:print_stat(st,"All Statistics")

        sp = calc_stat_part(gtask_list,n,p,part)
        if print_stat_part:print_stat(sp,"Partitioned Statistics")

        if part/p == 1 :
            alloc_tasks_seq(gtask_list,n,p,part,v)
        else:
            alloc_tasks_para(gtask_list,n,p,part,v)
        comm_cost(gtask_list,n,p,part)

        st = calc_stat_comm_cost(gtask_list,n,p,part,v)
        if print_comm_cost:print_stat(st,"Sequential Dist. Statistics")

        if comp_load_balance:
            gtask_list=alloc_tasks_weighted(gtask_list,n,p,part,v)
            comm_cost(gtask_list,n,p,part)

            st = calc_stat_comm_cost(gtask_list,n,p,part,v)
            if print_comm_cost:print_stat(st,"Weighted Dist. Statistics")

        if comm_load_balance:
            gtask_list=balance_comm(gtask_list,n,part)
            comm_cost(gtask_list,n,p,part)
            st = calc_stat_comm_cost(gtask_list,n,p,part,v)
            if print_comm_bal:print_stat(st,"Pipeline Broadcast Dist. Statistics")

        if print_final:
            for r in range(0,part):
                print "Processor :",r
                print "Global Task\tRead Data 1\tRead Data 2\tWrite Data\tRECV \tRECV\tSEND "
                print "-----------\t-----------\t-----------\t----------\t---- \t----\t---- "
                for g in gtask_list:
                    if g[TASK][HOST] == r: 
                        print_task(g)


def run_time_stat(fname):
    dbg=0
    stat=read_timings(fname)
    tws=sum ( [r[-4] for r in stat] ) 
    cws=sum ( [r[-2] for r in stat] ) 
    if dbg: print tws,cws
    idx = fname.find('/')
    while idx>0:
        pos=idx+1
        if dbg:print idx,fname[:pos]
        idx = fname.find('/',idx+1)
    
    fn = open(fname[:pos]+'chol_rtime.stat','a')
    fn.write('\n[ Mtd,Nt,Nb,Np,Nn,LB,CB,Node,TW,TW%,CW,CW% ]\n')
    for s in stat:
        if cws == 0:
            s[-1]='0%'
        else:
            s[-1]=str(int(s[-2]*100/cws))+'%'
        if tws == 0 :
            s[-3]='0%'
        else:
            s[-3]=str(int(s[-4]*100/tws))+'%'
        if dbg:print s
        fn.write(str(s)+'\n')
    fn.close()

def read_timings(fname):
    TRUN,DSND,DRCV=300,1011,1019
    dbg=0
    out = list()
    fn = open(fname)
    line = fn.readline()
    if dbg: print line
    while len(line)>0:
        ln =line.strip().split(' ')        
        if dbg: print ln
        if ln[0] == 'Mtd': 
            line = fn.readline()
            ln =line.strip().split(' ')        
            if dbg: print ln
            Mtd = int(ln[0])
            Nt = int(ln[1])
            Nb = int(ln[2])
            Np = int(ln[3])
            Nn = int(ln[4])
            Node =Nn-1
            CW,TW=0,0            
            line = fn.readline()
            continue
        if ln[0] == 'LB': 
            line = fn.readline()
            ln =line.strip().split(' ')        
            if dbg: print ln
            LB = int(ln[0])
            CB = int(ln[1])            
            line = fn.readline()
            continue
        if ln[0][0]=='*':
            Node = int(ln[0][3])
            out.append ( [Mtd,Nt,Nb,Np,Nn,LB,CB,Node,TW,0,CW,0 ])
            CW,TW=0,0            
            line = fn.readline()
            continue
        event=int(ln [2])
        work=int(ln[len(ln)-1])
        if  event==TRUN:
            TW += work
        if event ==DSND:
            CW += work
        if event ==DRCV:
            CW += work
        if dbg:print "Works",TW,CW
        line = fn.readline()
        if dbg:print line

    fn.close()
    return out

#chol(10,1,1)
#gen_tasks(100,30,30)
#gen_tasks_proc(100,16)
n = int(sys.argv[1])
p = int(sys.argv[2])
part = int(sys.argv[3])*p
loadbal, commpipe,printfinal=0,0,0
if len(sys.argv)>4:
    loadbal = int ( sys.argv[4])
if len(sys.argv)>5:
    commpipe= int ( sys.argv[5])
if len(sys.argv)>6:
    printfinal= int ( sys.argv[6])
if len(sys.argv)>7:    
    tfile=  sys.argv[7]
    run_time_stat(tfile)
else:
    experiment(0,0,commpipe,commpipe,printfinal,loadbal,commpipe,n,p,part)
    export_tasks(gtask_list,'chtasks.txt',p)
#    for t in gtask_list:
#        print_task(t)
