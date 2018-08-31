import subprocess, os, shutil
import re, glob, datetime, json, urllib2

class TestError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class TestResult:
    def __init__(self):
        self.startime = datetime.datetime.now()
        self.result = True
        self.name = None
        self.message = ""
        self.duration = None

    def end(self):
        if(None == self.duration):
            self.duration = datetime.datetime.now() - self.startime
            self.duration = (self.duration.seconds + self.duration.microseconds/1E6) * 1000

def appveyourapicall(path, values):
    appurl = os.environ.get('APPVEYOR_API_URL')
    r = json.dumps(values)

    if not None == appurl:
        url = appurl + path
        
        print('APIREQUEST', url, r)

        req = urllib2.Request(url, r , headers={'Content-type': 'application/json', 'Accept': 'application/json'})
        response = urllib2.urlopen(req)
        the_page = response.read()
        print(the_page)
    else:
        print('NO API SERVER', path, r)

def reporttest(testresult):
    name = testresult.name
    if(None == name):
        name = "None"
    testresult.end()
    duration = testresult.duration
    
    outcome = "Passed"
    if(not testresult.result):
        outcome = "Failed"
    if(None == testresult.result):
        outcome = "Inconclusive"

    values = {
    "testName": name,
    "testFramework": "NUnit",
    "fileName": "python.py",
    "outcome": outcome,
    "durationMilliseconds": str(duration),
    "ErrorMessage": testresult.message,
    "ErrorStackTrace": "",
    "StdOut": "",
    "StdErr": ""
    }

    appveyourapicall("api/tests", values)

def execcmd(cmd, verbose = False):
    printcmd(cmd) 
    try:
        res = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
        if(verbose):
            print(res)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise

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

def sanitize_i_files(thepath):
    path = os.path.join(thepath, '*_i.m4*')
    mg = getmg()

    for file in glob.glob(path):
        print(file)
        anal_2_file(mg, thepath, os.path.basename(file),  re.sub('m4[va]', 'txt', os.path.basename(file).lower()))


def recreatedir(dt):
    if os.path.exists(dt):
        shutil.rmtree(dt)    
    
    j = 0
    done = False


    while(j < 10):
        try:
            os.makedirs(dt)
            done = True
        except Exception as e:
            print(e.output)
        j += 1

    if Not done:
        raise
        