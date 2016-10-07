#!/bin/sh

while true
do
  output=$2_$(date +%F_%Hh%M).json
  echo $output
  timeout 1h python ./getStreams.py -e statuses/filter -p track="$1" >> $output
done
