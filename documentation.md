## Array

* `join(delimiter: String = ", ", prefix: String = "[", suffix: String = "]"): String`: join all elements into one string with the specified `delimiter`, `prefix`, and `suffix`. 

* `string_join(delimiter: String = ", ", prefix: String = "[", suffix: String = "]"): String `: similar to `join()`, but is faster if all elements in the array are already string. Throw `TypeError` is any element is not a string. 

* `subarray(start: Int = 0, end: Int = this.length, new_length: Int = end - start): Array `: return a subarray
* `iterator(): Iterator`
* `static copy(src: Array, dest: Array, src_index: Int, dest_index: Int, length: Int)`: Similar to `memcpy` in c. If `src` and `dest` are the same array, use `memmove` instead. 


## Map

* `get_or(key, default_value): Value`: if `key` does not exist in the map, return  `default_value`, otherwise return the value associated with `key`. 
* `delete(key): Value`: delete a key-value pair and return the value.
* `iterator(): Iterator`: the iterator's `next()` returns an array of length 2 `key, value` each time.

## String

* `substring(start: Int, end: Int): String`: return a new substring.
* `replace(start: Int, end: Int, insert: String): String`: return a new string which is equal to the old string with the part `[start, end)` replaced by the `insert` string. 
* `iterator(): Iterator`
* `static concat(values...): String`: concatenate values into one string. The values can be of any types. 

## Class

* `subclass_of(c: Class): Bool`: true if this class is a subclass of `c` (or `this == c`). Otherwise, return false. 

## Vector

* `init(elements...)`: initialize the vector with given elements. 
* `insert(element, index: Int)`: insert an element to a specific index
* `delete(index: Int): Value`: delete and return the value at a specific index
* `get(index: Int): Value`
* `set(index: Int, element)`: set the element at an existing index. Cannot use this function to add new elements.
* `append(element)`: add to the end
* `pop(): Value`: remove and return the last element. 
* `iterator(): Iterator`
* `size`: number of elements in the vector. 

***

* `fun range(start, end = nil, step = 1): Iterable`
* `fun benchmark(task: Function)`
* `fun foreach(iterable, handler: Function)`
* `fun enum(iterable): Iterable`
