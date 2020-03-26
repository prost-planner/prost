#!/bin/bash

$1 -b $2 -p $3 -s $4 --separate-session -t $5 -l ./ > ./server.log 2> ./server.err & sleep 10 && $6 $7 -p $3 --parser-options "-ipc2018 $8" [Prost -s $9 -ram ${10} -se [${11}]]
