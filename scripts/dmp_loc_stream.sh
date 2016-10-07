#!/bin/sh

# Usage ./dmp_loc_stream.sh "-73.98,-33.75,-29.34,5.27" "Brazil

while true
do
  output=$2_$(date +%F_%Hh%M).json
  echo $output
  timeout 1h python ./getStreams.py -e statuses/filter -p locations=$1 >> $output
done
