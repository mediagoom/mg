#!/usr/bin/python

import subprocess, os, shutil
import hashlib, re

import test_core


def hashres(res):
	sha1 = hashlib.sha1()
	sha1.update(res)
	return sha1.hexdigest()

def doit(cmd, rpl=None, hash=None):
	testok=False
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	if not rpl is None:
		for h in rpl:
			res = re.sub(h, "", res)
	#print res
	h =  hashres(res)
	if not hash is None:
		if not hash == h:
			print "invalid hash found " + h + " expected " + hash
			print ">---------------------------------------------"
			print res
			print "<---------------------------------------------"
		else:
			print 'hash matched'
			testok=True
	else:
		print "NO HASH: " + h
		print "//>---------------------------------------------"
		print res
		print "//<---------------------------------------------"

	return testok


def execmg(mg, mp4):
	res = []
	rpl = ['Edit add time \d\d:\d\d:\d\d.\d\d\d']
	
	cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:20000000']
	res.append(doit(cmd, rpl, '33bb37cd08ca43d101f717c06e0dd880336f1c91'))
	
	cmd = [mg, '-k:analyze', '-i:' + mp4]
	res.append(doit(cmd, rpl, 'f196260e03a4c42e1f7fa6fcfcaffdf7b99f09d7'))

	cmd = [mg, '-k:version']
	res.append(doit(cmd, None, 'd803c4fffed725bbcca1924baa7b61b660fe8c8e'))

	root = test_core.getroot()
	ts = os.path.join(root, 'tmp/video_750000_800000.ts')
	cmd = [mg, '-k:all', '-i:' + ts]
	doit(cmd, rpl))


	for x in res:
		if not x:
			 raise TestError("mg command line failed")


mp4 = test_core.getmp4()

mg = test_core.getmg()    

execmg(mg, mp4)

srcd = test_core.getsrcdir()

#cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', "'*.*'", '--blueprint', os.path.join(srcd, 'test_assets', 'hls_small.txt')]

