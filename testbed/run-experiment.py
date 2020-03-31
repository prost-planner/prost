#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import platform

from lab.environments import LocalEnvironment, BaselSlurmEnvironment

from benchmark_suites import *
from experiment import ProstExperiment


def is_running_on_cluster():
    node = platform.node()
    return node.endswith(".scicore.unibas.ch") or node.endswith(".cluster.bc2.ch")


REPO = "/home/kellert/workspace/planner/prost/prost-dev/"
REV = "9758a9bb"
ENV = LocalEnvironment(processes=4)
SUITES = [ELEVATORS_2011[0]]
NUM_RUNS = 1
if is_running_on_cluster():
    ENV = BaselSlurmEnvironment(partition="infai_2", email="tho.keller@unibas.ch")
    SUITES = IPC_ALL
    NUM_RUNS = 30

exp = ProstExperiment(suites=SUITES, environment=ENV, num_runs=NUM_RUNS)

if is_running_on_cluster():
    exp.add_config("ipc11", REPO, REV, "IPC2011", build_options=["-j6"])
exp.add_config("ipc14", REPO, REV, "IPC2014", build_options=["-j6"])

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

exp.run_steps()
