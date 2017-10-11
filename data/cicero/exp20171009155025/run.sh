#!/bin/bash
expId='exp20171009155025'
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
cmake -DCMAKE_BUILD_TYPE="Release" -DTWITTERVIS=OFF -DRHO_INIT=OFF  ../pma_cd
make 

# make twitterVis
mkdir -p $BUILDIR
cd $BUILDIR 
cmake -DPMA_BUILD_DIR=$PMABUILD_DIR -DCMAKE_BUILD_TYPE="Release" -DBENCH_PMQ=ON -DBENCH_BTREE=ON -DBENCH_RTREE=ON -DBENCH_DENSE=ON -DBENCH_RTREE_BULK=ON ..
make

#get machine configuration
echo "" > $DATADIR/info.org
~/Projects/pmq/scripts/g5k_get_info.sh $DATADIR/info.org 

# EXECUTE BENCHMARK

#Continue execution even if one these fails
set +e 


for EL in 16 32 64 128 256 ; do
    ELTSIZE=$(($EL-16))
    cmake -DELT_SIZE=$ELTSIZE . ; make

    #Run queries
    #t=$((10**6))
    t=26000
    b=1000
    #n=$(($t*$b))
    ref=8
    stdbuf -oL ./benchmarks/bench_queries_region -seed 123 -x 33 -rate ${b} -min_t ${t} -max_t ${t} -ref ${ref} -bf ../data/queriesLHS.csv >  ${TMPDIR}/bench_queries_region_random_${t}_${b}_${ref}_${ELTSIZE}_${EXECID}.log

done
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
