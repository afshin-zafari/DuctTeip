def create_all(N_,grids):  
  no=0
  for g in grids:
    P,p,q=g
    for B in [45]:
      for b in range(3,10):        
        z=N_/B/b
        #if z>1000: continue
        #if z< 400: continue
        N=z*B*b
        if float(N)/float(N_) < 0.99:continue
        no += 1
        #print "N=",N,";P=",P,";p=",p,";q=",p,";B=",B,";b=",b,";z=",z
        print "    %d) N=%d;P=%d;p=%d;q=%d;B=%d;b=%d;z=%d;;"%(no,N,P,p,q,B,b,z)
  return no
    
def create(N_,P):
  N=N_
  p=int(P**.5)
  no=0
  for a in range(10,40,5):
    B=a*p
    for b in range(3,10):
      z=N_/B/b
      #if z>1000: continue
      if z< 400: continue
      N=z*B*b
      if float(N)/float(N_) < 0.99:continue
      no += 1
      #print "N=",N,";P=",P,";p=",p,";q=",p,";B=",B,";b=",b,";z=",z
      print "    %d) N=%d;P=%d;p=%d;q=%d;B=%d;b=%d;z=%d;;"%(no,N,P,p,p,B,b,z)
  return no
N=100000
P=36
grids=[(9,1,9),(9,9,1)]
import sys
if len(sys.argv)>2:
  N=int(sys.argv[1])
  P=int(sys.argv[2])
print "function set_pars {"
print "  case $1 in" 
no=create_all(N,grids)
print "  esac"
print "}"
print "sim_cnt=%d;"%no
