#! /usr/bin/env python
# -*- coding: utf-8 -*-

import subprocess
import sys
import os

if __name__== "__main__":
    # Check if the environment variable "RDDLSIM_ROOT" exists
    try:
        rddlsim_root = os.environ['RDDLSIM_ROOT']
    except KeyError:
        err_msg = 'Error: an environment variable RDDLSIM_ROOT pointing to '\
                  'your rddlsim installation must be setup.'
        print(err_msg)
        exit(1)

        
    # The only mandatory parameter of the script is the folder containing
    # the instances
    if len(sys.argv) < 2:
        err_msg = 'Error: specify where the RDDL files can be found'
        print(err_msg)
        exit(1)
    benchmark_dir = sys.argv[1]

    # The port where the server listens
    port = '2323'
    if len(sys.argv) > 2:
        port = sys.argv[2]

    num_rounds = '30'
    if len(sys.argv) > 3:
        num_rounds = sys.argv[3]

    # The random seed of the server
    seed = '0'
    if len(sys.argv) > 4:
        seed = sys.argv[4]

    # If this is 1, the server is killed after one interaction with a client.
    # Otherwise, it waits for the next client to connect.
    kill_after_first_session = '0'
    if len(sys.argv) > 5:
        kill_after_first_session = sys.argv[5]

    # The total timeout for the instance in seconds. If this is 0, no timeout
    # at all is used.
    timeout = '0'
    if len(sys.argv) > 6:
        timeout = sys.argv[6]

    # The directory where the server log files are written
    log_dir = '/logs'
    if len(sys.argv) > 7:
        log_dir = sys.argv[7]

    # If this is 1, the client must specify that it wishes that a round "counts"
    monitor_execution = '0'
    if len(sys.argv) > 8:
        monitor_execution = sys.argv[8]

    # Specify the memory that is initially allocated by the Java VM
    initial_memory_alloc = "100M"
    if len(sys.argv) > 9:
        initial_memory_alloc = sys.argv[9]

    # Specify the maximum memory that may be allocated by the Java VM
    maximum_memory_alloc = "500M"
    if len(sys.argv) > 10:
        maximum_memory_alloc = sys.argv[10]

    lib_dir = os.path.join(rddlsim_root, 'lib')
    bin_dir = os.path.join(rddlsim_root, 'bin')
    jars = os.listdir(lib_dir)
    full_jars = [os.path.join(lib_dir, jar) for jar in jars]

    subprocess.check_call(["java", "-Xms{}".format(initial_memory_alloc),
                           "-Xmx{}".format(maximum_memory_alloc), "-classpath",
                           "{}:{}".format(bin_dir, ":".join(full_jars)),
                           "rddl.competition.Server", benchmark_dir, port,
                           num_rounds, seed, kill_after_first_session, timeout,
                           log_dir, monitor_execution])
