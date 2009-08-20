#!/bin/sh

echo "Removing old database test.sq3..."
rm -f test.sq3
echo "Creating new empty database test.sq3..."
sqlite3 test.sq3 < db_generator.sql
echo "Loading sample data into the database..."
sqlite3 test.sq3 < db_sample.sql
