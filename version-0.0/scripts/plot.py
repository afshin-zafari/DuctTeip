#!/usr/bin/env python

import sys
import pylab

def readvec(line):
  vec = line.rstrip("\n").split(' ')
  for i in range(0, len(vec)):
    vec[i] = int(vec[i])
  return vec

def readvecd(line):
  vec = line.rstrip("\n").split(' ')
  for i in range(0, len(vec)):
    vec[i] = float(vec[i])
  return vec

if (len(sys.argv) < 2):
  print "Usage: " + sys.argv[0] + " <filename>"
  sys.exit(0)

file = open(sys.argv[1]);

header = readvec(file.readline())
#header = [0, 0, 1]
x = []
y = []

print(header)
for i in range(0,header[2]):
  subheader = readvec(file.readline())
  print(subheader)
  xx = []
  yy = []
  for j in range(0, subheader[1]):
    line = readvecd(file.readline())
    xx.append(line[0])
    yy.append(line[1])
  x.append(xx)
  y.append(yy)

#cols=['^k', 'vk', '<k', '>k', '^w', 'vw', '<w','>w','.g','.m','.y']
cols =['ow', 'ok', 'ow', 'ok', 'ow', 'ok', 'ok','ow','.g','.m','.y']
cols2=['.k', '.w', '.k', '.w', 'ow', 'ok', 'ok','ow','.g','.m','.y']

for i in range(0, len(x)):
  pylab.plot(x[i], y[i], cols[i], markersize=3)
  pylab.plot(x[i], y[i], cols2[i], markersize=3)
pylab.axis([-.1, 1.1, -.1, 1.1])

pylab.show()

