#!/bin/bash

$1 -b $2 -p $3 -s $4 --separate-session -t $5 -l ./ -r $6 > ./server.log 2> ./server.err & sleep 10 && $7 $8 -p $3 --parser-options "-ipc2018 $9" [Prost ${10} -se [${11}]]
