# Experiments using Prost Lab

Information on how to setup Prost Lab can be found in the [Prost
wiki](https://github.com/prost-planner/prost/wiki/Evaluation).

We recommend to create a subdirectory here for your own experiments,
where you create your own experiment script and where Prost Lab will
store the experiment data.

If you plan to perform experiments on the IPC benchmarks, we recommend
to copy the file `issue-83/ipc_benchmarks.py` to your subdirectory.

A good place to start is the experiment script that can be found at
`issue-83/exp1.py` if you compare different configurations within the
same revision and `issue-106/v1.py` if you compare the same
configuration between different revisions.

If you want to compare the output of the parser over all instances, you
can find a script that runs the parser on all IPC instances at
`issue-102/run-rddl-parser.py`.