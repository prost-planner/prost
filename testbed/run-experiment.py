#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import platform

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from prostlab.experiment import ProstExperiment

from benchmark_suites import *

# TODO: Think about these:
from ipc_scores import IPCScores
from lab.reports import Attribute

from prostlab.reports.absolute import AbsoluteProstReport


def is_running_on_cluster():
    node = platform.node()
    return node.endswith(".scicore.unibas.ch") or node.endswith(".cluster.bc2.ch")

REPO = "/home/kellert/workspace/planner/prost/prost-dev/"
REV = "HEAD"

TIME_PER_STEP = 1.0
if is_running_on_cluster():
    ENV = BaselSlurmEnvironment(partition="infai_2", email="tho.keller@unibas.ch")
    SUITES = IPC_ALL
    NUM_RUNS = 30
    
else:
    ENV = LocalEnvironment(processes=4)
    SUITES = [ELEVATORS_2011[0], NAVIGATION_2011[0]]
    NUM_RUNS = 1

exp = ProstExperiment(suites=SUITES, environment=ENV, num_runs=NUM_RUNS, time_per_step=TIME_PER_STEP)

exp.add_algorithm("ipc11", REPO, REV, "IPC2011", build_options=["-j6"])
exp.add_algorithm("ipc14", REPO, REV, "IPC2014", build_options=["-j6"])

exp.add_parser(ProstExperiment.PROST_PARSER)
exp.add_parser(ProstExperiment.THTS_PARSER)
exp.add_parser(ProstExperiment.IDS_PARSER)

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

#exp.add_parse_again_step()

# TODO: Think about where to get these from
# Attributes to be displayed in the report.
ATTRIBUTES = [
    "ipc_score",
    "round_reward",
    "total_reward",
    "average_reward",
    "total_time",
    "parser_time",
    "trial_initial_state",
    "search_nodes_initial_state",
    Attribute("entries_prob_state_value_cache", min_wins=False),
    Attribute("buckets_prob_state_value_cache", min_wins=False),
    Attribute("entries_prob_applicable_actions_cache", min_wins=False),
    Attribute("buckets_prob_applicable_actions_cache", min_wins=False),
    Attribute("entries_det_state_value_cache", min_wins=False),
    Attribute("buckets_det_state_value_cache", min_wins=False),
    Attribute("entries_det_applicable_actions_cache", min_wins=False),
    Attribute("buckets_det_applicable_actions_cache", min_wins=False),
    Attribute("entries_ids_reward_cache", min_wins=False),
    Attribute("buckets_ids_reward_cache", min_wins=False),
    Attribute("ids_avg_search_depth_initial_state", min_wins=False),
    Attribute("ids_avg_search_depth_total", min_wins=False),
    Attribute("run_dir"),
]

ipc_scores = IPCScores()

exp.add_report(
    AbsoluteProstReport(
        attributes=ATTRIBUTES, filter=[ipc_scores.store_rewards, ipc_scores.add_score]
    ),
    outfile="report.html",
)


exp.run_steps()
