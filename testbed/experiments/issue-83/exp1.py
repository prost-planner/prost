#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import platform

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute
from prostlab.experiment import ProstExperiment
from prostlab.reports.absolute import AbsoluteReport

from ipc_benchmarks import *

def is_running_on_cluster():
    node = platform.node()
    return node.endswith(".scicore.unibas.ch") or node.endswith(".cluster.bc2.ch")

PARTITION = "infai_2"
EMAIL = "tho.keller@unibas.ch"
REPO = "/infai/kellert/prost/prost-dev/"
REV = "HEAD"

TIME_PER_STEP = 1.0
if is_running_on_cluster():
    ENV = BaselSlurmEnvironment(partition=PARTITION, email=EMAIL)
    SUITES = IPC_ALL
    NUM_RUNS = 30
    
else:
    ENV = LocalEnvironment(processes=4)
    SUITES = [ELEVATORS_2011[0], NAVIGATION_2011[0]]
    NUM_RUNS = 2

# Generate the experiment
exp = ProstExperiment(suites=SUITES, environment=ENV, num_runs=NUM_RUNS, time_per_step=TIME_PER_STEP)

# Add algorithm configurations to the experiment
exp.add_algorithm("ipc11", REPO, REV, "IPC2011", build_options=["-j6"])
exp.add_algorithm("ipc14", REPO, REV, "IPC2014", build_options=["-j6"])

# Add the default parsers to the experiment
exp.add_parser(ProstExperiment.PROST_PARSER)
exp.add_parser(ProstExperiment.THTS_PARSER)
exp.add_parser(ProstExperiment.IDS_PARSER)

# Add the default steps (build = compile planner; start = run
# experiment and parsers; fetch = collect results)
exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

# Uncomment this if you want to run your parsers again (by default,
# parsers are invoked as part of the "start" step above)
# exp.add_parse_again_step()

# Generate a report only with the default attributes
ATTRIBUTES = [
    "run_dir",
] + exp.get_default_attributes()
exp.add_report(AbsoluteReport(attributes=ATTRIBUTES), outfile="report-default.html")

# Generate a report with all attributes
ATTRIBUTES = [
    "run_dir",
] + exp.get_all_attributes()
exp.add_report(AbsoluteReport(attributes=ATTRIBUTES), outfile="report-all.html")

exp.run_steps()
