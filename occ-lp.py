#! /usr/bin/env python

import string, sys, tempfile, os, re

def main():    
    edges = []
    for line in sys.stdin.readlines():
        a, b = string.split(line)
        edges.append((int(a), int(b)))

    n = max([max(a, b) for a, b in edges]) + 1
    m = len(edges)

    glpsol_in, glpsol_out = os.popen2("glpsol --math /dev/stdin --display /dev/null --output /dev/stdout")

    print >> glpsol_in, \
"""param n > 0 integer;
param m > 0 integer;

set I := 1..m;		/* Edge index set  */

param v1{0..m-1}, >= 0;
param v2{0..m-1}, >= 0;

var occ{0..n-1} binary;
var p{0..n-1} binary;

minimize obj: sum{i in 0..n-1} occ[i];

s.t.	neq1{i in 0..m-1}: p[v1[i]] + p[v2[i]] + occ[v1[i]] + occ[v2[i]] >= 1;
	neq2{i in 0..m-1}: p[v1[i]] + p[v2[i]] - occ[v1[i]] - occ[v2[i]] <= 1;

data;

param n := %(n)d;
param m := %(m)d;
        
""" % { 'n': n, 'm': m }

    print >> glpsol_in, "param v1 :="
    for i in range(0, len(edges)):
        print >> glpsol_in, "\t%2d %2d," % (i, edges[i][0])
    print >> glpsol_in, ";\n\nparam v2 :="
    for i in range(0, len(edges)):
        print >> glpsol_in, "\t%2d %2d," % (i, edges[i][1])
    print >> glpsol_in, ";\n\nend;"

    glpsol_in.close()

    occpat = re.compile('.*occ\[(\d+)\]\s+\*\s+(\d+)')
    for line in glpsol_out.readlines():
        m = occpat.match(line)
        if m:
            if int(m.groups()[1]):
                print m.groups()[0]

main()
