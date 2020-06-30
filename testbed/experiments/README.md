# Experiments using Prost Lab

Information on how to setup Prost Lab can be found in the [Prost
wiki](https://github.com/prost-planner/prost/wiki/Evaluation).

We recommend to create a subdirectory here for your own experiments,
where you create your own experiment script and where Prost Lab will
store the experiment data.

If you plan to perform experiments on the IPC benchmarks, we recommend
to copy the file `issue-83/ipc_benchmarks.py` to your subdirectory.

A good place to start is the experiment script that can be found at
`issue-83/exp1.py`. It performs an experiment with two configurations of
Prost, uses all default parsers and creates two absolute reports, one
with all attributes parsed by one of the default parsers, and one with a
pre-specified subset of all attributes.