#! /usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Prost Lab uses the Lab package to conduct experiments with the
# PROST probabilistic planning system.
#

'''
Parse runs of PROST.
'''

import re

from lab.parser import Parser

# List of patterns that occurr only once.
SINGLE_PATTERNS = [
    ('time', r'PROST complete running time: (\d+\.?\d+)s\n', float),
    ('total_reward', r'>>> .* TOTAL REWARD: (-?\d+\.?\d+)\n', float),
    ('average_reward', r'>>> .* AVERAGE REWARD: (-?\d+\.?\d+)\n', float),
]

# List of repeated patterns (list attributes)
REPEATED_PATTERNS = [
    ('round_reward-all', r'>>> END OF ROUND .* -- REWARD RECEIVED: (-?\d+\.?\d*)\n', float),
]

# List of repeated patterns that should be splitted.
# They must have suffix '-all' in their names.
SPLITTED_PATTERNS = [
    'round_reward-all',
]

# List of attributes which are list of lists
LIST_PATTERNS = [
    ('reward_step-all', r'Round .*: (.+) = .*$'),
]

def _get_flags(flags_string):
    flags = 0
    for char in flags_string:
        flags |= getattr(re, char)
    return flags


def add_repeated_pattern(self, name, regex, file='run.log', type=int, flags='M'):
    '''
    *regex* must contain at most one group.
    '''
    flags = _get_flags(flags)

    def find_all_occurences(content, props):
        matches = re.findall(regex, content, flags=flags)
        match_list = [type(m) for m in matches]
        props[name] = match_list
        if name in SPLITTED_PATTERNS:
            new_name = name.replace('-all', '_')
            for cont, value in enumerate(match_list):
                props[new_name+str(cont)] = value

    parser.add_function(find_all_occurences, file=file)

def reduce_to_min(list_name, single_name):
    def reduce_to_minimum(content, props):
        values = props.get(list_name, [])
        if values:
            props[single_name] = min(values)

    return reduce_to_minimum

def add_list_pattern(parser, name, regex, file='run.log', flags='M'):
    '''
    *regex* must contain at most one group.
    '''
    flags = _get_flags(flags)

    def parse_repeated_list(content, props):
        matches = re.findall(regex, content, flags=flags)
        attr_list = []
        for m in matches:
            items = m.split()
            items = [float(i) for i in items]
            attr_list.append(items)
        props[name] = attr_list

    parser.add_function(parse_repeated_list, file=file)

if __name__ == '__main__':
    print 'Running Prost parser.'
    parser = Parser()
    for name, pattern, pattern_type in SINGLE_PATTERNS:
        parser.add_pattern(name, pattern, type=pattern_type)
    for name, pattern, pattern_type in REPEATED_PATTERNS:
        add_repeated_pattern(parser, name, pattern, type=pattern_type)
    for name, pattern in LIST_PATTERNS:
        add_list_pattern(parser, name, pattern)
    parser.parse()
