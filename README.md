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

  [cat]: https://www.codeproject.com/articles/16247/cat-a-statically-typed-programming-language-interp

Simple example
--------------

```
func main : println "Hello world!" .
```
This program prints "Hello world!" to the console, followed by a newline.
