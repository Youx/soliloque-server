#!/bin/sh

rm -f test.sq3
sqlite3 test.sq3 < db_generator.sql
sqlite3 test.sq3 < db_sample.sql
