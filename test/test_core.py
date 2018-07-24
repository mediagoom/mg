import subprocess, os, shutil
import re

class TestError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

def getfilestring(path):
    with open(path, 'r') as myfile:
        data = myfile.read()
        return data

def is_new_mpd(mpdpath):
    mpd = getfilestring(mpdpath)
    m = re.match(".*ccff.*", mpd, re.S)
    if m:
        return False
    return True

def getsrcdir():
    srcdir=os.environ.get('srcdir')
    if(srcdir == None):
        srcdir = os.path.dirname(os.path.realpath(__file__))
    return srcdir

def getroot():
    cwd = getsrcdir() #os.getcwd()
    cwd = os.path.abspath(os.path.join(cwd, os.pardir))
    return cwd

def getmp4():
    srcdir = getsrcdir()
    return os.path.join(srcdir, 'test_assets', 'MEDIA1.MP4')

def printcmd(cmd):
    line = ""
    for c in cmd:
        line += '"' + c + '" '
    print line

def getmg():
    cwd = getroot()
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
    
    if not os.path.exists(mg):
        raise TestError('mg executable not found')

    return mg