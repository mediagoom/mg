#!/usr/bin/python

import subprocess, os, shutil
import hashlib, re, glob

import test_core

def files2cmd(path):
    n = 1
    p = re.compile(r".*_(\d+)\..*")
    dash_i = os.path.join(path, '*_i.m4*')
    stream = []
    
    for file in glob.glob(dash_i):
        line = {}
        filename = os.path.basename(file)
        xline = "-S"
        xline += str(n)
        xline += ":"
        xline += file
        line[0] = xline
        print filename
        globname = re.sub("_i", "*", filename)
        dash_s = os.path.join(path, globname)
        print dash_s
        for chunk in glob.glob(dash_s):
            filename = os.path.basename(chunk)
            xline = "-S"
            xline += str(n)
            xline += ":"
            xline += chunk
            print chunk
            m = p.match(chunk)
            if m:
                #print m
                k = long(m.group(1))
                line[k] = xline
        stream.append(line)    
        n = n + 1
    return stream

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
    rpl = ['Edit add time \d\d:\d\d:\d\d.\d\d\d', 'Opening File [^\n]+', 'value mismatch[^\n]+', 'Add File [^\n]+']
    
    root = test_core.getroot()
    mpd = os.path.join(root, 'tmp_dash', 'index.mpd')
    isnew = test_core.is_new_mpd(mpd)

    dashhash = 'ae52490e5b4a7fad52580e9d92d127c5cc798dcf'

    if isnew:
        dashhash = 'aa73ea6d7a24a77c84f19f7b2220414844cfa8a3'

    cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:20000000']
    res.append(doit(cmd, rpl, '83e8b7d77b1da0d57f360613321a7c18ece8f4b8'))
    
    cmd = [mg, '-k:analyze', '-i:' + mp4]
    res.append(doit(cmd, rpl, '18cdf9deeaaf7593c58981a8cca7c251b271996e'))

    cmd = [mg, '-k:version']
    doit(cmd, None)

    cmd = [mg, '--help']
    res.append(doit(cmd, None, '3ad140e7203b801aee493559d372afc21314ab1b'))

    cmd = [mg, '-k:help', '-i:' + mp4 ]
    doit(cmd, None)
    
    m4f = os.path.join(root, 'tmp_dash', 'video_750000_800000.m4v')
    cmd = [mg, '-k:analyze', '-i:' + m4f]
    res.append(doit(cmd, rpl, dashhash))

    m4f = os.path.join(root, 'tmp_widevine', 'video_750000_800000.m4v')
    cmd = [mg, '-k:analyze', '-i:' + m4f]
    doit(cmd, rpl)
    
    ts = os.path.join(root, 'tmp', 'video_750000_800000.ts')
    cmd = [mg, '-k:PES', '-i:' + ts]
    doit(cmd, rpl)

    cmd = [mg, '-k:gop', '-i:' + mp4, '-s:0', '-e:0']
    res.append(doit(cmd, rpl, '2f388d65750cce9954cc3a0bc6a6bd6f750f8c34'))

    cmd = [mg, '-k:pick', '-i:' + mp4, '-s:0', '-e:200000000']
    doit(cmd, rpl) #, 'df3462ebd05e51d0b80eb2831da8bc50c2777d32')

    cmd = [mg, '-k:mux', '-i:' + mp4, '-s:0', '-e:100800000', '-o:out1.mp4']
    doit(cmd, rpl)

    cmd = [mg, '-k:mux', '-i:' + mp4, '-s:100800000', '-e:200400000', '-o:out2.mp4']
    doit(cmd, rpl)

    cmd = [mg, '-k:mux', '-i:out1.mp4', '-s:0', '-e:0', '-i:out2.mp4', '-s:0', '-e:0', '-o:out3.mp4']
    doit(cmd, rpl)

    cmd = [mg, '-k:pick', '-i:out3.mp4', '-s:0', '-e:200000000']
    doit(cmd, rpl)

    #todo: do not crash on missing parameter request
    #cmd = [mg, '-k:test']
    #doit(cmd, rpl)

    cmd = [mg, '-k:test', '-test:RangeMap']
    doit(cmd, rpl)

    cmd = [mg, '-k:test', '-test:parse_date_time', '-time:2018-07-24 15:13:23.012']
    doit(cmd, rpl)

    cmd = [mg, '-k:H264Header', '-i:' + mp4, '-s:0', '-e:200000000']
    doit(cmd, rpl) 

    cmd = [mg, '-k:testvideoindex', '-i:' + mp4]
    doit(cmd, rpl) 
   
    dash_i = os.path.join(root, 'tmp_dash')
    cmd = [mg, '-k:moof', '-o:out_moof.mp4']
    cmd2 = files2cmd(dash_i)
    print cmd2
    for stream in cmd2:
        for key in sorted(stream.iterkeys()):
            cmd.append(stream[key])
    doit(cmd, rpl)

    for x in res:
        if not x:
             raise test_core.TestError("mg command line failed")


mp4 = test_core.getmp4()

mg = test_core.getmg()    

execmg(mg, mp4)

srcd = test_core.getsrcdir()

#cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', "'*.*'", '--blueprint', os.path.join(srcd, 'test_assets', 'hls_small.txt')]

