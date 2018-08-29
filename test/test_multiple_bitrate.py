
#!/usr/bin/python

import subprocess, os, shutil, re, sys, getopt
from urllib2 import urlopen, URLError, HTTPError
import test_core, test_hash


#http://defgroupdisks.blob.core.windows.net/builds/PLAY

#002569741225_Sintel_2010_1080p_mkv_338_144_120.mp4
#002569741225_Sintel_2010_1080p_mkv_676_288_320.mp4
#002569741225_Sintel_2010_1080p_mkv_1352_576_750.mp4
#002569741225_Sintel_2010_1080p_mkv_1690_720_1200.mp4
#002569741225_Sintel_2010_1080p_mkv_1690_720_2000.mp4
#002569741225_Sintel_2010_1080p_mkv_1690_720_3500.mp4



def execdash(mg, files, bitrates, do):
    print mg
    cmd = [mg, '-k:adaptive', '-s:0', '-e:0', '-o:' + do]
    
    l = len(files)
    x = 0
    add = '-i:'
    while x < l: 
        print 'add', files[x]
        cmd.append(add + files[x])
        cmd.append('-b:' + str(bitrates[x]))
        add = '-j:'
        x += 1

    test_core.printcmd(cmd)    

    try:
        res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print e.output
        raise

def dlfile(url, dest):
    # Open the url
    try:
        f = urlopen(url)
        print "downloading " + url

        # Open our local file for writing
        with open(dest, "wb") as local_file:
            local_file.write(f.read())
    #handle errors
    except HTTPError, e:
        print "HTTP Error:", e.code, url
        raise
    except URLError, e:
        print "URL Error:", e.reason, url
        raise

def dofile(url, name, thedir):
    target = os.path.join(thedir, name)
    temp = os.path.join(thedir, "tmp.tmp")
    if not os.path.exists(target):
        dlfile(url + "/" + name, temp )
        os.rename(temp, target)
    return target

def main(argv):
    """main program function"""
    url = 'http://defgroupdisks.blob.core.windows.net/builds/PLAY'
    mg = test_core.getmg()    
    cwd = test_core.getroot()

    thedir = os.path.join(cwd, 'tmp_multi')

    files = ["002569741225_Sintel_2010_1080p_mkv_338_144_120.mp4", "002569741225_Sintel_2010_1080p_mkv_676_288_320.mp4"]
    files.append("002569741225_Sintel_2010_1080p_mkv_1352_576_750.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_1200.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_2000.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_3500.mp4")

    bitrates = [120, 320, 750, 1200, 2000, 3500]

    try:
        opts, args = getopt.getopt(argv,"hufrdb",["url=","dir="])
    except getopt.GetoptError:
        print 'test_dash.py --kind <dash|hls>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test_multiple_bitrate.py -u <url> -f <files> -r <bitrate> -d <directory> -b <blueprint>'
            sys.exit()
        elif opt in ("-u", "--url"):
            url = arg
        elif opt in ("-d", "--dir"):
            thedir = arg

    print 'url: ', url
    print 'dir: ', thedir
    print 'mg: ', mg

    if not os.path.exists(thedir):
        os.makedirs(thedir)

    dt = os.path.join(thedir, 'STATIC')
    
    if os.path.exists(dt):
        shutil.rmtree(dt)    
    os.makedirs(dt)

    targets = []
    l = len(files)
    print l
    x = 0
    while x < l: 
        print 'download', files[x]
        targets.append(dofile(url, files[x], thedir))
        x += 1

    execdash(mg, targets, bitrates, dt)

main(sys.argv[1:])