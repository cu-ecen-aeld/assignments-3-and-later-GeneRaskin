#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Error: two arguments are required: <writefile> <writestring>"
	exit 1
fi

writefile=$1
writestring=$2

mkdir -p "$(dirname "$writefile")"

if echo "$writestring" > "$writefile"; then
	exit 0
fi

echo "Error: Could not create or write to the file"
exit 1