#!/usr/bin/env python
# -*- coding:utf-8 -*-

import random
import sys

bases = 'GATC'
transit = {
      'A': ('C', 'T', 'G',),
      'T': ('C', 'A', 'G',),
      'G': ('C', 'T', 'A',),
      'C': ('A', 'T', 'G',),
}
numbrcd = 5
brcdlen = 20
tau = 2
ncanon = 6
nmut = 1

seq = []
for i in range(numbrcd):
    canon = [random.choice(bases) for j in range(brcdlen)]
    canon_seq = ''.join(canon)
    seq.extend([canon_seq] * ncanon)
#    sys.stdout.write(('%s\n' % canon_seq) * ncanon)
    for j in range(nmut):
        mut = canon[:]
        for k in random.sample(range(brcdlen), tau):
            mut[k] = random.choice(transit[mut[k]])
        seq.append(''.join(mut))
#        sys.stdout.write('%s\n' % ''.join(mut))

random.shuffle(seq)

for brcd in seq:
    sys.stdout.write('%s\n' % brcd)

