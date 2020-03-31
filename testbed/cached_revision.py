# -*- coding: utf-8 -*-
#
# Prost Lab uses the Lab package to conduct experiments with the
# Prost planning system.
#

import glob
import os.path
import subprocess

from lab import tools
from lab.cached_revision import CachedRevision


class CachedProstRevision(CachedRevision):
    """This class represents Prost checkouts.

    It provides methods for caching and compiling given revisions.
    """

    def __init__(self, repo, local_rev, build_options):
        """
        * *repo*: Path to Prost repository.
        * *local_rev*: Prost revision.
        * *build_options*: List of build.py options.
        """
        super().__init__(
            repo, local_rev, "build.py", build_options, ["scripts"]
        )

    def get_planner_resource_name(self):
        return "prost_" + self._hashed_name

    def get_server_resource_name(self):
        return "rddlsim_" + self._hashed_name

    def get_benchmark_dir_name(self):
        return "benchmarks_" + self._hashed_name

    def get_wrapper_resource_name(self):
        return "wrapper_" + self._hashed_name

    def _cleanup(self):
        # Only keep the binaries
        #for path in glob.glob(os.path.join(self.path, "builds", "*", "*")):
        #    if os.path.basename(path) not in ["rddl-parser", "search"]:
        #        tools.remove_path(path)

        # Remove unneeded files.
        #tools.remove_path(self.get_cached_path("build.py"))

        # Strip binaries.
        #binaries = []
        #for path in glob.glob(os.path.join(self.path, "builds", "*", "*")):
        #    if os.path.basename(path) in ["rddl-parser", "search"]:
        #        binaries.append(path)
        #subprocess.call(["strip"] + binaries)

        # Compress src directory.
        subprocess.call(
            ["tar", "-cf", "src.tar", "--remove-files", "src"], cwd=self.path
        )
        subprocess.call(["xz", "src.tar"], cwd=self.path)
