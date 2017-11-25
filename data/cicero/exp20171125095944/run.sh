#!/bin/bash
expId='exp20171125095944'
set -e
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
echo $(hostname) 

##########################################################
### SETUP THIS VARIABLES

BUILDIR=~/Projects/pmq/build-release
PMABUILD_DIR=~/Projects/hppsimulations/build-release
DATADIR=$(pwd)
# workaround as :var arguments are not been correctly tangled by my orgmode
#expId=$(basename $(pwd) | sed 's/exp//g')
expId=$(basename $(pwd))
TMPDIR=/dev/shm/$expId

# generate output name
if [ $1 ] ; then 
    EXECID=$1
else
    EXECID=$(date +%s)
fi

#########################################################

mkdir -p $TMPDIR
#mkdir -p $DATADIR

# make pma
mkdir -p $PMABUILD_DIR
cd $PMABUILD_DIR
cmake -DCMAKE_BUILD_TYPE="Release" -DTWITTERVIS=ON -DRHO_INIT=OFF ../pma_cd
make 

# make twitterVis
mkdir -p $BUILDIR
cd $BUILDIR 
cmake -DPMA_BUILD_DIR=$PMABUILD_DIR -DCMAKE_BUILD_TYPE="Release" ..
make

#get machine configuration
echo "" > $DATADIR/info.org
~/Projects/pmq/scripts/g5k_get_info.sh $DATADIR/info.org 

# EXECUTE BENCHMARK

#Continue execution even if one these fails
set +e 
# Queries insert remove count
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 11745 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_11745_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 17616 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_17616_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 20552 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_20552_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 22020 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_22020_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 22754 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_22754_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 23121 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_23121_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 23305 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_23305_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 23396 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_23396_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 23442 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_23442_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count  -f ../data/geo-tweets.dat  -rate 1000 -n 46976000 -T 23465 -tSize 23488000 > ${TMPDIR}/bench_ins_rm_23465_${EXECID}.log


set -e

cd $TMPDIR
tar -cvzf log_$EXECID.tgz *_$EXECID.log

cd $DATADIR
cp $TMPDIR/log_$EXECID.tgz .

git checkout $expId

git add info.org log_$EXECID.tgz run.sh 
git add -u
git commit -m "Finish execution $EXECID"
#git push origin $expId
