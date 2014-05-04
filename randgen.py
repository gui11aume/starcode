#!/usr/bin/env python
# -*- coding:utf-8 -*-

import random
import sys

bases = "ACTG"
numbrcd = 10000
brcdlen = 20
tau = 3
ncanon = 47
nmut = 3

seq = []
for i in range(numbrcd):
    canon = [random.choice(bases) for j in range(brcdlen)]
    canon_seq = ''.join(canon)
    seq.extend([canon_seq] * ncanon)
    for j in range(nmut):
        mut = canon
        for k in random.sample(range(brcdlen), tau):
            mut[k] = random.choice(bases)
        seq.append(''.join(mut))

random.shuffle(seq)

for brcd in seq:
    sys.stdout.write('%s\n' % brcd)

