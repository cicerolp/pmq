#!/bin/bash
ls -htl *tgz

tar xvzf log_1503497835.tgz

logFile='bench_insert_and_scan_1503497835.log
'
#echo $logFile
echo $(basename -s .log $logFile ).csv
grep "GeoHashBinary\|BTree\|RTree ;" $logFile | sed "s/InsertionBench//g" >  $(basename -s .log $logFile ).csv
