#!/usr/bin/bash
# -*- coding:utf-8 -*-

# Clustering with message passing.
../starcode test_file_spheres.fastq 2>/dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCC[[:space:]]6AGGGCTTACAAGTATAGGCA[[:space:]]2"

# Sphere clustering.
../starcode --sphere test_file_spheres.fastq 2>/dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCC[[:space:]]8"

# Sphere clustering with --non-redundant.
../starcode --sphere --non-redundant test_file_spheres.fastq 2>/dev/null | \
   tr -d "\r\n" | \
   grep -q "@seq1/1AGGGCTTACAAGTATAGGCC+BBBBBBBBBBBBBBBBBBBB"

# Testing per-read output format, with seq-id option set (shouldn't matter)
../starcode test_file_spheres.fastq --per-read --seq-id 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "GGGCTTACAAGTATAGGCC[[:space:]]1AGGGCTTACAAGTATAGGCC[[:space:]]2AGGGCTTACAAGTATAGGCC[[:space:]]3AGGGCTTACAAGTATAGGCC[[:space:]]4AGGGCTTACAAGTATAGGCC[[:space:]]5AGGGCTTACAAGTATAGGCC[[:space:]]6AGGGCTTACAAGTATAGGCA[[:space:]]7AGGGCTTACAAGTATAGGCA[[:space:]]8"

# Testing per-read output format, with sphere algo
../starcode test_file_spheres.fastq --per-read -s 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCC[[:space:]]1AGGGCTTACAAGTATAGGCC[[:space:]]2AGGGCTTACAAGTATAGGCC[[:space:]]3AGGGCTTACAAGTATAGGCC[[:space:]]4AGGGCTTACAAGTATAGGCC[[:space:]]5AGGGCTTACAAGTATAGGCC[[:space:]]6AGGGCTTACAAGTATAGGCC[[:space:]]7AGGGCTTACAAGTATAGGCC[[:space:]]8"

# Testing on a larger test set, should be identitical for this per-read between
#   the sphere and MP algos
../starcode test_file.txt --per-read 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCC[[:space:]]1AGGGCTTACAAGTATAGGCC[[:space:]]4AGGGCTTACAAGTATAGGCC[[:space:]]5AGGGCTTACAAGTATAGGCC[[:space:]]13AGGGCTTACAAGTATAGGCC[[:space:]]17AGGGCTTACAAGTATAGGCC[[:space:]]20AGGGCTTACAAGTATAGGCC[[:space:]]22CCTCATTATTTGTCGCAATG[[:space:]]3CCTCATTATTTGTCGCAATG[[:space:]]14CCTCATTATTTGTCGCAATG[[:space:]]19CCTCATTATTTGTCGCAATG[[:space:]]27CCTCATTATTTGTCGCAATG[[:space:]]30CCTCATTATTTGTCGCAATG[[:space:]]31CCTCATTATTTGTCGCAATG[[:space:]]32GGGAGCCCACAGTAAGCGAA[[:space:]]6GGGAGCCCACAGTAAGCGAA[[:space:]]7GGGAGCCCACAGTAAGCGAA[[:space:]]10GGGAGCCCACAGTAAGCGAA[[:space:]]12GGGAGCCCACAGTAAGCGAA[[:space:]]15GGGAGCCCACAGTAAGCGAA[[:space:]]24GGGAGCCCACAGTAAGCGAA[[:space:]]25TAGCCTGGTGCGACTGTCAT[[:space:]]8TAGCCTGGTGCGACTGTCAT[[:space:]]9TAGCCTGGTGCGACTGTCAT[[:space:]]16TAGCCTGGTGCGACTGTCAT[[:space:]]21TAGCCTGGTGCGACTGTCAT[[:space:]]28TAGCCTGGTGCGACTGTCAT[[:space:]]33TAGCCTGGTGCGACTGTCAT[[:space:]]35TGCGCCAAGTACGATTTCCG[[:space:]]2"

# Testing on a larger test set, should be identitical for this per-read between
#   the sphere and MP algos
../starcode test_file.txt --per-read -s 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCC[[:space:]]1AGGGCTTACAAGTATAGGCC[[:space:]]4AGGGCTTACAAGTATAGGCC[[:space:]]5AGGGCTTACAAGTATAGGCC[[:space:]]13AGGGCTTACAAGTATAGGCC[[:space:]]17AGGGCTTACAAGTATAGGCC[[:space:]]20AGGGCTTACAAGTATAGGCC[[:space:]]22CCTCATTATTTGTCGCAATG[[:space:]]3CCTCATTATTTGTCGCAATG[[:space:]]14CCTCATTATTTGTCGCAATG[[:space:]]19CCTCATTATTTGTCGCAATG[[:space:]]27CCTCATTATTTGTCGCAATG[[:space:]]30CCTCATTATTTGTCGCAATG[[:space:]]31CCTCATTATTTGTCGCAATG[[:space:]]32GGGAGCCCACAGTAAGCGAA[[:space:]]6GGGAGCCCACAGTAAGCGAA[[:space:]]7GGGAGCCCACAGTAAGCGAA[[:space:]]10GGGAGCCCACAGTAAGCGAA[[:space:]]12GGGAGCCCACAGTAAGCGAA[[:space:]]15GGGAGCCCACAGTAAGCGAA[[:space:]]24GGGAGCCCACAGTAAGCGAA[[:space:]]25TAGCCTGGTGCGACTGTCAT[[:space:]]8TAGCCTGGTGCGACTGTCAT[[:space:]]9TAGCCTGGTGCGACTGTCAT[[:space:]]16TAGCCTGGTGCGACTGTCAT[[:space:]]21TAGCCTGGTGCGACTGTCAT[[:space:]]28TAGCCTGGTGCGACTGTCAT[[:space:]]33TAGCCTGGTGCGACTGTCAT[[:space:]]35TGCGCCAAGTACGATTTCCG[[:space:]]2"
