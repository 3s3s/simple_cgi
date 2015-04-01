#!/usr/bin/python

import sys
import os

print "Content-Length = " + os.environ["Content-Length"]
print "VARIABLE2 = " + os.environ["VARIABLE2"]
print "VARIABLE3 = " + os.environ["VARIABLE3"]

body = sys.stdin.read( int(os.environ["Content-Length"]) )
print body