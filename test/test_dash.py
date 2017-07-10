#!/usr/bin/python

import subprocess, os, shutil, re, sys, getopt

def getsrcdir():
    srcdir=os.environ.get('srcdir')
    if(srcdir == None):
        srcdir = os.path.dirname(os.path.realpath(__file__))
    return srcdir

def getmp4():
    srcdir = getsrcdir()
    return os.path.join(srcdir, 'test_assets', 'MEDIA1.MP4')

def get_i_anal(mg, i_file):
    cmd = [mg, '-k:analyze', '-i:' + i_file]
    print cmd
    res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    #print res
    return res

def anal_2_file(mg, do, n, nn):
    aia = get_i_anal(mg, os.path.join(do, n))
    aia = aia.replace('\r', '')
    kia = re.split('\n', aia)
    naia = '\n'.join(kia[1:len(kia)-2])
    with open( os.path.join(do, nn), 'wb') as target:
            target.write(naia)

def execdash(mg, mp4, do, extra):
    cmd = [mg, '-k:dash', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
    cmd = cmd + extra
    #cmd  = [mg, '-k:version']
    print cmd
    print '----------------'
    try:
        res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print e.output
        raise

def exechls(mg, mp4, do):
	cmd = [mg, '-k:hls', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
	#cmd  = [mg, '-k:version']
	print cmd
	res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
	print res


def docheck(kind):
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
    mdir = 'tmp'
    checkfile = 'hls_small.txt'
    extra = []
    if 'dash' == kind:
        mdir = 'tmp_dash'   
        checkfile = 'dash_small.txt'
    elif 'widevine' == kind:
        mdir = 'tmp_widevine'
        checkfile = 'widevine_small.txt'
        key =   '-key:100B6C20940F779A4589152B57D2DACB'
        keyid = '-kid:EB676ABBCB345E96BBCF616630F1A3DA'
        widevinebody = '-widevinebody:CAESEOtnarvLNF6Wu89hZjDxo9oaDXdpZGV2aW5lX3Rlc3QiEGZrajNsamFTZGZhbGtyM2oqAkhEMgA='
        extra = [key, keyid, widevinebody]
    do = os.path.join(cwd, mdir)
    if os.path.exists(do):
        shutil.rmtree(do)    
    os.makedirs(do)
    if 'dash' == kind or 'widevine' == kind:
        execdash(mg, mp4, do, extra)
        print do
        anal_2_file(mg, do, 'audio_96000_i.m4a', 'audio_96000_i.txt')
        anal_2_file(mg, do, 'video_750000_i.m4v', 'video_750000_i.txt')
    else:
        exechls(mg, mp4, do)
    srcd = getsrcdir()
    cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', '*.*', '--blueprint', os.path.join(srcd, 'test_assets', checkfile)]
    #print cmd
    try:
        rr = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print e.output
        print "run to update: ", ' '.join( '"' + c + '"' for c in cmd), "-o"
        raise
   


def main(argv):
    """main program function"""
    kind = 'dash'
    try:
        opts, args = getopt.getopt(argv,"k",["kind="])
    except getopt.GetoptError:
      print 'test_dash.py --kind <dash|hls>'
      sys.exit(2)
    for opt, arg in opts:
      if opt == '-h':
         print 'test_hash.py -d <directory> -f <filefilter> -b <blueprint>'
         sys.exit()
      elif opt in ("-k", "--kind"):
         kind = arg
    print 'kind: ', kind
    docheck(kind)

main(sys.argv[1:])
