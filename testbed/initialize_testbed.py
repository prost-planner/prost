#! /usr/bin/env python
# -*- coding: latin-1 -*-

import os
import sys

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print >> sys.stderr, "Usage: ./initialize_testbed.py rddlsim_folder"
        exit()

    rddlsim_folder = sys.argv[1]
    print "create symlink to rddlsim at " + rddlsim_folder

    os.system("ln -s " + rddlsim_folder + " ./rddlsim")

    print "apply patch to rddlsim"
    
    os.system("cp rddlsim.patch " + rddlsim_folder)
    os.chdir(rddlsim_folder)
    print os.getcwd()
    os.system("git apply rddlsim.patch")
    os.system("rm rddlsim.patch")

    print "recompile rddlsim"
    os.system("./compile")
    
