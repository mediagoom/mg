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
    test_core.printcmd(cmd)
    try:
        res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print( "COMMAND ERROR: " + str(e.returncode))
        print(">------------------------------")
        print(e.output)
        print("<------------------------------")
        return False
    res = re.sub("\r\n", "\n", res)
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
    rpl = ['Edit add time \d\d:\d\d:\d\d.\d\d\d', 'Opening File [^\n]+']
    
    cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:20000000']
    res.append(doit(cmd, rpl, '83e8b7d77b1da0d57f360613321a7c18ece8f4b8'))
    
    cmd = [mg, '-k:analyze', '-i:' + mp4]
    res.append(doit(cmd, rpl, '18cdf9deeaaf7593c58981a8cca7c251b271996e'))

    cmd = [mg, '-k:version']
    doit(cmd, None)

    cmd = [mg, '--help']
    doit(cmd, None)

    root = test_core.getroot()

    m4f = os.path.join(root, 'tmp_dash', 'video_750000_800000.m4v')
    cmd = [mg, '-k:analyze', '-i:' + m4f]
    doit(cmd, rpl)
    
    ts = os.path.join(root, 'tmp', 'video_750000_800000.ts')
    cmd = [mg, '-k:all', '-i:' + ts]
    doit(cmd, rpl)

    cmd = [mg, '-k:gop', '-i:' + mp4, '-s:0', '-e:0']
    doit(cmd, None)

    cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:200000000']
    doit(cmd, None)

    cmd = [mg, '-k:mux', '-i:' + mp4, '-s:0', '-e:100000000', '-o:out1.mp4']
    doit(cmd, None)

    cmd = [mg, '-k:mux', '-i:' + mp4, '-s:100000000', '-e:200000000', '-o:out2.mp4']
    doit(cmd, None)

    cmd = [mg, '-k:mux', '-i:out1.mp4', '-s:0', '-e:0', '-i:out2.mp4', '-s:0', '-e:0', '-o:out3.mp4']
    doit(cmd, None)

    cmd = [mg, '-k:pick', '-i:out3.mp4', '-s:0', '-e:200000000']
    doit(cmd, None)


    for x in res:
        if not x:
             raise test_core.TestError("mg command line failed")


mp4 = test_core.getmp4()

mg = test_core.getmg()    

execmg(mg, mp4)

srcd = test_core.getsrcdir()

#cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', "'*.*'", '--blueprint', os.path.join(srcd, 'test_assets', 'hls_small.txt')]

