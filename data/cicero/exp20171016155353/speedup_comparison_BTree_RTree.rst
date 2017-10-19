=======================================
Speedup comparison with BTree and RTree
=======================================


.. contents::

These results show the query's throughput speed-up for PMQ over BTree and RTree.

All the results were sorted by increasing speedUp value. 
The colors show the ratio between amount of false positive elements over the total amount of elements scanned by the query. 

.. image:: ./img/speedup_BTREE_RTREE.svg

We can see a correlation between the amount false positives and the Speedup over **RTrees** on small queries. 
This is due to the discontinuities caused by the Z-ordering scheme, used in the GeoHash algorithm. 
We can see that this correlation doesn't appear in the **BTree** speedups.

.. image:: ./img/speedup_BTREE_RTREE_facet.svg
