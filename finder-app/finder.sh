#!/bin/sh

# Finder script for assignment 1.

# Check number of arguments
if [ $# -ne 2 ]; then
  echo "Error: Invalid number of arguments"
  echo "Usage: $0 <filesdir> <searchstr>"
  exit 1
fi

# Variables
filesdir=$1
searchstr=$2

# Check if the arguments are not empty
if [ -z ${filesdir} ] || [ -z ${searchstr} ]; then
    echo "Usage: $0 <directory> <search_string>"
    exit 1
fi

# Check if the directory exists
if [ ! -d "${filesdir}" ]; then
    echo "Directory ${filesdir} does not exist."
    exit 1
fi

# Count the number of files in the directory
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of lines matching the search string in the files
match_count=$( grep -r "${searchstr}" ${filesdir} | wc -l )
echo "The number of files are ${file_count} and the number of matching lines are ${match_count}"