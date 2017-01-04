#!/usr/bin/python

import subprocess, os

def getmp4():
	srcdir=os.environ.get('srcdir')
	if(srcdir == None):
		srcdir = os.path.dirname(os.path.realpath(__file__))
	return os.path.join(srcdir, 'test_assets', 'MEDIA1.MP4')


def exechls(mg, mp4, do):
	cmd = [mg, '-k:hls', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
	#cmd  = [mg, '-k:version']
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	print res

cwd = os.getcwd()
cwd = os.path.abspath(os.path.join(cwd, os.pardir))
print cwd

mg = os.path.join(cwd, 'src', 'mgcli', 'mg')

mp4 = getmp4()

do = os.path.join(cwd, 'tmp')

if not os.path.exists(do):
    os.makedirs(do)

exechls(mg, mp4, do)

