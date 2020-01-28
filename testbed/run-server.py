#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import subprocess
import shutil
import sys
import os

def parse_arguments():
    formatter = lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog,max_help_position=32)
    parser = argparse.ArgumentParser(description = 'Run RDDL server from directory defined '
                                                   'by environment variable RDDLSIM_ROOT.',
                                     formatter_class = formatter)
    parser.add_argument('--all-benchmarks', action='store_true', default=False,
                        help="Run all benchmarks.")
    parser.add_argument('-b', '--benchmark', action='store', default=None,
                        help='Path to RDDL files.')
    parser.add_argument('-p', '--port', action='store', default = '2323', type=str,
                        help='Port number where the server listens.')
    parser.add_argument('-r', '--num-rounds', action='store', default = '30', type=str,
                        help='Number of rounds.')
    parser.add_argument('-s', '--seed', action='store', default = '0', type=str,
                        help='Random seed of the server.')
    parser.add_argument('-ss', '--separate-session', action='store', default = '0', type=str,
                        help='The server terminates after a separate session with a client finishes.')
    parser.add_argument('-t', '--timeout', action='store', default = '5', type=str,
                        help='Timeout in seconds. If "0" is passed then no timeout is used.')
    parser.add_argument('-l', '--log-dir', action='store', default = '/logs',
                        help='Directory where log files are written.')
    parser.add_argument('-e', '--monitor-execution', action='store', default = '0', type=str,
                        help='If execution is monitored, a client must specify at the beginning of each round if it counts.')
    parser.add_argument('-Xms', '--init-memory', action='store', default = '100M',
                        help='Initial amount of memory allocated by the Java VM.')
    parser.add_argument('-Xmx', '--max-memory', action='store', default = '500M',
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
        sys.exit()

    args = parse_arguments()

    lib_dir = os.path.join(rddlsim_root, 'lib')
    bin_dir = os.path.join(rddlsim_root, 'bin')
    jars = os.listdir(lib_dir)
    full_jars = [os.path.join(lib_dir, jar) for jar in jars]

    if args.all_benchmarks:
        # If we want to run all benchmarks, we set up a temporary folder with
        # symlinks to all tasks from the 'benchmarks/' directory.
        print ('Running all benchmarks...')
        if args.benchmark:
            print ('The "benchmark" argument will be ignored!')

        if not os.path.exists('tmp-benchmark'):
            os.mkdir('tmp-benchmark')

        for subdir, dirs, files in os.walk('benchmarks/'):
            for file in files:
                src_file = '{}/{}/{}'.format(os.getcwd(), subdir, file)
                year = subdir.split('-')[-1]
                symlink_name = 'tmp-benchmark/' + file.replace('.rddl', '_'+year+'.rddl')
                try:
                    os.symlink(src_file,  symlink_name)
                except OSError:
                    print ("Error: {1} probably already exists.".format(src_file, symlink_name))

        directory = 'tmp-benchmark/'

    else:
        if not args.benchmark:
            print ('No benchmark specified.')
            sys.exit()
        print ('Running benchmark %s...' % args.benchmark)
        directory = args.benchmark

    try:
        subprocess.check_call(["java", "-Xms{}".format(args.initial_memory_alloc),
                               "-Xmx{}".format(args.maximum_memory_alloc), "-classpath",
                               "{}:{}".format(bin_dir, ":".join(full_jars)),
                               "rddl.competition.Server", directory, args.port,
                               args.num_rounds, args.seed, args.separate_session, args.timeout,
                               args.log_dir, args.monitor_execution])
    except KeyboardInterrupt:
        if args.all_benchmarks:
            # Delete temporary directory
            print ("Delete temporary directory with symlinks.")
            shutil.rmtree('tmp-benchmark')
        print ("Finishing RDDLsim.")
