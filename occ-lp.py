#! /usr/bin/env python

import string, sys, os, re, getopt, popen2

def usage(fd):
    print >> fd, "Usage: occ-lp [-c]"
    print >> fd, "Calculate minimum odd cycle cover by integer linear programming."
    print >> fd, "  -c   Print only the size of the odd cycle cover"
    print >> fd, "  -s   Print only statistics"
    print >> fd, "  -e   Cover by edges (default: vertices)"
    print >> fd, "  -m   Do not solve, just prin the LP in GNU MathProg format"
    print >> fd, "  -l   Do not solve, just prin the LP in CPLEX LP format"

def mathprog_lp(vertex_numbers, edges, is_edge_occ):
    n = len(vertex_numbers)
    m = len(edges)

    prog = \
"""
param n > 0 integer;
param m > 0 integer;

param v1{0..m-1}, >= 0;
param v2{0..m-1}, >= 0;

var occ{0..%(occ_range)s-1} binary;
var p{0..n-1} binary;

minimize obj: sum{i in 0..%(occ_range)s-1} occ[i];

s.t.	neq1{i in 0..m-1}: p[v1[i]] + p[v2[i]] + (%(occ_fudge)s) >= 1;
	neq2{i in 0..m-1}: p[v1[i]] + p[v2[i]] - (%(occ_fudge)s) <= 1;

data;

param n := %(n)d;
param m := %(m)d;
        
""" % { 'n' : n,
        'm' : m,
        'occ_range' : is_edge_occ and "m" or "n",
        'occ_fudge' : is_edge_occ and "occ[i]" or "occ[v1[i]] + occ[v2[i]]" }

    prog += "param v1 :="
    for i in range(0, len(edges)):
        prog += "\t%2d %2d," % (i, edges[i][0])
    prog += ";\n\nparam v2 :="
    for i in range(0, len(edges)):
        prog += "\t%2d %2d," % (i, edges[i][1])
    prog += ";\n\nend;"

    return prog
    

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hcseml", ["--help"])
    except getopt.GetoptError:
        usage(sys.stderr)
        sys.exit(2)

    count_only = False
    stats = False
    is_edge_occ = False
    just_mathprog = False
    just_cplex = False
    for o, a in opts:
        if o in ("-h", "--help"):
            usage(sys.stdout)
            sys.exit(0)
        if o == "-e":
            is_edge_occ = True
        if o == "-c":
            count_only = True
        if o == "-s":
            count_only = True
            stats = True
        if o == "-m":
            just_mathprog = True
        if o == "-l":
            just_cplex = True

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
    prog = mathprog_lp(vertex_numbers, edges, is_edge_occ)

    if just_mathprog:
        print prog
        sys.exit(0)

    glpsol = popen2.Popen3("glpsol --math /dev/stdin --display /dev/null --output /dev/stdout")
    glpsol_in  = glpsol.tochild
    glpsol_out = glpsol.fromchild

    print >> glpsol_in, prog
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
                    occ_el = int(match.groups()[0])
                    if is_edge_occ:
                        edge = edges[occ_el]
                        print vertex_names[edge[0]], vertex_names[edge[1]]
                    else:
                        print vertex_names[occ_el]
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
