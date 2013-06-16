close all
Np=[20000 
    20000 
    20000 
    20000 
    19000
19000
19000
18000
18000
18000
18000
17000
17000
17000
17000
16000
16000
16000
15000
15000
15000
15000
14000
14000
14000
14000
13000
13000
13000
12000
12000
12000
11000
11000
11000
10000
10000
9000
9000
9000
8000
8000
8000
7000
7000
7000
6000
6000
6000
5000
5000
5000
4000
4000
4000
3000
3000
3000
2000
2000
2000
1000
1000
1000
1000
1000
1000
1000
1000
1000
1000
1000
1000
1000
1000

];
Nc=[56
40
24
24
56
40
24
56
40
24
24
56
40
24
24
56
24
24
56
40
24
24
56
40
24
24
56
40
24
56
40
24
56
40
24
56
40
56
40
24
56
40
24
56
40
24
56
40
24
56
40
24
56
40
24
56
40
24
56
40
24
56
40
35
30
30
25
25
24
24
20
20
18
18
15

];
t=[300
300
300
255.04
300
300
300
300
300
300
224.67
249.07
300
224
221.47
210
216.91
213.15
195.98
278.32
208.61
196.81
163.4
255.98
300
164.16
127.51
238.26
164.27
100.71
203.18
139.1
86.66
162.66
120.8
69.47
116.8
54.76
97.36
124.09
46.55
74.44
102.55
38.54
52.47
84.03
31.34
39.71
75.11
25.85
23.94
50.85
22.74
16.48
29.16
20.28
12.23
18.02
20.92
10.86
6.4
18.84
9.08
4.29
3.11
3.01
2.77
2.13
2.59
3.66
1.66
1.38
1.82
1.62
1.09

];
t2=[90.87
128.55
82.19
116.37
74.06
101.68
64.92
91.85
56.56
79.92
142.79
51.11
75.26
124.24
121.49
43.85
64.53
108.66
106.58
38.41
54.2
91.84
31.34
44.11
79
26.05
36.96
67.17
22.23
32.36
53.83
17.58
26.41
44.63
14.1
20.32
35.3
10.76
15.51
25.4
7.88
11.18
18.76
5.55
7.83
13.07
3.57
5.14
8.56
2.11
2.95
4.76
1.07
1.43
2.16
0.39
0.43
0.34
0.38
0.58
0.43
0.79
0.72
0.72
1.01
0.66
0.12
0.16
];
m = min ( size(t,1),size(t2,1));
x = Np(1:m);
y = Nc(1:m);
z=t(1:m);
% plot3(Np(1:m),Nc(1:m),t(1:m),'b.')
% hold on
% plot3(Np(1:m),Nc(1:m),t2(1:m),'rs')
% grid on
% xlabel('Points')
% ylabel('Cores')
% zlabel('Time')


figure

[XI,YI] = meshgrid((1:20).*1000,1:64);
ZI = griddata(x,y,z,XI,YI);

mesh(XI,YI,ZI)

hold
plot3(x,y,z,'o')

z=t2(1:m);
Z2 = griddata(x,y,z,XI,YI);
mesh(XI,YI,Z2) 
plot3(x,y,z,'rs')

axis ij