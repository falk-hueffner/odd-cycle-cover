#! /usr/bin/env python

import string, sys, os, re, getopt, popen2

def usage(fd):
    print >> fd, "Usage: occ-lp [-c]"
    print >> fd, "Calculate minimum odd cycle cover by integer linear programming."
    print >> fd, "  -c   Print only the size of the odd cycle cover"

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hcs", ["--help"])
    except getopt.GetoptError:
        usage(sys.stderr)
        sys.exit(2)

    count_only = 0
    stats = 0
    for o, a in opts:
        if o in ("-h", "--help"):
            usage(sys.stdout)
            sys.exit(0)
        if o == "-c":
            count_only = 1
        if o == "-s":
            count_only = 1
            stats = 1

    edges = []
    vertex_names = {}
    vertex_numbers = {}
    next_vertex = 0
    while 1:
        line = sys.stdin.readline()
        if not line:
            break
        line = string.strip(line)
        if not line or line[0] == '#':
            if line == "# Graph Name":
                while string.strip(sys.stdin.readline()) != "# Edges":
                    pass
            continue
        a, b = string.split(line)
        
        if a in vertex_numbers:
            i = vertex_numbers[a]
        else:
            i = next_vertex
            next_vertex += 1
            vertex_numbers[a] = i
            vertex_names[i] = a
        if b in vertex_numbers:
            j = vertex_numbers[b]
        else:
            j = next_vertex
            next_vertex += 1
            vertex_numbers[b] = j
            vertex_names[j] = b
            
        edges.append((i, j))

    n = len(vertex_numbers)
    m = len(edges)

    #glpsol_in, glpsol_out = os.popen2("glpsol --math /dev/stdin --display /dev/null --output /dev/stdout")
    glpsol = popen2.Popen3("glpsol --math /dev/stdin --display /dev/null --output /dev/stdout")

    glpsol_in  = glpsol.tochild
    glpsol_out = glpsol.fromchild

    print >> glpsol_in, \
"""param n > 0 integer;
param m > 0 integer;

param v1{0..m-1}, >= 0;
param v2{0..m-1}, >= 0;

var occ{0..n-1} binary;
var p{0..n-1} binary;

minimize obj: sum{i in 0..n-1} occ[i];

s.t.	neq1{i in 0..m-1}: p[v1[i]] + p[v2[i]] + occ[v1[i]] + occ[v2[i]] >= 1;
	neq2{i in 0..m-1}: p[v1[i]] + p[v2[i]] - occ[v1[i]] - occ[v2[i]] <= 1;

data;

param n := %d;
param m := %d;
        
""" % (n, m)

    print >> glpsol_in, "param v1 :="
    for i in range(0, len(edges)):
        print >> glpsol_in, "\t%2d %2d," % (i, edges[i][0])
    print >> glpsol_in, ";\n\nparam v2 :="
    for i in range(0, len(edges)):
        print >> glpsol_in, "\t%2d %2d," % (i, edges[i][1])
    print >> glpsol_in, ";\n\nend;"

    glpsol_in.close()
    output = glpsol_out.readlines()
    status = glpsol.wait()
    failed = os.WIFSIGNALED(status) or os.WEXITSTATUS(status) != 0

    # example output:
    #    No. Column name       Activity     Lower bound   Upper bound
    #------ ------------    ------------- ------------- -------------
    #     1 occ[0]       *              1             0             1
    #     2 occ[1]       *              0             0             1
    occpat = re.compile('.*occ\[(\d+)\]\s+\*\s+(\d+)')
    occ_size = 0
    for line in output:
        match = occpat.match(line)
        if match:
            if int(match.groups()[1]):
                if not count_only:
                    print vertex_names[int(match.groups()[0])]
                occ_size += 1
    if stats:
        user, system, child_user, child_system, wall = os.times()
        if failed:
            occ_size = "--"
        print "%5d %6d %5s %10.2f" % (n, m, occ_size, user + child_user)
    elif count_only:
        print occ_size
    if failed:
        sys.exit(1)

main()
