#!/bin/sh

# Writer script for assignment 1.

# Check number of arguments
if [ $# -ne 2 ]; then
  echo "Error: Invalid number of arguments"
  echo "Usage: $0 <writefile> <writestr>"
  exit 1
fi

# Variables
writefile=$1
writestr=$2

# Check if the argumentos are not empty
if [ -z ${writefile} ] || [ -z ${writestr} ]; then
  echo "Usage: $0 <writefile> <writestr>"
  exit 1
fi

# Get the directory of the writefile
writedir=$(dirname "${writefile}")

# Crate the directory if it does not exist
if [ ! -d "${writedir}" ]; then
  mkdir -p "${writedir}"
  if [ $? -ne 0 ]; then
    echo "Error: Could not create directory ${writedir}"
    exit 1
  fi
fi

# Write the string to the file
echo "${writestr}" > "${writefile}"
if [ $? -ne 0 ]; then
  echo "Error: Could not write ${writestr} to file ${writefile}"
  exit 1
fi
