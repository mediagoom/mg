#!/usr/bin/python

import subprocess, os, shutil, re, sys, getopt
import test_core, test_hash





def execdash(mg, mp4, do, extra):
    cmd = [mg, '-k:dash', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
    cmd = cmd + extra
    #cmd  = [mg, '-k:version']
    test_core.printcmd(cmd)
    print('----------------')
    try:
        res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise

def exechls(mg, mp4, do):
    cmd = [mg, '-k:hls', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
    #cmd  = [mg, '-k:version']
    test_core.printcmd(cmd)
    res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    print(res)

def exechelp(mg):
    cmd = [mg, '--help']
    test_core.printcmd(cmd)
    res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    print(res)


def docheck(kind):
    
    mg = test_core.getmg()    
    mp4 = test_core.getmp4()
    cwd = test_core.getroot()

    mdir = 'hls'
    checkfile = 'hls_small.txt'
    extra = []
    if 'dash' == kind:
        mdir = 'dash'   
        checkfile = 'dash_small.txt'
    elif 'widevine' == kind:
        mdir = 'widevine'
        checkfile = 'widevine_small.txt'
        key =   '-key:100B6C20940F779A4589152B57D2DACB'
        keyid = '-kid:EB676ABBCB345E96BBCF616630F1A3DA'
        widevinebody = '-widevinebody:CAESEOtnarvLNF6Wu89hZjDxo9oaDXdpZGV2aW5lX3Rlc3QiEGZrajNsamFTZGZhbGtyM2oqAkhEMgA='
        extra = [key, keyid, widevinebody]
    do = os.path.join(cwd, 'tmp', mdir)
    if os.path.exists(do):
        shutil.rmtree(do)    
    os.makedirs(do)
    if 'dash' == kind or 'widevine' == kind:
        execdash(mg, mp4, do, extra)
        print(do)
        test_core.anal_2_file(mg, do, 'audio_96000_i.m4a', 'audio_96000_i.txt')
        test_core.anal_2_file(mg, do, 'video_750000_i.m4v', 'video_750000_i.txt')

        mpd = os.path.join(do, 'index.mpd')
        isnew = test_core.is_new_mpd(mpd)

        print("mpd: ", mpd, " isnew: " , isnew)
        if isnew:
            checkfile = "new_" + checkfile


    elif 'hls' == kind:
        exechls(mg, mp4, do)
    else:
        exechelp(mg)
    srcd = test_core.getsrcdir()
    
    cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', '*.*', '--blueprint', os.path.join(srcd, 'test_assets', checkfile)]
    #test_core.printcmd(cmd)
    
    #try:
    #    rr = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    #except subprocess.CalledProcessError as e:
    #    print(e.output)
    #    print("run to update: ", ' '.join( '"' + c + '"' for c in cmd), "-o")
    #    raise
   
    #print(rr)

    res = test_hash.hash_check(do, "*.*", test_hash.exclude, os.path.join(srcd, 'test_assets', checkfile), False, False)

    if(not res):
        print('FAILED')
        print("run to update: ", ' '.join( '"' + c + '"' for c in cmd), "-o")
        sys.exit(45)
    else:
        print('OK')

def main(argv):
    """main program function"""
    kind = 'dash'
    try:
        opts, args = getopt.getopt(argv,"k",["kind="])
    except getopt.GetoptError:
      print('test_dash.py --kind <dash|hls>')
      sys.exit(2)
    for opt, arg in opts:
      if opt == '-h':
         print('test_hash.py -d <directory> -f <filefilter> -b <blueprint>')
         sys.exit()
      elif opt in ("-k", "--kind"):
         kind = arg
    print('kind: ', kind)
    docheck(kind)

main(sys.argv[1:])
