
import os

def version_file():
        file = os.path.dirname(os.path.realpath(__file__))
        file = os.path.join(file, 'src', 'mgcli', 'version.h')
        return file

def write_file(path, cnt):
        with open(path, 'w') as f:
            f.write(cnt)
            

def get_file_content(path, linex):
        i = 1
        ctx = ''
        with open(path, 'r') as f:
                for line in f:
                        if i != 2:
                            ctx += line
                        else:
                            ctx += linex
                            ctx += '\n' #os.linesep
                        i += 1
        return ctx

line='#define VERSION _T("'
v=version_file()
appv=os.environ.get('APPVEYOR_REPO_TAG')

appbuild=os.getenv('APPVEYOR_BUILD_VERSION', 'nn.nn.nn')
appm=os.getenv('APPVEYOR_REPO_COMMIT_MESSAGE', '===')
appb=os.getenv('APPVEYOR_REPO_BRANCH', '---')
apptag=os.getenv('APPVEYOR_REPO_TAG_NAME', 'no tag')

lversion=os.environ.get('VERSION')
if None == lversion:
        lversion='0.0.x'
if None == appv:
        #not in appveyor
        line += lversion
else:
        if('False' == appv):
            line += appbuild 
        else:
            line += apptag
    line += ' '
    line += appm
    line += ' '
    line += appb


line += '")'

cnt = get_file_content(v, line)

print cnt

write_file(v, cnt)
