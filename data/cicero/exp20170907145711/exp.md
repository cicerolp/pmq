
# Description     :export:

Test the queries on uniform data. 
And compare the folling performances.

Use 10\*\*8 elements. 

-   PMQ / GEOHASH
-   BTREE
-   RTREE - quadratic algorithm
-   RTREE - quadratic algorithm with bulk loading

Use the refinement level = 8 


# Analisys


## Results


### Plot overview     :export:

![img](./img/overview_query_region.png)


### Conclusions     :export:

-   PMQ shows its best benefits on large range queries
-   for very small queries we are similar to othe Btree an Rtree
-   Bulk loading on Rtree only work on static case. The partitionning is optimized when all the queries are loaded together.


## What is the actual count of elements per query ?:


### Table     :export:

There are some queries where the count differs for Rtree by a small amout of elements
Counts have some differences :
Cases where the Count doesn't match exactly. 

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-right" />

<col  class="org-right" />

<col  class="org-right" />

<col  class="org-right" />

<col  class="org-right" />

<col  class="org-right" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-right">queryId</th>
<th scope="col" class="org-right">BTree\_Count</th>
<th scope="col" class="org-right">GeoHashBinary\_Count</th>
<th scope="col" class="org-right">RTreeBulk\_Count</th>
<th scope="col" class="org-right">RTree\_Count</th>
<th scope="col" class="org-right">Var</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-right">1</td>
<td class="org-right">13238671</td>
<td class="org-right">13238671</td>
<td class="org-right">13238674</td>
<td class="org-right">13238672</td>
<td class="org-right">2</td>
</tr>


<tr>
<td class="org-right">2</td>
<td class="org-right">13232631</td>
<td class="org-right">13232631</td>
<td class="org-right">13232632</td>
<td class="org-right">13232632</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">3</td>
<td class="org-right">13236197</td>
<td class="org-right">13236197</td>
<td class="org-right">13236199</td>
<td class="org-right">13236199</td>
<td class="org-right">1.333</td>
</tr>


<tr>
<td class="org-right">5</td>
<td class="org-right">13234142</td>
<td class="org-right">13234142</td>
<td class="org-right">13234143</td>
<td class="org-right">13234143</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">6</td>
<td class="org-right">13236459</td>
<td class="org-right">13236459</td>
<td class="org-right">13236459</td>
<td class="org-right">13236456</td>
<td class="org-right">2.25</td>
</tr>


<tr>
<td class="org-right">7</td>
<td class="org-right">13237088</td>
<td class="org-right">13237088</td>
<td class="org-right">13237091</td>
<td class="org-right">13237091</td>
<td class="org-right">3</td>
</tr>


<tr>
<td class="org-right">8</td>
<td class="org-right">13237620</td>
<td class="org-right">13237620</td>
<td class="org-right">13237620</td>
<td class="org-right">13237617</td>
<td class="org-right">2.25</td>
</tr>


<tr>
<td class="org-right">11</td>
<td class="org-right">3307714</td>
<td class="org-right">3307714</td>
<td class="org-right">3307716</td>
<td class="org-right">3307716</td>
<td class="org-right">1.333</td>
</tr>


<tr>
<td class="org-right">14</td>
<td class="org-right">3311510</td>
<td class="org-right">3311510</td>
<td class="org-right">3311512</td>
<td class="org-right">3311512</td>
<td class="org-right">1.333</td>
</tr>


<tr>
<td class="org-right">15</td>
<td class="org-right">3307750</td>
<td class="org-right">3307750</td>
<td class="org-right">3307751</td>
<td class="org-right">3307749</td>
<td class="org-right">0.667</td>
</tr>


<tr>
<td class="org-right">16</td>
<td class="org-right">3306478</td>
<td class="org-right">3306478</td>
<td class="org-right">3306479</td>
<td class="org-right">3306480</td>
<td class="org-right">0.917</td>
</tr>


<tr>
<td class="org-right">20</td>
<td class="org-right">827282</td>
<td class="org-right">827282</td>
<td class="org-right">827283</td>
<td class="org-right">827283</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">23</td>
<td class="org-right">826550</td>
<td class="org-right">826550</td>
<td class="org-right">826550</td>
<td class="org-right">826549</td>
<td class="org-right">0.25</td>
</tr>


<tr>
<td class="org-right">26</td>
<td class="org-right">826961</td>
<td class="org-right">826961</td>
<td class="org-right">826961</td>
<td class="org-right">826960</td>
<td class="org-right">0.25</td>
</tr>


<tr>
<td class="org-right">27</td>
<td class="org-right">826865</td>
<td class="org-right">826865</td>
<td class="org-right">826866</td>
<td class="org-right">826866</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">30</td>
<td class="org-right">206006</td>
<td class="org-right">206006</td>
<td class="org-right">206006</td>
<td class="org-right">206005</td>
<td class="org-right">0.25</td>
</tr>


<tr>
<td class="org-right">33</td>
<td class="org-right">206557</td>
<td class="org-right">206557</td>
<td class="org-right">206558</td>
<td class="org-right">206558</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">41</td>
<td class="org-right">51758</td>
<td class="org-right">51758</td>
<td class="org-right">51759</td>
<td class="org-right">51759</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">42</td>
<td class="org-right">51959</td>
<td class="org-right">51959</td>
<td class="org-right">51960</td>
<td class="org-right">51960</td>
<td class="org-right">0.333</td>
</tr>


<tr>
<td class="org-right">56</td>
<td class="org-right">12961</td>
<td class="org-right">12961</td>
<td class="org-right">12962</td>
<td class="org-right">12962</td>
<td class="org-right">0.333</td>
</tr>
</tbody>
</table>

