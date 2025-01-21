#!/bin/sh

if [ $# -ne 2 ]; then
  echo "Error: two arguments are required: <filesdir> <searchdir>"
  exit 1
fi

filesdir=$1
searchdir=$2

if [ ! -d "$filesdir" ]; then
	echo "Error: $filesdir is not a valid directory"
	exit 1
fi

numfiles=$(find "$filesdir" -type f | wc -l)
num_matching_lines=$(grep -r "$searchdir" "$filesdir" 2>/dev/null | wc -l)

echo "The number of files are $numfiles and the number of matching lines are $num_matching_lines"
exit 0

