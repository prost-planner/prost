#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import datetime
import json
import os
import shutil
import subprocess
import sys

from benchmark_suites import *

############ SLURM PARAMETER ############

# Load "infai" settings for partition and qos
partition = "infai_2"
# partition = 'gki_cpu-ivy'
qos = "normal"

# The email adress that receives an email when the experiment is finished
email = "my.name@unibas.ch"

# Specify when an email is sent. Possible values include NONE, END, or FAIL
mail_type = "NONE"

############ OTHER PARAMETERS ############

# Set to true if you want to run the experiment in debug mode
run_debug = False

# A list of domains that are used in this experiment. Each entry must correspond
# to a folder in testbed/benchmarks. See testbed/benchmark_suites.py for some
# predefined benchmark sets, such as IPC2018 or IPC_ALL.
benchmark = IPC_ALL

# The search engine configurations that are started in this experiment.
# (each of these is run on each instance in the benchmark folder)
configs = [
    "IPC2011",  # The configuration that participated at IPC 2011
    "IPC2014",  # The configuration that participated at IPC 2014
    #'UCT -init [Single -h [RandomWalk]]',              # The configuration that is closest to "plain UCT"
    #'UCT -init [Expand -h [IDS]] -rec [MPA]',          # Best UCT configuration according to Keller's dissertation
    #'DP-UCT -init [Single -h [Uniform]]'               # A configuration that works well in wildfire and sysadmin
]

# The number of runs (should be at least 100 to obtain acceptable confidence)
num_runs = 100

# Average time per step which depends on the horizion and number of runs
# As a rule of thumb, we provide 75*2.5*H seconds for each instance
# (exceptions are possible), where H is the finite horizon. As a result
# of a discussion among the participants and organizers, we decided to
# increase the average deliberation time significantly (this was 50*H seconds before).
step_time = 2.5

# Time to wait for the rddl server to setup in sec
sleep_time = 100

# If enabled, rddlsim ensures that the total runtime is no longer
# than num_runs * step_time * horizon seconds. Otherwise, the planner
# also has step_time seconds per step, but there is no guarantee that
# the total runtime isn't exceeded.
rddlsim_enforces_runtime = False

# The maximum amount of available memory per task. The value's format is
# either "<mem>M" or "<mem>G", where <mem> is an integer number, M
# stands for MByte and G for GByte. Note that PROST additionally has an
# internal memory management that aims to not use more memory than a
# given value (see src/search/prost_planner.cc), but that is not very
# reliable.
memout = "3872M"

# This is the first port that is used for TCP/IP communication with
# rddlsim. Make sure to set this in a way that a number of ports equal to
# the number of tasks of this experiment, starting with port, are open.
port = 2000


############ (USUALLY) NO NEED TO CHANGE THE FOLLOWING ############

# The current revision (used for appropriate naming only).
revision = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .decode("utf-8")
    .splitlines()[0]
)

# The memory limit in kb
if memout[-1] == "M":
    memout_kb = int(memout[:-1]) * 1024
elif memout[-1] == "G":
    memout_kb = int(memout[:-1]) * 1024 * 1024

# A soft memory limit that is used with ulimit
soft_memout_kb = int(0.98 * memout_kb)

# The experiment's name
name = "prost_" + revision

# Directory of testbed folder
testbed_dir = os.path.dirname(os.path.abspath(__file__))

# Directory results are written to
results_dir = os.path.join(testbed_dir, "results", name)

# Path to executable
prost_file = os.path.join(results_dir, "prost")

# The file where the grid engine's stderr is directed to
err_file = os.path.join(results_dir, "slurm.err")

# The file where the grid engine's stdout is directed to
log_file = os.path.join(results_dir, "slurm.log")

# Template for the string that is executed for each job
TASK_TEMPLATE = (
    "export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH && "
    "touch %(run_dir)s/driver.log && "
    "cd %(run_dir)s && "
    "%(testbed_dir)s/run-server.py -b %(testbed_dir)s/benchmarks/%(benchmark)s -p %(port)s -r %(num_runs)d -s 0 --separate-session -t %(run_time)d -l %(run_dir)s "
    "> %(run_dir)s/server.log "
    "2> %(run_dir)s/server.err & "
    "cd %(run_dir)s && "
    "sleep %(sleep_time)d &&"
    '%(prost_file)s %(instance)s -p %(port)s --parser-options "-ipc2018 %(use_ipc2018_parser)s" [PROST -s 1 -ram %(memout)s -se [%(config)s]] '
    "> %(run_dir)s/run.log "
    "2> %(run_dir)s/run.err"
)

# Template for slurm specific commands
SLURM_TEMPLATE = """#! /bin/bash -l
### Set name.
#SBATCH --job-name=%(name)s
### Redirect stdout and stderr.
#SBATCH --error=%(err_file)s
#SBATCH --output=%(log_file)s
### Set partition.
#SBATCH --partition=%(partition)s
### Set quality-of-service group.
#SBATCH --qos=%(qos)s
### Set memory limit
#SBATCH --mem-per-cpu=%(memout)s
### Set timeout
#SBATCH -t %(timeout)s
### Number of tasks.
#SBATCH --array=1-%(num_tasks)s
### Adjustment to priority ([-2147483645, 2147483645]).
#SBATCH --nice=5000
### Send mail?
#SBATCH --mail-type=%(mail_type)s
#SBATCH --mail-user=%(email)s
### Extra options.
ulimit -Sv %(soft_memory_limit)s
"""


def build_planner():
    try:
        if run_debug:
            subprocess.check_call([os.path.join(testbed_dir, "../build.py"), "--debug"])
        else:
            subprocess.check_call([os.path.join(testbed_dir, "../build.py")])
    except subprocess.CalledProcessError as e:
        print("./build.py failed!")
        print("PROST planner could not be compiled.")
    return


def copy_binaries():
    if run_debug:
        parser_name = "rddl-parser-debug"
        parser_file = os.path.join(
            testbed_dir, "../builds/debug/rddl_parser/rddl-parser"
        )
        search_file = ios.path.join(testbed_dir, "../builds/debug/search/search")
    else:
        parser_name = "rddl-parser-release"
        parser_file = os.path.join(
            testbed_dir, "../builds/release/rddl_parser/rddl-parser"
        )
        search_file = os.path.join(testbed_dir, "../builds/release/search/search")

    shutil.copy2(parser_file, os.path.join(results_dir, parser_name))
    shutil.copy2(search_file, os.path.join(results_dir, "prost"))


def create_tasks(filename, instances, timeout):
    global port
    tasks = []
    domains_in_benchmark = set()
    for task in benchmark:
        # The path in benchmark_suites is the directory name.
        # We use this name to identify the domains instead of the RDDL domain file.
        domains_in_benchmark.add(task.path)

    # Create properties file of the whole experiment.
    properties = dict()
    properties["domains"] = list(domains_in_benchmark)
    properties["algorithms"] = configs
    properties["memory_limit"] = memout
    properties["name"] = name
    properties["num_runs"] = num_runs
    properties["partition"] = partition
    properties["revision"] = revision
    properties["run_debug"] = run_debug
    properties["time_limit"] = timeout
    props_path = os.path.join(results_dir, "properties")
    with open(props_path, "w") as fp:
        json.dump(properties, fp, indent=2)

    task_id = 1
    lower = 1
    upper = 100
    for config in configs:
        for instance in sorted(instances):
            run_batch = "runs-{:0>5}-{:0>5}".format(lower, upper)
            run = "{:0>5}".format(task_id)
            run_dir = os.path.join(results_dir, run_batch, run)
            if not os.path.exists(run_dir):
                os.makedirs(run_dir)
                if run_debug:
                    os.symlink(
                        os.path.join(results_dir, "rddl-parser-debug"),
                        os.path.join(run_dir, "rddl-parser-debug"),
                    )
                else:
                    os.symlink(
                        os.path.join(results_dir, "rddl-parser-release"),
                        os.path.join(run_dir, "rddl-parser-release"),
                    )

            run_time = 0
            if rddlsim_enforces_runtime:
                run_time = int(instance[2] * num_runs * step_time)
            task = TASK_TEMPLATE % dict(
                config=config,
                benchmark=instance[0],
                instance=instance[1],
                port=port,
                memout=memout_kb,
                num_runs=num_runs,
                run_time=run_time,
                prost_file=prost_file,
                sleep_time=sleep_time,
                run_dir=run_dir,
                testbed_dir=testbed_dir,
                use_ipc2018_parser=instance[5],
            )

            # Create properties file of the run.
            # Lab requires at least the attributes with the names:
            # domain, problem and algorithm.
            properties = dict()
            properties["domain"] = instance[0]
            properties["algorithm"] = config
            properties["id"] = config, instance[0], instance[1]
            properties["problem"] = instance[1]
            properties["horizon"] = instance[2]
            properties["max_score"] = instance[4]
            properties["memory_limit"] = memout
            properties["min_score"] = instance[3]
            properties["num_runs"] = num_runs
            properties["revision"] = revision
            properties["run_dir"] = run_dir
            properties["run_time"] = run_time
            properties["time_limit"] = timeout
            props_path = os.path.join(run_dir, "static-properties")
            with open(props_path, "w") as fp:
                json.dump(properties, fp, indent=2)

            task_id += 1
            if task_id > upper:
                lower += 100
                upper += 100
            tasks.append(task)
            port = port + 1

    jobs = SLURM_TEMPLATE % dict(
        name=name,
        err_file=err_file,
        log_file=log_file,
        partition=partition,
        qos=qos,
        memout=memout,
        timeout=timeout,
        num_tasks=str(len(tasks)),
        email=email,
        mail_type=mail_type,
        soft_memory_limit=soft_memout_kb,
    )
    jobs += "\n"

    for task_id, task in zip(range(1, len(tasks) + 1), tasks):
        jobs += "if [ " + str(task_id) + " -eq $SLURM_ARRAY_TASK_ID ]; then\n"
        jobs += "    " + task + "\n"
        jobs += "    exit $?\n"
        jobs += "fi\n"

    f = open(filename, "w")
    f.write(str(jobs))
    f.close()


def run_experiment(filename):
    subprocess.check_call(["sbatch", filename, "&"])


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print >>sys.stderr, "Usage: create-jobs.py"
        exit()

    # Create results and log directory
    subprocess.check_call(["mkdir", "-p", results_dir])

    # Compile and copy planner
    build_planner()
    copy_binaries()

    # Generate tasks
    instances = []
    max_horizon = 0
    for instance in benchmark:
        domain_name = instance.path
        problem_name = instance.problem.replace(".rddl", "")
        problem_horizon = instance.horizon
        max_horizon = max(max_horizon, problem_horizon)
        problem_min_score = instance.min_score
        problem_max_score = instance.max_score
        use_ipc2018_parser = int(domain_name.endswith("2018"))
        instances.append(
            (
                domain_name,
                problem_name,
                problem_horizon,
                problem_min_score,
                problem_max_score,
                use_ipc2018_parser,
            )
        )

    # The slurm timeout makes sure that no job runs longer than necessary.
    # We compute the maximal time an instance may take, and pass this as
    # the timeout (with a 5% safety net)
    timeout = str(
        datetime.timedelta(
            seconds=1.05 * max_horizon * num_runs * step_time + sleep_time
        )
    )
    filename = os.path.join(results_dir, "experiment_" + revision)
    create_tasks(filename, instances, timeout)

    # Run experiment
    run_experiment(filename)
