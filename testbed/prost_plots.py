"""This module contains all classes necessary to plot list attributes
using Matplotlib.


The following classes are defined here:

   - ListPlot: Plots a given attribute based on a dataset and a list of filters
     provided by the user.  It can save the plot directly to an output file or
     show it in a new window to the user.
   - PlotAttribute: A class defining a specific style (w.r.t. markers, color,
     and line style) for an attribute that the user wants to plot.  The
     attribute is provided by the user.  We say that such objects are filters.
   - PlotDomain, PlotProblem, and PlotAlgorithm: Shortcuts to create and
     PlotAttribute object with a pre-specified attribute

NOTES:

   - There is no consistency check between style of different filters.  For
     example, you can pass a filter PlotDomain to plot a given domains D with
     blue color as parameter, and then also pass another filter PlotProblem for
     a problem P with red color.  When plotting the results for problem P of
     domain D with both filters, the color used is ill-defined.  We expect the
     user to handle the styles properly.
   - If the legend of the plot or the size of the axis is not as wished by the
     user, you can plot without an output file and manage the sizes and the
     legend manually on the TK window displayed by Python.  Note that the legend
     is always draggable in this case.

"""

import json
import os

from collections import defaultdict
from itertools import product

import matplotlib.pyplot as plt
import numpy as np


def get_matching_tuple(run, matches):
    """Checks which combination of attribute and values matches with the run.

    """
    for m in matches:
        match = True
        for k, v in m.iteritems():
            if v != run[k]:
                match = False
                break
        if match:
            return True, m
    return False, None


class ListPlot(object):
    """Handles the plot of an attribute in list format.

    """

    def __init__(self, path, filters=None):
        self.eval_path = self._cleanup_path(path) + "-eval/"
        if not os.path.exists(self.eval_path + "properties"):
            # We do not want to throw and error if the properties file does not
            # exist yet.
            print(
                "ATTENTION: Properties file does not exist yet.",
                "Running any Prost plot will result in an error.",
                "Please run the other steps first and then rerun the Prost plot steps.",
            )
        else:
            with open(self.eval_path + "properties") as json_file:
                self.properties = json.load(json_file)
        if filters is None:
            filters = []
        for f in filters:
            assert callable(f)
            self.properties = self._apply_filter(self.properties, f)

    def _apply_filter(self, properties, filter_func):
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
        matches = [
            dict(zip(values_per_attr, x)) for x in product(*values_per_attr.values())
        ]

        # We set up some properties of the plot to become more similar to the
        # Lab plots
        plt.rc("font", family="serif")

        # Loop over all the runs, verify which attributes and values are
        # matching with the ones we want to plot.  Then, we retrieve the style
        # matching these values and finally we plot the list as a line.
        for _, run in self.properties.iteritems():
            match, t = get_matching_tuple(run, matches)
            if match:
                color, marker, linestyle = self._retrieve_style(t, map_to_style)
                # We might overwrite the key several times if the attribute
                # styles is ill-defined.
                legend = "-".join([v for _, v in t.iteritems()])
                assert isinstance(run[attribute_name], list)
                y = run[attribute_name]
                x = np.arange(len(y))
                plt.plot(
                    x, y, c=color, marker=marker, linestyle=linestyle, label=legend
                )

        # Add legend
        leg = plt.legend(loc="best", handlelength=3.5)

        plt.tight_layout()
        if outfile is None:
            if leg:
                leg.draggable()
            plt.show()
        else:
            assert isinstance(outfile, str)
            plt.savefig(self.eval_path + outfile)

    def _retrieve_style(self, t, map_to_style):
        color = marker = linestyle = None
        # We get the first value for each of the variables.  We do not check for
        # consistency
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = v
            color = style.color
            if color is not None:
                break
        if color is None:
            # If no attribute defined the color, we set it to black
            color = "black"
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = v
            marker = style.marker
            if marker is not None:
                break
        if marker is None:
            # If no attribute defined the marker, we set it to a circle
            marker = "o"
        for k, v in map_to_style.iteritems():
            if k[1] != t[k[0]]:
                continue
            style = v
            linestyle = style.linestyle
            if linestyle is not None:
                break
        if linestyle is None:
            # If no attribute defined the linestyle, we set it to a solid line
            linestyle = "-"
        return color, marker, linestyle

    def _cleanup_path(self, path):
        # Simply parses '/path/to/exp/' into '/path/to/exp'
        if path[-1] == "/":
            path = path[:-1]
        return path


# Right now, PlotAttribute and the other related classes are simply storing the
# data.  We should adjust it to implement auxiliary methods and reduce the code
# complexity of ListPlot.
class PlotAttribute(object):
    """
    Implements a filter for an attribute with a defined style for the plot.
    """

    def __init__(self, attribute, value, color=None, marker=None, linestyle=None):
        self.attribute = attribute
        self.value = value
        self.color = color
        self.marker = marker
        self.linestyle = linestyle

    def __str__(self):
        return "attr: {}, value: {}, color: {}, marker: {}, linestyle: {}".format(
            self.attribute, self.value, self.color, self.marker, self.linestyle
        )


class PlotDomain(PlotAttribute):
    """
    Implements a filter specifically over the 'domain' attribute.
    """

    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, "domain", value, color, marker, linestyle)


class PlotProblem(PlotAttribute):
    """
    Implements a filter specifically over the 'problem' (i.e., instance name) attribute.
    """

    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, "problem", value, color, marker, linestyle)


class PlotAlgorithm(PlotAttribute):
    """
    Implements a filter specifically over the 'algorithm' (i.e., configuration) attribute.
    """

    def __init__(self, value, color=None, marker=None, linestyle=None):
        PlotAttribute.__init__(self, "algorithm", value, color, marker, linestyle)
