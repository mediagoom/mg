
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

def addfiles2cmd(files, bitrates):
    cmd = []
    l = len(files)
    x = 0
    add = '-i:'
    while x < l: 
        print('add', files[x])
        cmd.append(add + files[x])
        cmd.append('-b:' + str(bitrates[x]))
        add = '-j:'
        x += 1
    return cmd

def execdashjoins(mg, files, bitrates, do):
    print(mg)
                              #01:28          #2:30
    cmd = [mg, '-k:adaptive', '-s:880000000', '-e:1500000000', '-o:' + do]
    
    cmd2 = addfiles2cmd(files,bitrates)
    cmd += cmd2

            #12:26
    cmd3 = ['-s:7460000000', '-e:0']
    cmd += cmd3

    cmd += cmd2
    
    test_core.execcmd(cmd)

def execdash(mg, files, bitrates, do):
    print(mg)
    cmd = [mg, '-k:adaptive', '-s:0', '-e:0', '-o:' + do]
    
    cmd2 = addfiles2cmd(files,bitrates)
    cmd += cmd2
    
    test_core.execcmd(cmd)

def dlfile(url, dest):
    # Open the url
    try:
        f = urlopen(url)
        print("downloading " + url)

        # Open our local file for writing
        with open(dest, "wb") as local_file:
            local_file.write(f.read())
    #handle errors
    except HTTPError, e:
        print("HTTP Error:", e.code, url)
        raise
    except URLError, e:
        print("URL Error:", e.reason, url)
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

    thedir = os.path.join(cwd, 'tmp','multi')

    files = ["002569741225_Sintel_2010_1080p_mkv_338_144_120.mp4", "002569741225_Sintel_2010_1080p_mkv_676_288_320.mp4"]
    files.append("002569741225_Sintel_2010_1080p_mkv_1352_576_750.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_1200.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_2000.mp4")
    files.append("002569741225_Sintel_2010_1080p_mkv_1690_720_3500.mp4")

    bitrates = [120, 320, 750, 1200, 2000, 3500]

    try:
        opts, args = getopt.getopt(argv,"hufrdb",["url=","dir="])
    except getopt.GetoptError:
        print('test_dash.py --kind <dash|hls>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('test_multiple_bitrate.py -u <url> -f <files> -r <bitrate> -d <directory> -b <blueprint>')
            sys.exit()
        elif opt in ("-u", "--url"):
            url = arg
        elif opt in ("-d", "--dir"):
            thedir = arg

    print( 'url: ', url)
    print( 'dir: ', thedir)
    print( 'mg: ', mg)

    if not os.path.exists(thedir):
        os.makedirs(thedir)

    srcd = test_core.getsrcdir()

    #>----------FILES FULL-----------------

    targets = []
    l = len(files)
    print l
    x = 0
    while x < l: 
        print 'download', files[x]
        targets.append(dofile(url, files[x], thedir))
        x += 1

    dt = os.path.join(thedir, 'STATIC')
    
    test_core.recreatedir(dt)

    dj = os.path.join(thedir, 'JOIN')
    
    test_core.recreatedir(dj)


    execdash(mg, targets, bitrates, dt)
    test_core.sanitize_i_files(dt)

    checkfile = 'multibitrate.txt'

    res = test_hash.hash_check(dt, "*.*", test_hash.exclude, os.path.join(srcd, 'test_assets', checkfile), False, False)

    testresult = test_core.TestResult()
    testresult.name = "Multibirate"
    testresult.result = res
    test_core.reporttest(testresult)

    execdashjoins(mg, targets, bitrates, dj)
    test_core.sanitize_i_files(dj)

    checkfile = 'join.txt'

    res = test_hash.hash_check(dj, "*.*", test_hash.exclude, os.path.join(srcd, 'test_assets', checkfile), False, False)

    testresult2 = test_core.TestResult()
    testresult2.name = "Multibirate-Join"
    testresult2.result = res
    test_core.reporttest(testresult2)



main(sys.argv[1:])