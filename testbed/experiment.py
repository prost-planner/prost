# -*- coding: utf-8 -*-
#
# Prost Lab uses the Lab package to conduct experiments with the
# Prost planning system.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
A module for running Prost experiments.
"""
import os

from collections import defaultdict, OrderedDict

from cached_revision import CachedProstRevision
from lab import tools
from lab.experiment import Experiment, get_default_data_dir, Run

from benchmark_suites import *

class ProstRun(Run):
    def __init__(self, exp, config, task, port, num_runs, rddlsim_seed, rddlsim_run_time, prost_seed, memory_limit):
        Run.__init__(self, exp)
        self.config = config
        self.task = task
        self.port = port
        self.num_runs = num_runs
        self.rddlsim_seed = rddlsim_seed
        self.rddlsim_run_time = rddlsim_run_time
        self.prost_seed = prost_seed
        self.memory_limit = memory_limit

        self._set_properties()

        # Linking to instead of copying the PDDL files makes building
        # the experiment twice as fast.
        #self.add_resource("domain", self.task.domain_file, "domain.pddl", symlink=True)
        #self.add_resource(
        #    "problem", self.task.problem_file, "problem.pddl", symlink=True
        #)

        use_ipc2018_parser = int(task.path.endswith("2018"))

        self.add_command(
            "planner",
            ["{" + config.cached_revision.get_wrapper_resource_name() + "}",
             "{" + config.cached_revision.get_server_resource_name() + "}",
             "{" + config.cached_revision.get_benchmark_dir_name() + "}/" + self.task.path,
             str(self.port),
             str(self.rddlsim_seed),
             str(self.rddlsim_run_time),
             "{" + config.cached_revision.get_planner_resource_name() + "}",
             task.problem.replace('.rddl',''),
             str(use_ipc2018_parser),
             self.prost_seed,
             self.memory_limit,
             config.search_engine_desc]
        )

        # self.add_command(
        #     "server",
        #     ["{" + config.cached_revision.get_server_resource_name() + "}", "-b",
        #      "{" + config.cached_revision.get_benchmark_dir_name() + "}/" + self.task.path,
        #      "-p", str(self.port), "-r", str(self.num_runs),
        #      "-s", str(self.server_seed), "--separate-session",
        #      "-t", str(self.server_run_time), "-l", "./"]
        # )
        
        # self.add_command(
        #     "planner",
        #     [tools.get_python_executable(),
        #      "{" + config.cached_revision.get_planner_resource_name() + "}",
        #      task.problem.replace('.rddl',''), "-p", str(self.port),
        #      "--parser-options", '"-ipc2018"', str(use_ipc2018_parser),
        #      "[PROST", "-s", "1", "-ram", "TODO", "-se",
        #      "[" + config.search_engine_desc + "]]"]
        # )

    def _set_properties(self):
#         self.set_property("algorithm", self.algo.name)
#         self.set_property("repo", self.algo.cached_revision.repo)
#         self.set_property("local_revision", self.algo.cached_revision.local_rev)
#         self.set_property("global_revision", self.algo.cached_revision.global_rev)
#         self.set_property("revision_summary", self.algo.cached_revision.summary)
#         self.set_property("build_options", self.algo.cached_revision.build_options)
#         self.set_property("driver_options", self.algo.driver_options)
#         self.set_property("component_options", self.algo.component_options)

#         for key, value in self.task.properties.items():
#             self.set_property(key, value)

#         self.set_property("experiment_name", self.experiment.name)

        self.set_property("id", [self.config.name, self.task.domain, self.task.problem])


class ProstConfig(object):
    def __init__(self, name, cached_revision, search_engine_desc):
        self.name = name
        self.cached_revision = cached_revision
        self.search_engine_desc = search_engine_desc

    def __eq__(self, other):
        """Return true iff all components (excluding the name) match."""
        return (
            self.cached_revision == other.cached_revision
            and self.search_engine_desc == other.search_engine_desc
        )


class ProstExperiment(Experiment):
    """Conduct a Prost experiment.

    The most important methods for customizing an experiment are
    :meth:`.add_algorithm`, :meth:`.add_suite`, :meth:`.add_parser`,
    :meth:`.add_step` and :meth:`.add_report`.

    .. note::

        To build the experiment, execute its runs and fetch the results,
        add the following steps (previous Lab versions added these steps
        automatically):

        >>> exp = ProstExperiment(suites=IPC2014)
        >>> exp.add_step('build', exp.build)
        >>> exp.add_step('start', exp.start_runs)
        >>> exp.add_fetcher(name='fetch')

    """
    def __init__(self, suites=IPC_ALL, num_runs=30, time_per_step=1.0, port=2000, rddlsim_seed=0, rddlsim_enforces_runtime=False, prost_seed=0, memory_limit="3584M", path=None, environment=None, revision_cache=None):
        """
        See :class:`lab.experiment.Experiment` for an explanation of
        the *path* and *environment* parameters.

        *revision_cache* is the directory for caching Prost
        revisions. It defaults to ``<scriptdir>/data/revision-cache``.
        This directory can become very large since each revision uses
        about 30 MB.

        >>> from lab.environments import BaselSlurmEnvironment
        >>> env = BaselSlurmEnvironment(email="my.name@unibas.ch")
        >>> exp = ProstExperiment(environment=env)

        You can add parsers with :meth:`.add_parser()`. See
        :ref:`parsing` for how to write custom parsers.

        """
        Experiment.__init__(self, path=path, environment=environment)

        self.revision_cache = revision_cache or os.path.join(
            get_default_data_dir(), "revision-cache"
        )
        
        self.suites = suites
        self.num_runs = num_runs
        self.time_per_step=time_per_step
        self.port = port
        self.rddlsim_seed = rddlsim_seed
        self.rddlsim_enforces_runtime = rddlsim_enforces_runtime
        self.prost_seed = prost_seed
        self.memory_limit = memory_limit

        # Use OrderedDict to ensure that names are unique and ordered.
        self.configs = OrderedDict()

    def add_config(
        self,
        name,
        repo,
        rev,
        search_engine_desc,
        build_options=None,
    ):
        """Add a Prost algorithm to the experiment, i.e., a
        planner configuration in a given repository at a given
        revision.

        *name* is a string describing the algorithm (e.g. ``"ipc14"``).

        *repo* must be a path to a Prost repository.

        *rev* must be a valid revision in the given repository (e.g.,
        ``"master"``, ``"HEAD~3"``, ``"issue94"``).

        *search_engine_desc* must be a string describing the used search engine.

        If given, *build_options* must be a list of strings. They will
        be passed to the ``build.py`` script. Options can be build names
        (``"release"`` or ``"debug"``), ``build.py`` options (e.g.,
        ``"--debug"``) or options for Make. If *build_options* is
        omitted, the ``"release"`` version is built.

        Example experiment setup to test Prost2011 in the latest revision 
        on the master branch:

        >>> import os
        >>> from lab.cached_revision import get_version_control_system, MERCURIAL
        >>> exp = ProstExperiment()
        >>> repo = os.environ["PROST_REPO"]
        >>> rev = "master"
        >>> exp.add_config("ipc11", repo, rev, "IPC2011")

        """
        if not isinstance(name, tools.string_type):
            logging.critical("Config name must be a string: {}".format(name))
        if name in self.configs:
            logging.critical("Config names must be unique: {}".format(name))
        build_options = build_options or []
        config = ProstConfig(
            name,
            CachedProstRevision(repo, rev, build_options),
            search_engine_desc,
        )

        print("{}: {}".format(config.name, config.search_engine_desc))
        for conf in self.configs.values():
            if config == conf:
                logging.critical(
                    "Configs {conf.name} and {config.name} are "
                    "identical.".format(**locals())
                )
        self.configs[name] = config

    def build(self, **kwargs):
        """Add Prost code, runs and write everything to disk.

        This method is called by the second experiment step.

        """
        if not self.configs:
            logging.critical("You must add at least one config.")

        # We convert the problems in suites to strings to avoid errors when converting
        # properties to JSON later. The clean but more complex solution would be to add
        # a method to the JSONEncoder that recognizes and correctly serializes the class
        # Problem.
        #serialized_suites = {
        #    benchmarks_dir: [str(problem) for problem in benchmarks]
        #    for benchmarks_dir, benchmarks in self.suites
        #}
        #self.set_property("suite", serialized_suites)
        self.set_property("configs", list(self.configs.keys()))

        self._cache_revisions()
        self._add_code()
        self._add_runs()

        Experiment.build(self, **kwargs)

    def _get_unique_cached_revisions(self):
        unique_cached_revs = set()
        for config in self.configs.values():
            unique_cached_revs.add(config.cached_revision)
        return unique_cached_revs

    def _cache_revisions(self):
        for cached_rev in self._get_unique_cached_revisions():
            cached_rev.cache(self.revision_cache)

    def _add_code(self):
        """Add the compiled code to the experiment."""
        for cached_rev in self._get_unique_cached_revisions():
            self.add_resource(
                "", cached_rev.get_cached_path(), cached_rev.get_exp_path()
            )
            # Overwrite the script to set an environment variable.
            self.add_resource(
                cached_rev.get_planner_resource_name(),
                cached_rev.get_cached_path("prost.py"),
                cached_rev.get_exp_path("prost.py"),
            )
            self.add_resource(
                cached_rev.get_server_resource_name(),
                cached_rev.get_cached_path("testbed", "run-server.py"),
                cached_rev.get_exp_path("testbed", "run-server.py"),
            )
            self.add_resource(
                cached_rev.get_benchmark_dir_name(),
                cached_rev.get_cached_path("testbed", "benchmarks"),
                cached_rev.get_exp_path("testbed", "benchmarks"),
            )
            self.add_resource(
                cached_rev.get_wrapper_resource_name(),
                cached_rev.get_cached_path("testbed", "wrapper.sh"),
                cached_rev.get_exp_path("testbed", "wrapper.sh"),
            )

    def _add_runs(self):
        # The memory limit in kb
        if self.memory_limit[-1] == "M":
            memout_kb = int(self.memory_limit[:-1]) * 1024
        elif self.memory_limit[-1] == "G":
            memout_kb = int(self.memory_limit[:-1]) * 1024 * 1024
        else:
            logging.critical("Memory limit must be given in MB or GB.")

        
        port = self.port
        for config in self.configs.values():
            for task in self.suites:
                rddlsim_run_time = 0
                if self.rddlsim_enforces_runtime:
                    rddlsim_run_time = int(task.horizon * self.num_runs * self.time_per_step)
                self.add_run(ProstRun(self, config, task, port, self.num_runs, self.rddlsim_seed, rddlsim_run_time, self.prost_seed, memout_kb))
                port += 1
