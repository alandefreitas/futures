# Partitioners

All algorithms also have an optional extra parameter for custom partitioners. A [partitioner] is simply a callable
object that receives two iterators representing a range and returns an iterator indicating where this range should be
split for a parallel algorithm. For instance, this would be a partitioner that always splits the problem in half:

{{ code_snippet("test/unit/snippets.cpp", "partitioner") }}

When we execute an algorithm with our custom partitioner, the algorithm would recursively split the input in half and
launch a task for each of these parts.

{{ code_snippet("test/unit/snippets.cpp", "partitioner_algorithm") }}

!!! hint

    A partitioner can indicate the problem is too small to be partitioned by returning `last`, which means the problem
    should not be split.

When no partitioner is provided, the [default_partitioner] is used, which imposes a minimum grain size to split the
problem and stops recommending the problem to be split if threads are busy with the problem.

--8<-- "docs/references.md"