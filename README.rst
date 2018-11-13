.. figure:: https://img.shields.io/badge/version-1.0-blue.svg 

.. figure:: https://img.shields.io/github/license/cicerolp/pmq.svg

====================
Related Repositores 
====================

- `Main Source Code <https://github.com/cicerolp/pmq>`_
- `Supplementary Material <https://github.com/cicerolp/pmq-extras>`_

====================
Authors
====================

- Júlio Toss, Cícero L. Pahins, Bruno Raffin, and João L. Comba

====================
Packed-Memory Quadtree
====================

Packed-Memory Quadtree: a Cache-Oblivious Data Structure for Visual Exploration Of Streaming Spatiotemporal Big Data
---------------------------

ABSTRACT
---------------------------
The visual analysis of large multidimensional spatiotemporal datasets poses challenging questions regarding storage requirements and query performance. Several data structures have recently been proposed to address these problems that rely on indexes that
pre-compute different aggregations from a known-a-priori dataset. Consider now the problem of handling streaming datasets, in which data arrive as one or more continuous data streams. Such datasets introduce challenges to the data structure, which now has to support dynamic updates (insertions/deletions) and rebalancing operations to perform self-reorganizations. In this work, we present the Packed-Memory Quadtree (PMQ), a novel data structure designed to support visual exploration of streaming spatiotemporal datasets. The PMQ is cache-oblivious to perform well under different cache configurations. We store streaming data in an internal index that keeps a spatiotemporal ordering over the data following a quadtree representation, with support for real-time insertions and deletions. We validate our data structure under different dynamic scenarios and compare to competing strategies. We demonstrate how PMQ can be used to answer different types of visual spatiotemporal range queries of streaming datasets.
