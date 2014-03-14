import os
import itertools
import sys

tau = int(sys.argv[1])
prefixes = [i for i in itertools.product('ACTG N',repeat=tau)]
distgroups = [[prefixes.pop(0)]]

def nwdist(p,q):
    L = len(p)+1
    
    # Initialize matrix.
    m = [[0 for x in xrange(L)] for x in xrange(L)]
    for x in range(L):
        m[x][0] = m[0][x] = x
        
    # Start algorithm.
    for c in range(L-1):
        for r in range(L-1):
            m[r+1][c+1] = min( m[r][c] + int(p[r]!=q[c]), min(m[r+1][c],m[r][c+1])+1)
    
    return m[-1][-1]
   


for p in prefixes:
    newgroup = True

    # Iterate over all the groups
    for group in distgroups:
        maxdist = True

        # Check all the distances with all the prefixes in this group
        for q in group:
            if nwdist(p,q) != tau:
                # If distance is not max, this is not your group
                maxdist = False
                break

        # Add to group if dist max with all group members
        if maxdist == True:
            group.append(p)
            newgroup = False
            break

    # If p did not find any suitable group, make a new one
    if newgroup == True:
        distgroups.append([p])

for group in distgroups:
    for prefix in group:
        print ''.join(prefix)
    print ''
