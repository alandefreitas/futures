# Partitioners

All algorithms also have an optional extra parameter for the algorithms can also accept custom partitioners instead of the [default_partitioner]. A [partitioner] is simply a callable object that receives two iterators representing a range and returns an iterator indicating where this range should be split for a parallel algorithm.


--8<-- "docs/references.md"