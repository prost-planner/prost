#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import platform

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute

from prostlab.experiment import ProstExperiment

from ipc_benchmarks import *

# TODO: Think about these:
from ipc_scores import IPCScores

from prostlab.reports.absolute import AbsoluteReport


def is_running_on_cluster():
    node = platform.node()
    return node.endswith(".scicore.unibas.ch") or node.endswith(".cluster.bc2.ch")

PARTITION = "infai_2"
EMAIL = "tho.keller@unibas.ch"
REPO = "/home/kellert/workspace/planner/prost/prost-dev/"
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

exp = ProstExperiment(suites=SUITES, environment=ENV, num_runs=NUM_RUNS, time_per_step=TIME_PER_STEP)

exp.add_algorithm("ipc11", REPO, REV, "IPC2011", build_options=["-j6"])
exp.add_algorithm("ipc14", REPO, REV, "IPC2014", build_options=["-j6"])

exp.add_parser(ProstExperiment.PROST_PARSER)
exp.add_parser(ProstExperiment.THTS_PARSER)
exp.add_parser(ProstExperiment.IDS_PARSER)

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

exp.add_parse_again_step()

# Attributes to be displayed in the report.
ATTRIBUTES = [
    "run_dir",
] + exp.get_all_attributes()

ipc_scores = IPCScores()

exp.add_report(
    AbsoluteReport(
        attributes=ATTRIBUTES, filter=[ipc_scores.store_rewards, ipc_scores.add_score]
    ),
    outfile="report.html",
)


exp.run_steps()
