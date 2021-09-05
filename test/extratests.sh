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

# Testing per-read output format, alone
../starcode test_file_spheres.fastq --per-read --seq-id 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "AGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCCAGGGCTTACAAGTATAGGCAAGGGCTTACAAGTATAGGCA"

# Testing per-read output format, with seq-id
../starcode test_file_spheres.fastq --per-read --seq-id 2> /dev/null | \
   tr -d "\r\n" | \
   grep -q "GGGCTTACAAGTATAGGCC[[:space:]]1AGGGCTTACAAGTATAGGCC[[:space:]]2AGGGCTTACAAGTATAGGCC[[:space:]]3AGGGCTTACAAGTATAGGCC[[:space:]]4AGGGCTTACAAGTATAGGCC[[:space:]]5AGGGCTTACAAGTATAGGCC[[:space:]]6AGGGCTTACAAGTATAGGCA[[:space:]]7AGGGCTTACAAGTATAGGCA[[:space:]]8"
