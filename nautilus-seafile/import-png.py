#!/usr/bin/env python2

import os,sys,shutil

if __name__ == '__main__':
    if len(sys.argv) <= 2:
        print 'This is a little helper script to copy the pngs file to the correct directory'
        print 'Usage: %s [directory] [name]' % sys.argv[0]
        sys.exit(-1)
    os.chdir(os.path.dirname(__file__))
    source_dir = os.path.abspath(sys.argv[1])
    source_name = os.path.splitext(os.path.basename(source_dir))[0]
    png_files = os.listdir(source_dir)
    for png_file in png_files:
        prefix = os.path.splitext(os.path.basename(png_file))[0]
        target_name = os.path.join('hicolor', prefix, 'emblems')
        if not os.path.exists(target_name):
            os.makedirs(target_name)
        source = os.path.join(source_dir, png_file)
        target = os.path.join(target_name, sys.argv[2] + '.png')
        print 'copy %s to %s' % (source, target)
        shutil.copyfile(source, target)
