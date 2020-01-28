#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import subprocess
import sys
import os

def parse_arguments():
    formatter = lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog,max_help_position=48)
    parser = argparse.ArgumentParser(description = 'Run RDDL server from directory defined '
                                                   'by environment variable RDDLSIM_ROOT.',
                                     formatter_class = formatter)
    parser.add_argument('benchmark_dir', action='store',
                        help='Path to RDDL files.')
    parser.add_argument('port', action='store', default = '2323', nargs='?',
                        help='Port number where the server listens.')
    parser.add_argument('num_rounds', action='store', default = '30', nargs='?',
                        help='Number of rounds.')
    parser.add_argument('seed', action='store', default = '0', nargs='?',
                        help='Random seed of the server.')
    parser.add_argument('kill_after_first_session', action='store', default = '0', nargs='?',
                        help='If this is 1, the server is killed after one interaction with a client. '
                        'Otherwise, it waits for the next client to connect.')
    parser.add_argument('timeout', action='store', default = '0', nargs='?',
                        help='Timeout in seconds. If "0" is passed then no timeout is used.')
    parser.add_argument('log_dir', action='store', default = '/logs', nargs='?',
                        help='Directory where log files are written.')
    parser.add_argument('monitor_execution', action='store', default = '0', nargs='?',
                        help='If this is 1, the client must specify that it wishes that a round "counts".')
    parser.add_argument('initial_memory_alloc', action='store', default = '100M', nargs='?',
                        help='Initial amount of memory allocated by the Java VM.')
    parser.add_argument('maximum_memory_alloc', action='store', default = '500M', nargs='?',
                        help='Maximum amount of memory that can be allocated by the Java VM.')
    args = parser.parse_args()
    return args

if __name__== "__main__":
    # Check if the environment variable "RDDLSIM_ROOT" exists
    try:
        rddlsim_root = os.environ['RDDLSIM_ROOT']
    except KeyError:
        err_msg = 'Error: an environment variable RDDLSIM_ROOT pointing to '\
                  'your rddlsim installation must be setup.'
        print(err_msg)
        exit(1)

    args = parse_arguments()

    lib_dir = os.path.join(rddlsim_root, 'lib')
    bin_dir = os.path.join(rddlsim_root, 'bin')
    jars = os.listdir(lib_dir)
    full_jars = [os.path.join(lib_dir, jar) for jar in jars]

    subprocess.check_call(["java", "-Xms{}".format(args.initial_memory_alloc),
                           "-Xmx{}".format(args.maximum_memory_alloc), "-classpath",
                           "{}:{}".format(bin_dir, ":".join(full_jars)),
                           "rddl.competition.Server", args.benchmark_dir, args.port,
                           args.num_rounds, args.seed, args.kill_after_first_session, args.timeout,
                           args.log_dir, args.monitor_execution])
