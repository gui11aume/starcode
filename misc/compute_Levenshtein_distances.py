#!/usr/bin/env python
# -*- coding:utf-8 -*-

import sys

from itertools import combinations

def levenshtein(s1, s2):
    '''This implementation of the Levensthein distance is taken
    from the following address (with the implicit assumption that
    a public wiki version is less likely to have bugs than a private
    version) http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#Python '''
    if len(s1) < len(s2):
        return levenshtein(s2, s1)
 
    # len(s1) >= len(s2)
    if len(s2) == 0:
        return len(s1)
 
    previous_row = range(len(s2) + 1)
    for i, c1 in enumerate(s1):
        current_row = [i + 1]
        for j, c2 in enumerate(s2):
            insertions = previous_row[j + 1] + 1 # j+1 instead of j since previous_row and current_row are one character longer
            deletions = current_row[j] + 1       # than s2
            substitutions = previous_row[j] + (c1 != c2)
            current_row.append(min(insertions, deletions, substitutions))
        previous_row = current_row
 
    return previous_row[-1]

if __name__ == '__main__':
   # Read the fasta file (ignore headers).
   sequences = []
   with open('tiny.fasta') as f:
      for line in f:
         if line[0] == '>': continue
         else: sequences.append(line.rstrip())

   # Compare pairwise Levenshtein distances.
   count = 0
   for a,b in combinations(sequences,2):
      d = levenshtein(a,b)
      if d < 4:
         count += 1
         sys.stdout.write('%d. %s %s\n' % (count, a,b))
