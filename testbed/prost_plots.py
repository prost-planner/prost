
import json
import os
import sys

import matplotlib.lines as mlines
import matplotlib.pyplot as plt

import numpy as np

from collections import defaultdict
from itertools import product


class ListPlot(object):
    def __init__(self, path, filters=[]):
        self.eval_path = path + '-eval/'
        with open(self.eval_path+'properties') as json_file:
            self.properties = json.load(json_file)
        for f in filters:
            assert callable(f)
            self.properties = self._apply_filter(properties, f)

    def _apply_filter(self, properties, f):
        new_properties = dict()
        for run in properties:
            if filter_func(properties[run]):
                new_properties[run] = properties[run]
        return new_properties

    def plot_list_attribute(self, lst, attribute_name, outfile=None):
        # First, we collect all tuples of attributes that we want to match and
        # keep the style of every attribute match.
        values_per_attr = defaultdict(set)
        map_to_style = dict()
        for l in lst:
            assert isinstance(l, PlotAttribute)
            map_to_style[(l.attribute, l.value)] = l
            values_per_attr[l.attribute].add(l.value)

        # Then, we compute the cartesian product over all possible attribute
        # values.
        matches = [dict(zip(values_per_attr, x)) for x in product(*values_per_attr.values())]


        # We set up some properties of the plot to become more similar to the
        # Lab plots
        plt.rc('font', family='serif')

        for key, run in self.properties.iteritems():
            match, t = self._get_matching_tuple(run, matches)
            if match:
                color, marker, linestyle = self.retrieve_style(run, t, map_to_style)
                # We might overwrite the key several times if the attribute
                # styles is ill-defined.
                legend = '-'.join([v for k, v in t.iteritems()])
                assert isinstance(run[attribute_name], list)
                y = run[attribute_name]
                x = np.arange(len(y))
                plt.plot(x, y,
                         c=color, marker=marker, linestyle=linestyle,
                         label=legend)

        # Add legend
        leg = plt.legend(loc='best', handlelength=3.5)

        plt.tight_layout()
        if outfile == None:
            if leg:
                leg.draggable()
            plt.show()
        else:
            assert isinstance(outfile, str)
            plt.savefig(outfile)


    def _get_matching_tuple(self, run, matches):
        for m in matches:
            match = True
            for k, v in m.iteritems():
                if v != run[k]:
                    match = False
                    break
            if match:
                return True, m
        return False, None

    def retrieve_style(self, run, t, map_to_style):
        color = marker = linestyle = None
        # We get the first value for each of the variables.  We do not check for
        # consistency
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = map_to_style[k]
            color = style.color
            if color != None:
                break
        if color == None:
            # If no attribute defined the color, we set it to black
            color = 'black'
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = map_to_style[k]
            marker = style.marker
            if marker != None:
                break
        if marker == None:
            # If no attribute defined the marker, we set it to a circle
            marker = 'o'
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = map_to_style[k]
            linestyle = style.linestyle
            if linestyle != None:
                break
        if linestyle == None:
            # If no attribute defined the linestyle, we set it to a solid line
            linestyle = '-'
        return color, marker, linestyle


class PlotAttribute(object):
    def __init__(self, attribute, value, color=None, marker=None, linestyle=None):
        self.attribute = attribute
        self.value = value
        self.color = color
        self.marker = marker
        self.linestyle = linestyle

    def __str__(self):
        return "attr: {}, value: {}, color: {}, marker: {}, linestyle: {}".format(
            self.attribute, self.value, self.color, self.marker, self.linestyle)


class PlotDomain(PlotAttribute):
    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, 'domain', value, color, marker, linestyle)


class PlotProblem(PlotAttribute):
    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, 'problem', value, color, marker, linestyle)


class PlotAlgorithm(PlotAttribute):
    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, 'algorithm', value, color, marker, linestyle)
