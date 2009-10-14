#!/bin/sh
./waf distclean
scan-build -o html ./waf configure
scan-build -o html ./waf build
