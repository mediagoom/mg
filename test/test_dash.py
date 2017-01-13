#!/usr/bin/python

import subprocess, os, shutil, re

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

def exechls(mg, mp4, do):
    cmd = [mg, '-k:dash', '-s:0', '-e:0', '-b:750', '-i:' + mp4, '-o:' + do]
    #cmd  = [mg, '-k:version']
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

mp4 = getmp4()

do = os.path.join(cwd, 'tmp_dash')

if os.path.exists(do):
    shutil.rmtree(do)    
    
os.makedirs(do)

exechls(mg, mp4, do)

print do

anal_2_file(mg, do, 'audio_96000_i.m4a', 'audio_96000_i.txt')
anal_2_file(mg, do, 'video_750000_i.m4v', 'video_750000_i.txt')

srcd = getsrcdir()

cmd = ['python', os.path.join(srcd, 'test_hash.py'), '--dir', do, '--filter', '*.*', '--blueprint', os.path.join(srcd, 'test_assets', 'dash_small.txt')]

print cmd

try:
    rr = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
except subprocess.CalledProcessError as e:
    print e.output
    raise
    
#print rr
