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

    def __init__(self, repo, rev, build_options):
        """
        * *repo*: Path to Prost repository.
        * *rev*: Prost revision.
        * *build_options*: List of build.py options.
        """
        super().__init__(repo, rev, ["build.py"] + build_options, ["scripts"])

    def _cleanup(self):
        # Strip binaries.
        binaries = []
        for path in glob.glob(os.path.join(self.path, "builds", "*", "*", "*")):
            if os.path.basename(path) in ["rddl-parser", "search"]:
                binaries.append(path)
        subprocess.call(["strip"] + binaries)

        # Compress src directory.
        subprocess.call(
            ["tar", "-cf", "src.tar", "--remove-files", "src"], cwd=self.path
        )
        subprocess.call(["xz", "src.tar"], cwd=self.path)
