#!/bin/bash

#rm -f bin/*

config.sh L1D_num_ways_16_config.json
make

config.sh L1D_num_ways_32_config.json
make

config.sh L1D_num_ways_64_config.json
make

config.sh L1D_num_ways_128_config.json
make
