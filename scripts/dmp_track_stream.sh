#!/bin/sh

#usage ./dmp_track_stream.sh "pizza" "filename"

while true
do
  output=$2_$(date +%F_%Hh%M).json
  stderr=$2_$(date +%F_%Hh%M)_err.json
  echo $output
  timeout 1h python ./getStreams.py -e statuses/filter -p track="$1" >> $output 2>> $stderr
done
