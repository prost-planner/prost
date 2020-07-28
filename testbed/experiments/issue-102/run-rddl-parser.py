#! /usr/bin/env python
# -*- coding: latin-1 -*-

import os
import subprocess

from ipc_benchmarks import *

def create_empty_dir(dirname):
   if (os.path.isdir(dirname)):
      if (os.listdir(dirname)):
         print(f"Directory {dirname} is not empty!")
         exit(1)
   else:
      try:
         os.mkdir(dirname)
      except OSError:
         print(f"Creation of directory {dirname} failed.")
         exit(1)

create_empty_dir("parsed_files_2011")
SUITES = IPC_UNTIL_2014
for b in SUITES:
   subprocess.check_call(
      [
         "../../../builds/release/rddl_parser/rddl-parser",
         b.domain_file,
         b.problem_file,
         os.path.join("parsed_files_2011", os.path.split(b.problem_file)[-1][:-5]),
         "-s", "1"
       ]
   )

create_empty_dir("parsed_files_2018")
SUITES = IPC2018
for b in SUITES:
   if int(b.problem_file[-7:-5]) <= 10:
      subprocess.check_call(
         [
            "../../../builds/release/rddl_parser/rddl-parser",
            b.domain_file,
            b.problem_file,
            os.path.join("parsed_files_2018", os.path.split(b.problem_file)[-1][:-5]),
            "-s", "1"
         ]
      )
