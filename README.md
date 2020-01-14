<p align="center">
  <img src="almalogo.svg" title="(alma logo)" width="324" height="119" />
</p>

Alma [![Build Status](https://travis-ci.org/broomweed/alma.svg?branch=master)](https://travis-ci.org/broomweed/alma)
====

Alma is a statically-typed, stack-based, functional programming language
heavily inspired by Forth, Standard ML, and the language [Cat][cat] created
by Christopher Diggins.

Note: I'm still getting around to rewriting the type system for the new version!
A lot of things don't work yet.

  [cat]: https://www.codeproject.com/articles/16247/cat-a-statically-typed-programming-language-interp

Simple examples
---------------

```
func main ( "Hello world!" say )
```
This program prints "Hello world!" to the console, followed by a newline.

```
func comp sort-one (
    shift -> first
    [first append]
    if: [first comp apply] [2dip] [dip]
)

func comp partition (
    [{} {}] dip
    while*: [empty not] [comp sort-one]
    drop
)

func quicksort (
    when*: [len 1 >] [
        shift -> pivot
        [pivot <] partition
        'quicksort to-both
        pivot prefix
        concat
    ]
)
```
This implements the famous [quicksort](https://en.wikipedia.org/wiki/Quicksort)
algorithm.

We can also implement the Sieve of Eratosthenes in under 10 lines:
```
func sieve (
    -> n ({} n iota shift drop)
    while*: [empty not] [
        shift -> prime
        [prime append]
        [[prime multiple not] filter]
        2>>2
    ]
    drop
)
```

Check out the `examples/` directory for a few more examples.
