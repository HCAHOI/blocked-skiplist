# blocked-skiplist
Blocked SkipList is an ordered structure, unlike the common SkipList, Blocked SkipList organizes a certain number of elements in a contiguous memory space to achieve better spatial locality.

## Advantages

- Written in C++, unlike the C implementation, this container is more free to store key-value pairs read-efficiently.

- The number of elements in each node is balanced by a balancing mechanism.

- C++ STL-like interface, easy to use.