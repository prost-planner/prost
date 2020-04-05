#! /usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Prost Lab uses the Lab package to conduct experiments with the
# PROST probabilistic planning system.
#

"""
Parse runs of PROST.
"""

import re

from lab.parser import Parser

# List of patterns that occurr only once.
SINGLE_PATTERNS = [
    ("total_time", "PROST complete running time: (.+)\n", float),
    ("parser_time", "PROST parser complete running time: (.+)\n", float),
    ("total_reward", ">>> END OF SESSION  -- TOTAL REWARD: (.+)\n", float),
    ("average_reward", ">>> END OF SESSION  -- AVERAGE REWARD: (.+)\n", float),
    (
        "ids_learned_search_depth",
        "THTS heuristic IDS: Setting max search depth to: (.+)\n",
        int,
    ),
]

# List of repeated patterns (list attributes)
REPEATED_PATTERNS = [
    ("round_reward", ">>> END OF ROUND .* -- REWARD RECEIVED: (.+)\n", float),
    (
        "entries_prob_state_value_cache",
        "Entries in probabilistic state value cache: (.+)\n",
        int,
    ),
    (
        "buckets_prob_state_value_cache",
        "Buckets in probabilistic state value cache: (.+)\n",
        int,
    ),
    (
        "entries_prob_applicable_actions_cache",
        "Entries in probabilistic applicable actions cache: (.+)\n",
        int,
    ),
    (
        "buckets_prob_applicable_actions_cache",
        "Buckets in probabilistic applicable actions cache: (.+)\n",
        int,
    ),
    (
        "rem_steps_first_solved_state",
        "Number of remaining steps in first solved state: (.+)\n",
        int,
    ),
    ("trial_initial_state", "Number of trials in initial state: (.+)\n", int),
    (
        "search_nodes_initial_state",
        "Number of search nodes in initial state: (.+)\n",
        int,
    ),
    (
        "perc_exploration_initial_state",
        "Percentage exploration in initial state: (.+)\n",
        float,
    ),
    (
        "entries_det_state_value_cache",
        "Entries in deterministic state value cache: (.+)\n",
        int,
    ),
    (
        "buckets_det_state_value_cache",
        "Buckets in deterministic state value cache: (.+)\n",
        int,
    ),
    (
        "entries_det_applicable_actions_cache",
        "Entries in deterministic applicable actions cache: (.+)\n",
        int,
    ),
    (
        "buckets_det_applicable_actions_cache",
        "Buckets in deterministic applicable actions cache: (.+)\n",
        int,
    ),
    ("entries_ids_reward_cache", "Entries in IDS reward cache: (.+)\n", int),
    ("buckets_ids_reward_cache", "Buckets in IDS reward cache: (.+)\n", int),
    (
        "ids_avg_search_depth_initial_state",
        "Average search depth in initial state: (.+)\n",
        float,
    ),
    ("ids_total_num_runs", "Total number of runs: (.+)\n", int),
    ("ids_avg_search_depth_total", "Total average search depth: (.+)\n", float),
]


def _get_flags(flags_string):
    flags = 0
    for char in flags_string:
        flags |= getattr(re, char)
    return flags


def add_repeated_pattern(parser, name, regex, file="run.log", type=int, flags="M"):
    """
    *regex* must contain at most one group.
    """
    flags = _get_flags(flags)

    def find_all_occurences(content, props):
        matches = re.findall(regex, content, flags=flags)
        props[name] = [type(m) for m in matches]

    parser.add_function(find_all_occurences, file=file)


print("Running Prost parser.")

parser = Parser()
for name, pattern, pattern_type in SINGLE_PATTERNS:
    parser.add_pattern(name, pattern, type=pattern_type)
for name, pattern, pattern_type in REPEATED_PATTERNS:
    add_repeated_pattern(parser, name, pattern, type=pattern_type)
parser.parse()
