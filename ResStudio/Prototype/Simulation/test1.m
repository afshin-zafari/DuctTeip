clear all
close all
clc
res=[];
N=2.^(3:8)
nD = N.^2
nT = N.^3
nC=20;
K = 2000 %(1-nC) .* N ./ (N.*(nC-1) - nC) 
format long 
c1 = (1./K+1).*nT
c2 = c1/nC + nD    
floor(c1./c2)
mean(floor(c1./c2))
m=0:0.1:0.9;
plot(m, 1 ./ (1-0.99 * m))
grid on 