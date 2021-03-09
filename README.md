<p align="center">
  <img src="almalogo.svg" title="(alma logo)" width="324" height="119" />
</p>

Alma [![Build Status](https://travis-ci.org/broomweed/alma.svg?branch=master)](https://travis-ci.org/broomweed/alma)
====

Alma is a statically-typed, stack-based, functional programming language
heavily inspired by Forth, Standard ML, and the language [Cat][cat] created
by Christopher Diggins.

Note: The type system actually is still languishing.
I had written typechecking for the old version,
but I didn't write any comments and now I am not sure how to reimplement it after I rewrote the rest of the code.
So right now badly-typed programs will just crash the interpreter.
Oops! Eventually I will fix this.

  [cat]: https://www.codeproject.com/articles/16247/cat-a-statically-typed-programming-language-interp

Build
-----

In order to build this, you will need the `readline` and `subunit` libraries, as well as the `check` library for testing.
On debian-based linux these packages are `libreadline-dev`, `libsubunit-dev` and `check`;
I don't know how to build them on other systems right now, sorry ._.;

Simple examples
---------------

```
def main ( "Hello world!" say )
```
This program prints "Hello world!" to the console, followed by a newline.

```
def comp sort-one (
    shift -> first
    [first append]
    if: [first comp apply] [2dip] [dip]
)

def comp partition (
    [{} {}] dip
    while*: [empty not] [comp sort-one]
    drop
)

def quicksort (
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
def sieve (
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

If normal programming languages are like reading prose, this kind of concatenative
programming is more like poetry. I'm not sure if it's useful, but it can be fun
and aesthetically pleasing, so that's nice at least. Here's how to find the twin
primes up to 1000 using the above `sieve` function in interactive mode:

```
alma> 1000 sieve -> primes ( primes [ -> n | primes [n 2 + contains] [n 2 - contains] 'or fork ] filter )
  { 3, 5, 7, 11, 13, 17, 19, 29, 31, 41, 43, 59, 61, 71, 73, 101, 103, 107, 109, 137, 139, 149, 151, 179, 181, 191, 193, 197, 199, 227, 229, 239, 241, 269, 271, 281, 283, 311, 313, 347, 349, 419, 421, 431, 433, 461, 463, 521, 523, 569, 571, 599, 601, 617, 619, 641, 643, 659, 661, 809, 811, 821, 823, 827, 829, 857, 859, 881, 883 }
````

Check out the `examples/` directory for a few more examples.
