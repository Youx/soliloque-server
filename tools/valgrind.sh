#!/bin/sh
./waf build
valgrind --leak-check=yes --show-reachable=yes --track-origins=yes ./output/default/soliloque-server
