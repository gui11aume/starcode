#!/usr/bin/env python
# -*- coding:utf-8 -*-

import sys
import eUtils

with open(sys.argv[1]) as f:
   pmids = [line.rstrip() for line in f]

sys.stdout.write(eUtils.eFetch_query(id=','.join(pmids)).read())
