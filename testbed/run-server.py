#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import subprocess
import shutil
import sys
import os


def parse_arguments():
    formatter = lambda prog: argparse.ArgumentDefaultsHelpFormatter(
        prog, max_help_position=38
    )
    parser = argparse.ArgumentParser(
        description="Run rddlsim from directory defined "
        "by environment variable RDDLSIM_ROOT.",
        formatter_class=formatter,
    )
    parser.add_argument(
        "--all-ipc-benchmarks",
        action="store_true",
        default=False,
        help="Run all benchmarks.",
    )
    parser.add_argument(
        "-b", "--benchmark", action="store", default=None, help="Path to RDDL files."
    )
    parser.add_argument(
        "-p",
        "--port",
        action="store",
        default="2323",
        type=str,
        help="Port number where rddlsim listens.",
    )
    parser.add_argument(
        "-r",
        "--num-rounds",
        action="store",
        default="30",
        type=str,
        help="Number of rounds.",
    )
    parser.add_argument(
        "-s", "--seed", action="store", default="0", type=str, help="Random seed."
    )
    parser.add_argument(
        "--separate-session",
        action="store_true",
        default=False,
        help="rddlsim terminates " "after a separate session with a client finishes.",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        action="store",
        default="0",
        type=str,
        help="Total timeout in seconds. No timeout is " 'used if timeout is "0".',
    )
    parser.add_argument(
        "-l",
        "--log-dir",
        action="store",
        default="./rddlsim_logs",
        help="Directory where log files are written.",
    )
    parser.add_argument(
        "--monitor-execution",
        action="store_true",
        default=False,
        help="Force client to specify if a " "round is considered.",
    )
    parser.add_argument(
        "-Xms",
        "--init-memory",
        action="store",
        default="100",
        help="Initial amount of memory in MB " "allocated by the Java VM.",
    )
    parser.add_argument(
        "-Xmx",
        "--max-memory",
        action="store",
        default="500",
        help="Maximum amount of memory in MB " "that may be allocated by the Java VM.",
    )
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    # Check if the environment variable RDDLSIM_ROOT exists
    try:
        rddlsim_root = os.environ["RDDLSIM_ROOT"]
    except KeyError:
        err_msg = (
            "Error: an environment variable RDDLSIM_ROOT pointing to "
            "your rddlsim installation must be setup."
        )
        print(err_msg)
        sys.exit()

    args = parse_arguments()

    lib_dir = os.path.join(rddlsim_root, "lib")
    bin_dir = os.path.join(rddlsim_root, "bin")
    jars = os.listdir(lib_dir)
    full_jars = [os.path.join(lib_dir, jar) for jar in jars]

    # Create log_dir folder if it doesn't exist
    log_dir = os.path.abspath(args.log_dir)
    if not os.path.exists(log_dir):
        os.mkdir(log_dir)

    # Turn bool arguments into a 0/1
    separate_session = "0"
    if args.separate_session:
        separate_session = "1"

    monitor_execution = "0"
    if args.monitor_execution:
        monitor_execution = "1"

    if args.all_ipc_benchmarks:
        # If we want to run all benchmarks, we set up a temporary folder with
        # symlinks to all tasks from the 'benchmarks/' directory.
        print("Running all benchmarks...")
        if args.benchmark:
            print('The "benchmark" argument will be ignored!')

        if not os.path.exists("tmp-benchmark"):
            os.mkdir("tmp-benchmark")

        for subdir, dirs, files in os.walk("benchmarks/"):
            for file in files:
                src_file = "{}/{}/{}".format(os.getcwd(), subdir, file)
                year = subdir.split("-")[-1]
                symlink_name = "tmp-benchmark/"
                symlink_name += file.replace(".rddl", "_" + year + ".rddl")
                try:
                    os.symlink(src_file, symlink_name)
                except OSError:
                    err_msg = "Error: {1} probably already exists.".format(
                        src_file, symlink_name
                    )
                    print(err_msg)

        directory = "tmp-benchmark/"

    else:
        if not args.benchmark:
            print("No benchmark specified.")
            sys.exit()
        print("Running benchmark {}...".format(args.benchmark))
        directory = args.benchmark

    try:
        subprocess.check_call(
            [
                "java",
                "-Xms{}M".format(args.init_memory),
                "-Xmx{}M".format(args.max_memory),
                "-classpath",
                "{}:{}".format(bin_dir, ":".join(full_jars)),
                "rddl.competition.Server",
                directory,
                args.port,
                args.num_rounds,
                args.seed,
                separate_session,
                args.timeout,
                log_dir,
                monitor_execution,
            ]
        )
    except KeyboardInterrupt:
        pass
    except subprocess.CalledProcessError as e:
        if e.output:
            print(e.output)

    if args.all_ipc_benchmarks:
        # Delete temporary benchmark directory
        print("Delete temporary directory with symlinks.")
        shutil.rmtree("tmp-benchmark")
    print("Finishing rddlsim.")
