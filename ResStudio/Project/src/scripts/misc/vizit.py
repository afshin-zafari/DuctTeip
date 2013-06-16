#!/usr/bin/env python

import sys
import pylab 

w    = 2.5;  h           = 2.5;  ymax       = 55 ;   xmax    = 60 ;  ncnt   = 4  ; num_thrd = 8
td_sp= 0.3;  thrd_margin = 1.5;  box_margin = 0.1;   x_offset= 2.0;  offset = 9.2;

EVENT_DATA_RECEIVED 	    =    1; EVENT_DATA_ACK 	            =    2; EVENT_LSNR_RECEIVED 	   =   10
EVENT_LSNR_ACK 	            =   20; EVENT_TASK_RECEIVED 	    =  100; EVENT_TASK_ACK	 	       =  200
EVENT_TASK_STARTED 	        =  300; EVENT_TASK_RUNNING 	        =  400; EVENT_TASK_FINISHED 	   =  500
EVENT_ALL_TASK_FINISHED     =  600; EVENT_DATA_STS_CHANGED      = 1000; EVENT_TASK_STS_CHANGED     = 1001
EVENT_LSNR_STS_CHANGED      = 1002; EVENT_DATA_ADDED            = 1003; EVENT_TASK_ADDED           = 1004
EVENT_DATA_POPULATED        = 1005; EVENT_LSNR_CLEANED          = 1006; EVENT_LSNR_DATA_RECEIVED   = 1007
EVENT_LSNR_DATA_SENT        = 1008; EVENT_TASK_CHECKED          = 1009; EVENT_TASK_CLEAND          = 1010
EVENT_DATA_SEND_REQUESTED   = 1011; EVENT_LSNR_SENT_REQUESTED   = 1012; EVENT_TASK_SEND_REQUESTED  = 1013
EVENT_CYCLED                = 1014; EVENT_LSNR_ADDED            = 1015; EVENT_DATA_DEP             = 1016

COMM_AREA = 0;COMP_AREA = 1;THRD_AREA = 2
LSNR_TYPE = 0;DATA_TYPE = 1;TASK_TYPE = 2

splot= ncnt/2*100+21

def read_all_lists(filename):
    fh = open(filename, "r")
    aline = fh.readline()
    out = list()
    cnt = 0
    while len(aline) != 0 :
        if (aline.split()[0]).strip() == "VIZ":
            tststr = aline.split()[1]
            tstamp = int(tststr[len(tststr)-7:len(tststr)])
            if (aline.split()[2]).isdigit():node   = int(aline.split()[2])
            if (aline.split()[3]).isdigit():event  = int(aline.split()[3])
            if (aline.split()[4]).isdigit():i1     = int(aline.split()[4])
            if (aline.split()[5]).isdigit():i2     = int(aline.split()[5])
            i3     = 0 
            if event == EVENT_DATA_DEP:
                if (aline.split()[6]).isdigit():i3     = int(aline.split()[6])
            drawEvent(node,event,tstamp,i1,i2,i3)
            cnt = cnt +1 
        else : 
            print (aline.split()[0]).strip()
            
#        if aline.find('----') >= 0: 
#            k = input("Enter a key to continue")

        aline = fh.readline()
    return out

filename = "manual_viz.txt"
if len(sys.argv) > 1:
    filename = sys.argv[1]



def drawEvent(node,event,tstamp,id1,id2,id3):
    pylab.ioff()
    if   event == EVENT_TASK_ADDED          :
        drawBox(node,COMP_AREA,TASK_TYPE,id1,0)
        if node != 0 : 
            clearBox(node,COMM_AREA,TASK_TYPE,id1)
    elif event == EVENT_TASK_CLEAND         :clearBox(node,COMP_AREA,TASK_TYPE,id1)
    elif event == EVENT_TASK_CHECKED        :drawBox(node,COMP_AREA,TASK_TYPE,id1,-1)
    elif event == EVENT_TASK_STS_CHANGED    :drawBox(node,COMP_AREA,TASK_TYPE,id1,-2)
    elif event == EVENT_TASK_SEND_REQUESTED :drawBox(node,COMM_AREA,TASK_TYPE,id1,0)
    elif event == EVENT_TASK_RECEIVED 	    :drawBox(node,COMM_AREA,TASK_TYPE,id1,0)
    elif event == EVENT_TASK_ACK	 	    :clearBox(node,COMM_AREA,TASK_TYPE,id1)
    elif event == EVENT_TASK_STARTED 	    :drawBox(node,THRD_AREA,id2,id1,0)
    elif event == EVENT_TASK_RUNNING 	    :drawBox(node,THRD_AREA,id2,id1,-1)
    elif event == EVENT_TASK_FINISHED 	    :drawBox(node,THRD_AREA,id2,id1,-2)

    elif event == EVENT_DATA_ADDED          :drawBox(node,COMP_AREA,DATA_TYPE,id1,0)
    elif event == EVENT_DATA_DEP            :drawDep(node,id1,id2,id3)
    elif event == EVENT_DATA_STS_CHANGED    :drawBox(node,COMP_AREA,DATA_TYPE,id1,-1)
    elif event == EVENT_DATA_POPULATED      :drawBox(node,COMP_AREA,DATA_TYPE,id1,-2)
    elif event == EVENT_DATA_SEND_REQUESTED :drawBox(node,COMM_AREA,DATA_TYPE,id1,0)
    elif event == EVENT_DATA_RECEIVED 	    :drawBox(node,COMM_AREA,DATA_TYPE,id1,0)
    elif event == EVENT_DATA_ACK 	        :clearBox(node,COMM_AREA,DATA_TYPE,id1)

    elif event == EVENT_LSNR_ADDED          :drawBox(node,COMP_AREA,LSNR_TYPE,id1,0)
    elif event == EVENT_LSNR_DATA_RECEIVED  :drawBox(node,COMM_AREA,DATA_TYPE,id1,0)
    elif event == EVENT_LSNR_DATA_SENT      :drawBox(node,COMM_AREA,DATA_TYPE,id1,-3)
    elif event == EVENT_LSNR_STS_CHANGED    :drawBox(node,COMP_AREA,LSNR_TYPE,id1,-1)
    elif event == EVENT_LSNR_CLEANED        :clearBox(node,COMP_AREA,LSNR_TYPE,id1)
    elif event == EVENT_LSNR_SENT_REQUESTED :drawBox(node,COMM_AREA,LSNR_TYPE,id1,0)
    elif event == EVENT_LSNR_RECEIVED 	    :drawBox(node,COMM_AREA,LSNR_TYPE,id1,-3)
    elif event == EVENT_LSNR_ACK 	        :clearBox(node,COMM_AREA,LSNR_TYPE,id1)
#    elif event == EVENT_ALL_TASK_FINISHED   :drawBox(node,COMP_AREA,DATA_TYPE,id1,1)
    pylab.ion()
    pylab.draw()

def drawBox(node,area,ot,oid,sts):
    pylab.subplot(node+splot)
    name = ['L','d','t']

    if area != THRD_AREA:
        x0 = oid*w  +box_margin+x_offset
        x1 = oid*w+w-box_margin+x_offset
        y0 = ot*(h+td_sp)  +box_margin+offset*area
        y1 = ot*(h+td_sp)+h-box_margin+offset*area
        if ot == TASK_TYPE and area == COMP_AREA: y0 += offset;y1 += offset
        if sts == -1:
            clr='b'
            boxColor(x0,x1,y0,y1,clr)
        elif sts == -2:
            clr='g'
            boxColor(x0,x1,y0,y1,clr)
        elif sts == -3:
            x0 += 7*w ; x1 += 7*w
            clr=[1,1,0.28]
            boxColor(x0,x1,y0,y1,clr)
        else:
            box(x0,x1,y0,y1)
        drawText(x0+w/2.0,y0+h/2.0,name[ot]+str(oid))

    else:
        x0 = thrd_margin  +box_margin+ w +ot*w
        x1 = thrd_margin+w-box_margin+ w +ot*w
        y0 =  box_margin+offset*area+offset
        y1 = -box_margin+offset*area+offset+num_thrd*h
        if sts == -1:
            clr='b'
            boxColor(x0,x1,y0,y1,clr)
        elif sts == -2:
            clr='g'
            boxColor(x0,x1,y0,y1,clr)
        else:
            box(x0,x1,y0,y1)
        drawText(x0+w/2.0,y0+h/2.0,"t"+str(oid))

def clearBox(node,area,ot,oid):
    pylab.subplot(node+splot)
    x0 = oid*w-0.05 +0.05+x_offset      ; x1 = oid*w+w-0.05-0.025+x_offset
    y0 = ot*(h+td_sp)+offset*area+0.1   ; y1 = ot*(h+td_sp)+h+0.1+offset*area-0.025
    if ot == TASK_TYPE and area == COMP_AREA: y0 += offset
    clr = [[1,1,0.48],[1,1,1],[1,1,1]]
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], ec=clr[area], fc=clr[area])

def drawDep(node,id1,id2,direction):
    pylab.subplot(node+splot)
    dir=['-b','--b']
    x0 = id1*w+w-box_margin-w/2.0+x_offset
    x1 = id2*w + box_margin+w/2.0+x_offset
    y0 = TASK_TYPE*(h+td_sp)+box_margin  +offset*COMP_AREA+offset   
    y1 = DATA_TYPE*(h+td_sp)-box_margin+h+offset*COMP_AREA
    pylab.plot([x0,x1],[y0,y1],dir[direction])

def box(x0, x1, y0, y1):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fill=False)
    
def boxColor(x0, x1, y0, y1,clr):
    pylab.fill([x0, x0, x1, x1], [y0, y1, y1, y0], fc=clr)
    
def drawText(x,y,text):
    pylab.text(x,y,text,horizontalalignment='center',verticalalignment='center',style='italic',size='small')

pylab.ion()
pylab.figure(1,figsize=(19,10) ) 
pylab.ioff()
for nd in range(0,ncnt):
    pylab.subplot(nd+splot)
    pylab.xticks([])
    pylab.yticks([])
    box(0.01,0.02,0.01,0.02)
    box(xmax+0.01,xmax+0.02,ymax+0.01,ymax+0.02)
    boxColor(.05,xmax-0.05,0.05,offset-0.05,[1,1,0.48])
    x = box_margin+w*0.5
    y = offset * THRD_AREA + box_margin + num_thrd*h/2+ offset
    pylab.text(x,y,'Threads',horizontalalignment='center',verticalalignment='center',style='italic',rotation=90)
    y = offset * COMP_AREA + box_margin + offset
    pylab.text(x,y,'Comp',horizontalalignment='center',verticalalignment='center',style='italic',rotation=90)
    y = offset * COMM_AREA + box_margin + offset/2.0
    pylab.text(x,y,'Comm',horizontalalignment='center',verticalalignment='center',style='italic',rotation=90)
    for i in range(0,num_thrd):
        y = i*(h+box_margin)+offset * THRD_AREA + box_margin+h/2 + offset
        pylab.text(x+w*0.5,y,str(i),\
           horizontalalignment='center',verticalalignment='center',style='italic')
pylab.ion()

events = read_all_lists(filename)
print "FIN"
pylab.ioff()
pylab.show()

