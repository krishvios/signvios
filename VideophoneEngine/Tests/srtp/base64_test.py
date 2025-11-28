#!/usr/bin/env python

import base64

print "'" + base64.b64encode("") + "'"
print "'" + base64.b64encode("01234567") + "'"
print "'" + base64.b64encode("012345678") + "'"
print "'" + base64.b64encode("0123456789") + "'"
