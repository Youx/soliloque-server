#!/bin/sh

usage() {
	echo "Generate a basic sqlite3 database for soliloque-server"
	echo "./gen_db.sh <output_file>"
	exit 0
}

generate() {
	file=$1
	echo "Removing old database $file..."
	rm -f $file
	echo "Creating new empty database..."
	sqlite3 $file < db_generator.sql
	echo "Loading sample data into the database..."
	sqlite3 $file < db_sample.sql
	echo "Done. Don't forget to update your configuration file."
}

if [ -z $1 ]
then
	usage
fi

if [ $1 = "--help" ]
then
	usage
fi

generate $1
