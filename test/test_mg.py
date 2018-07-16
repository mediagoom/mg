#!/usr/bin/python

import subprocess, os, shutil

def getsrcdir():
	srcdir=os.environ.get('srcdir')
	if(srcdir == None):
		srcdir = os.path.dirname(os.path.realpath(__file__))
	return srcdir

def getmp4():
	srcdir = getsrcdir()
	return os.path.join(srcdir, 'test_assets', 'MEDIA1.MP4')


def execmg(mg, mp4):
	cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:20000000']
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	print res
	cmd = [mg, '-k:analyze', '-i:' + mp4]
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	print res

cwd = os.getcwd()
cwd = os.path.abspath(os.path.join(cwd, os.pardir))
print cwd

mg = os.path.join(cwd, 'src', 'mgcli', 'mg')

print mg

if not os.path.exists(mg):
    mg = os.path.join(cwd, 'test', 'out', 'Release', 'mg')
if not os.path.exists(mg):
    mg = os.path.join(cwd, 'test', 'bin', 'Win32', 'Debug', 'mg.exe')
if not os.path.exists(mg):
    mg = os.path.join(cwd, 'test', 'bin', 'Win32', 'Release', 'mg.exe')
if not os.path.exists(mg):
    mg = os.path.join(cwd, 'test', 'bin', 'x64', 'Release', 'mg.exe')

mp4 = getmp4()


    

execmg(mg, mp4)

srcd = getsrcdir()

#cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', "'*.*'", '--blueprint', os.path.join(srcd, 'test_assets', 'hls_small.txt')]

