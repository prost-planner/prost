echo "rm -f ~/Desktop/elevators_inst_mdp__2 ~/Desktop/crossing_traffic_inst_mdp__2"
rm -f ~/Desktop/elevators_inst_mdp__2 ~/Desktop/crossing_traffic_inst_mdp__2

echo "Running elevators_inst_mdp__2 test and checking results with old test."
./rddl-parser ../../testbed/benchmarks/ippc-all/rddl/elevators_mdp.rddl ../../testbed/benchmarks/ippc-all/rddl/elevators_inst_mdp__2.rddl ~/Desktop
meld ~/Desktop/elevators_inst_mdp__2 ~/projects/prost/testbed/benchmarks/ippc-all/prost/elevators_inst_mdp__2

echo "Running crossing_traffic_inst_mdp__2 test and checking results with old test."
./rddl-parser ../../testbed/benchmarks/ippc-all/rddl/crossing_traffic_mdp.rddl ../../testbed/benchmarks/ippc-all/rddl/crossing_traffic_inst_mdp__2.rddl ~/Desktop
meld ~/Desktop/crossing_traffic_inst_mdp__2 ~/projects/prost/testbed/benchmarks/ippc-all/prost/crossing_traffic_inst_mdp__2