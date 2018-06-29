<p align="center">
  <img src="almalogo.svg" title="(alma logo)" width="324" height="119" />
</p>

Alma [![Build Status](https://travis-ci.org/broomweed/alma.svg?branch=master)](https://travis-ci.org/broomweed/alma)
====

Alma is a statically-typed, stack-based, functional programming language
heavily inspired by Forth, Standard ML, and the language [Cat][cat] created
by Christopher Diggins.

As of right now I'm pretty heavily rewriting all of it, so there's not
too much to show. The old version right now lives in the `old/` folder
but will be cleaned up at some point in the future. (It does have a working
type-inference system, which the current version so far lacks and which
I will probably end up cannibalizing for this version.)

However, the new version is approximately ***20*** times faster than
the old version. So I'm thinking the rewrite was a good idea. (compare
old/tritest with examples/triangle-numbers.alma: on my computer, the
old one takes 43.3s to run, while the new one takes about 2.1s.) This
speedup is due to a couple things that have changed in the new version:

  * no runtime name lookup (functions looked up at compile time)
  * no value copying (all values are immutable and passed around as pointers)
  * no type information at runtime (even though types are checked at compile
    time, the old version had values carry their types around at all times? why)
    - (however, note that the new one doesn't actually check types at all
        right now. but it will do them at compile time, so I think it's fair.)
  * to be fair, the new version in this experiment was compiled with -O3
    and the old version was compiled with no optimization, but also to be fair,
    the old one mysteriously segfaults (?!?!) if compiled with -O3, so, not
    sure what's up with that

  [cat]: https://www.codeproject.com/articles/16247/cat-a-statically-typed-programming-language-interp

Simple example
--------------

```
main [ "Hello world!" say ]
```
This program prints "Hello world!" to the console, followed by a newline.

```
empty [ len 0 = ]
small [ len 1 ≤ ]
concat [ if*: [empty] [drop] [unappend [concat] dip append] ]
2dip [ swap [dip] dip ]
when* [ [ ] if* ]
shift [ uncons swap ]

comp sort-one [
    shift -> first
    if: [first comp apply] [2dip: [first append]] [dip: [first append]]
]

comp partition [
    dip: [{} {}]
    while*: [empty not] [comp sort-one]
    drop
]

quicksort [
    when*: [small not] [
        shift -> pivot
        [pivot <] partition
        dip: [quicksort pivot append] ; quicksort concat
    ]
]
```
This program implements the famous [quicksort](https://en.wikipedia.org/wiki/Quicksort)
algorithm.
(In the future, the little things defined at the top would be part of the standard
 library... but there's no standard library yet ;) )

Check out the `examples/` directory for a few more examples.
