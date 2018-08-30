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
    print(line)

def getmg():
    cwd = getroot()
    print(cwd)

    mg = os.path.join(cwd, 'src', 'mgcli', 'mg')

    failure = []
    
    if not os.path.exists(mg):
        failure.append("mg not found in " + mg)
        mg = os.path.join(cwd, 'test', 'out', 'Release', 'mg')
    if not os.path.exists(mg):
        failure.append("mg not found in "+ mg)
        mg = os.path.join(cwd, 'test', 'bin', 'Win32', 'Debug', 'mg.exe')
    if not os.path.exists(mg):
        failure.append("mg not found in " + mg)
        mg = os.path.join(cwd, 'test', 'bin', 'Win32', 'Release', 'mg.exe')
    if not os.path.exists(mg):
        failure.append("mg not found in " + mg)
        mg = os.path.join(cwd, 'test', 'bin', 'x64', 'Debug', 'mg.exe')
    if not os.path.exists(mg):
        failure.append("mg not found in " + mg)
        mg = os.path.join(cwd, 'test', 'bin', 'x64', 'Release', 'mg.exe')
    
    if not os.path.exists(mg):
        print("mg not found!")
        for x in failure:
            print("\t    ", x)
        raise TestError('mg executable not found')

    return mg

def get_i_anal(mg, i_file):
    cmd = [mg, '-k:analyze', '-i:' + i_file]
    printcmd(cmd)
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