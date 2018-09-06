#!/usr/bin/python

import sys, getopt, os, glob, ast
import hashlib, re

# BUF_SIZE is totally arbitrary, change for your app!
BUF_SIZE = 65536  # lets read stuff in 64kb chunks!

exclude=r'((audio)|(video))_\d+_i\.m4[av]'

#import pdb; pdb.set_trace()

def in_hash(filepath):
    with open(filepath, 'r') as f:
        s = f.read()
        return ast.literal_eval(s)
        #return eval(s)


def out_hash(filepath, hh):
        with open(filepath, 'w') as target:
            target.write(str(hh))

def file_hash(filepath):
    sha1 = hashlib.sha1()
    with open(filepath, 'rb') as f:
            while True:
                data = f.read(BUF_SIZE)
                if not data:
                    break
                sha1.update(data)
    return sha1.hexdigest()

def do_files(dir, filter, excl):
        """process file in a directory"""
        path = os.path.join(dir, filter)
        print(path)
        dic = {}
        rx = re.compile(excl)
        for file in glob.glob(path):
            if None == rx.search(file):	
                hx = file_hash(file)
                dic[os.path.basename(file)] = hx
        return dic

def hash_check(dir, filter, excludeexpr, blueprint, out, verbose):
    hh = do_files(dir, filter, excludeexpr)
    l = len(hh)
    print('dictonary len is ', l) #, hh

    if out:
            out_hash(blueprint, hh)
            #print 'output hash to ', hh
    else:
            zz = in_hash(blueprint)
            if len(zz) > len(hh):
                    print('invalid hash size ', len(zz), len(hh))
                    return False
            for k in zz.keys():
                    if(verbose):
                        print(k, ' ', zz[k], ' ', hh[k])
                    if(zz[k] != hh[k]):
                            print('invalid k ', k, zz[k], hh[k])
                            return False
    return True


def main(argv):
    """main program function"""
    dir=''
    filter=''
    blueprint=''
    out=False
    
    excludeexpr = exclude

    try:
        opts, args = getopt.getopt(argv,"ohd:f:b:",["dir=","filter=","blueprint=","exclude="])
    except getopt.GetoptError as e:
      print(e)
      print('test_hash.py --dir <directory> --filter <*.ts> --blueprint <blueprint-file-path> -o <output:true/false> --exclude <exclude-rx>')
      sys.exit(2)
    for opt, arg in opts:
      if opt == '-h':
         print('test_hash.py -d <directory> -f <filefilter> -b <blueprint>')
         sys.exit()
      elif opt in ("-d", "--dir"):
         dir = arg
      elif opt in ("-f", "--filter"):
         filter = arg
      elif opt in ("-b", "--blueprint"):
         blueprint = arg
      elif opt in ("--exclude"):
          excludeexpr = arg
      elif opt == '-o':
              out=True
    print('dir is : ', dir)
    print('file filter is: ', filter)
    if('' == blueprint):
            blueprint = os.path.dirname(os.path.realpath(__file__))
            blueprint = os.path.join(blueprint, 'test_assets', 'hls_full.txt')
    print('file blueprint is: ', blueprint)
    print('exclude : ', excludeexpr)
    print('out', out)
    
    check = hash_check(dir, filter, excludeexpr, blueprint, out, True)
    if(not check):
        sys.exit(9)
    
    print('ok')

if __name__ == "__main__":
    main(sys.argv[1:])
