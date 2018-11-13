#!/bin/bash
expId='exp20170925155952'
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
cmake -DPMA_BUILD_DIR=$PMABUILD_DIR -DCMAKE_BUILD_TYPE="Release" -DBENCH_PMQ=ON -DBENCH_BTREE=ON -DBENCH_RTREE=ON -DBENCH_DENSE=ON ..
make

#get machine configuration
echo "" > $DATADIR/info.org
~/Projects/pmq/scripts/g5k_get_info.sh $DATADIR/info.org 

# EXECUTE BENCHMARK

#Continue execution even if one these fails
set +e 
#rm ${TMPDIR}/bench_ins_rm_17616_${EXECID}.log
#touch ${TMPDIR}/bench_ins_rm_17616_${EXECID}.log

# Queries insert remove count

# PMQ
cmake -DBENCH_PMQ=ON -DBENCH_BTREE=OFF -DBENCH_RTREE=OFF -DBENCH_DENSE=OFF . ; make
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 > ${TMPDIR}/bench_ins_rm_17616_23488000_${EXECID}.log

# BTREE and RTREE
cmake -DBENCH_PMQ=OFF -DBENCH_BTREE=ON -DBENCH_RTREE=ON -DBENCH_DENSE=OFF . ; make


stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 26424000 > ${TMPDIR}/bench_ins_rm_17616_26424000_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 22020000 > ${TMPDIR}/bench_ins_rm_17616_22020000_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 19818000 > ${TMPDIR}/bench_ins_rm_17616_19818000_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 18717000 > ${TMPDIR}/bench_ins_rm_17616_18717000_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 18166500 > ${TMPDIR}/bench_ins_rm_17616_18166500_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 17891250 > ${TMPDIR}/bench_ins_rm_17616_17891250_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 17753625 > ${TMPDIR}/bench_ins_rm_17616_17753625_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 17684812 > ${TMPDIR}/bench_ins_rm_17616_17684812_${EXECID}.log
stdbuf -oL ./benchmarks/bench_insert_remove_count -rate 1000 -n 35232000 -T 17616 -tSize 17650406 > ${TMPDIR}/bench_ins_rm_17616_17650406_${EXECID}.log

set -e

cd $TMPDIR
tar -cvzf log_$EXECID.tgz *_$EXECID.log

cd $DATADIR
cp $TMPDIR/log_$EXECID.tgz .

git checkout $expId

git add info.org log_$EXECID.tgz run.sh 
git add -u
git commit -m "Finish execution $EXECID"
git push origin $expId
