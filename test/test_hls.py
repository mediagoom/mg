#!/usr/bin/python

import subprocess, os, shutil

def getsrcdir():
	srcdir = os.path.dirname(os.path.realpath(__file__))
	return srcdir



def exechls():
	srcd = getsrcdir()
	cmd = ['python', os.path.join(srcd, 'test_dash.py'), '--kind', 'hls']
	#cmd  = [mg, '-k:version']
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	print res


exechls()

