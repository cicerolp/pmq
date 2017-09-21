====================
Experiments with PMQ
====================


.. contents::

1 Benchmark Insert and Scan
---------------------------

.. _exp20170822165129:

1.1 **DONE** [2017-08-22 Ter]  Experiment *bench_insert_and_scan* ``exp20170822165129``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`./data/inf-desktop/exp20170822165129/exp.rst <./data/inf-desktop/exp20170822165129/exp.rst>`_

1.2 **ANALYSIS** [2017-08-25 Sex]  Twitter dataset *bench_insert_and_scan* ``exp20170825181747``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Testing with twitter distribution of data

`./data/cicero/exp20170825181747/exp.rst <./data/cicero/exp20170825181747/exp.rst>`_

1.3 **DONE** [2017-08-22 Ter]  *bench_insert_and_scan* V.2 ``exp20170907112116``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Old exeperiment used Rtree\* algorithm, use the quadratic one now.

- Count was not correct in the plots

`exp20170822165129`_

`./data/cicero/exp20170907112116/exp.rst <./data/cicero/exp20170907112116/exp.rst>`_

1.4 **DONE** [2017-09-15 Sex]  Experiment *bench_insert_and_scan* ``paper:exp20170919161448``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test insertions with larger inputs

- Rtree (quadratic)

- Btree

- PMQ

- DenseVector

`./data/cicero/exp20170919161448/exp.rst <./data/cicero/exp20170919161448/exp.rst>`_

2 Benchmark Queries region
--------------------------

2.1 **DONE** [2017-08-30 Qua]  *bench_queries_region* ``exp20170830124159``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`./data/cicero/exp20170830124159/exp.rst <./data/cicero/exp20170830124159/exp.rst>`_

- PMQ best on queries with large amount of elements

2.2 **DONE** [2017-09-04 Seg]  Test the refinments levels ``exp20170904153555``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test the best refinement level to use in Geohash programs. 

`./data/cicero/exp20170904153555/exp.rst <./data/cicero/exp20170904153555/exp.rst>`_

2.3 **ANALYSIS** [2017-09-07 Qui]  *bench_queries_region* V.2 ``exp20170907145711``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test with a larger ammount of data -> 10\*8

- added bulk RTREE loading to the experiments

`./data/cicero/exp20170907145711/exp.rst <./data/cicero/exp20170907145711/exp.rst>`_

2.4 **DONE** [2017-09-15 Sex]  *bench_queries_region* V.3 ``exp20170915143003``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Fixed code of Rtree.

- More elements in the time window: 6h \* batches of size 1000

`./data/cicero/exp20170915143003/exp.rst <./data/cicero/exp20170915143003/exp.rst>`_

3 Benchmark Insert and Remove
-----------------------------

3.1 **ANALYSIS** [2017-09-14 Qui]  *bench_insert_remove_count* ``exp20170914091842``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Test performance of removals in the PMQ. 

`./data/cicero/exp20170914091842/exp.rst <./data/cicero/exp20170914091842/exp.rst>`_
