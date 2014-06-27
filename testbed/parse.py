#! /usr/bin/env python
# -*- coding: latin-1 -*-

# Starts the preprocess component (src/rddl-parser) of all domain / instance
# pairs that are found in the rddl_prefix subfolder of the given directory, and
# stores them in the 'prost' subfolder.

import os
import sys

if __name__ == "__main__":
    if len(sys.argv) <2:
        print >> sys.stderr, "Usage: ./preprocess-all.py <directory>"
        exit()

    dirs = os.listdir(sys.argv[1])
    if "rddl_prefix" not in dirs:
        print >> sys.stderr, "The given directory must contain a directory called rddl_prefix."
        exit()

    files = os.listdir(sys.argv[1]+"/rddl_prefix")

    domainFiles = list()

    for f in files:
        if f.endswith("_mdp.rddl_prefix"):
            domainFiles.append(f)

    instances = dict()
    for f in files:
        if not f.endswith("_mdp.rddl_prefix"):
            domName = f.split("_inst_mdp")[0] + "_mdp.rddl_prefix"
            if domName not in domainFiles:
                print >> sys.stderr, ("No domain file for instance " + f + ".")
                exit()
            instances[f] = domName

    os.system("rm -rf " + sys.argv[1] + "/prost")
    os.system("mkdir " + sys.argv[1] + "/prost")
    for inst in instances:
        os.system("../src/rddl_parser/rddl-parser " + sys.argv[1] + "/rddl_prefix/" + instances[inst] + " " + sys.argv[1] + "/rddl_prefix/" + inst + " " + sys.argv[1] + "/prost/ -s 1")

