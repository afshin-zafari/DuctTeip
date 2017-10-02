
#13: @Simulation: N=99684,P=16, p=4,q=4,B=12,b=13,k=13,t=54891,T=28,s=24,S=13249204416,r=30,R=16561505520,c=3,z=639,i=7552.000000
import glob as g
files=g.glob('sim_result*')
print 'sim_res=[\\'
for fn in files:
  f=open(fn,'rb')
  for line in f:
    if "@Simulation" not in line: continue
    flds=line.split(',')
    f0=flds[0].split(':')
    rank=int(f0[0])
    del flds[-1]
    rec=map(lambda x: int(x.split('=')[1]),flds)
    print '[',rank,
    for r in rec: print ',',r,
    print '],'
  f.close()
print ']'