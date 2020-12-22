# Unnamed C Compiler

A feature-incomplete C to ARM compiler

## Setup

```bash
apt-get install g++
```

## Building

```bash
g++ --std=c++11 -o ucc src/*cc src/*cpp
```

## Usage

To compile `source.c`:
```bash
./ucc -S -c assembly.s source.c
```
Will create a file called `assembly.s`

See `inputs/` for an example of a compatible C source file.

## Running assembly

ARM assembly can be assembled and run on x86 systems with gcc cross compilers and qemu

```bash
sudo apt install gcc-arm-linux-gnueabi qemu-user
arm-linux-gnueabi-gcc -o output assembly.s
qemu-arm /usr/arm-linux-gnueabi/lib/ld-linux.so.3 --library-path /usr/arm-linux-gnueabi/lib ./output
```

## Feature support
#### Includes:
* `int` datatype
* Functions with up to 4 arguments, returning `int` or `void`
* Addition and subtraction
* Equality and greater than/less than comparisons
* `if` and `else`
* `while` and `for` loops (Experimental)
* Compile time evaluation of basic expressions

#### Planned:
* Nested expressions e.g. `(a + b + (c - a) + (d + e))`
* Arrays and item access `a[2]`
* Bitshift operations `a << 2`
* Inplace increment and decrement `a++/a--`
* Conditional expressions `(a==1)?b:c`
* Pointers

#### Unsupported:
* Structs
* Heap memory allocation

