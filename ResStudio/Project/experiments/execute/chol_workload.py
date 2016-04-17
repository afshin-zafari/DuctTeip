global d,t,s,u
t,d=0,0
s=16
def fn_cnd_oper(nb):
    s,p,d=0,0,0
    for i in range(1,nb+1):
        s=s+2*(i-1)*(nb-i+1+1)
        p=p+nb-i
        d=d+1
    return (s,p,d,s+p+d)
    
def chol(n):
    global d,t,s,u
    if n>s:
        chol(n/2)
        pnl(n/2)
        gemm(n/2)
        chol(n/2)
    else:
        u += 'c '
        t+=1
        d+=1
def pnl(n):
    global d,t,s,u
    if n>s:
        pnl(n/2)
        pnl(n/2)
        gemm(n/2)
        gemm(n/2)
        pnl(n/2)
        pnl(n/2)
    else:
        u += 'p'
        t+=1
        d+=1
def gemm(n):
    global d,t,s,u
    if n>s:
        gemm(n/2)
        gemm(n/2)
        gemm(n/2)
        gemm(n/2)
        gemm(n/2)
        gemm(n/2)
    else:
        u += 'g'
        t+=2

def chol_test(n):
    t =0
    u =' '
    for j in range(0,n):
        u += ' c '
        t += 1
        for i in range(j+1,n):
             u +='p'
             t += 1
        #u += ' '
        for i in range(j+1,n):
            for k in range(0,j+1):
                t += 1
                u += 'g'
    return [u,t] 

    
print "#\tx*s\ttasks\t\td2"
for j in range(1,17):
    t,d = 0 ,0
    u=' '
    k = j 
    chol((k)*s)
    #print j,'\t',k,'\t',t,'\t\t',d+ sum ( range(1,k))
    #print u,len(u)
    [u ,t]= chol_test(k)
    print "--------------------------"
    print k,t,sum ( range(1,k+1))
    
