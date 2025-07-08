# Print Float

Shortest-round-trip single-precision float printing in C.

This aims to be an extremely simple implementation in C of shortest-round-trip
floating point printing. The algorithm here is inspired by [A New Dragon in the
Den: Fast Conversion From Floating Point Numbers](https://youtu.be/w0WrRdW7eqg)
by Cassio Neri from C++Now 2024.

The simplicity of this implementation comes in part from the fact that it
uses double-precision floating-point arithmetic to implement its key
mathematical operations, rather than manual, dynamic fixed point arithmetic
using integers as many other float-printing libraries do. There is a tradeoff
in doing this, since you lose some guarantees about exact results given the
slight flexibility of IEEE 754 and the possibility of different CPU rounding
modes, etc., and you may sacrifice speed depending on hardware. But, the code
ends up being extremely simple, so for the sake of my understanding I decided
to write it this way.

The round-trip guarantee assumes the round-to-even tie-breaking policy for
the parser. It also makes no attempt to provide the nearest decimal
representation in the event that multiple valid shortest ones exist.

## Usage

The C program in this repo is not designed as a library, it is a standalone
program that performs a verification of the round-trip property, by converting
every possible positive 32-bit float to a string, then parsing it back again
with `strtof` from the C standard library.

If you want to use this code as a library, you'll need to convert it that way
on your own, though doing so should be very simple (probably just remove the
`main` function, and the `static` keyword on the function you want to use).

If you want to run this standalone test program, you can run:

```sh
gcc -O3 print-float.c -lm
./a.out
```

(substitute `clang` if you like).

## Verification

I've successfully run the round-trip verification program on the following
platforms:

- Apple clang 16.0.0 on macOS 15.5, arm64
- GCC 11.4.0 on Ubuntu 22.04, x86-64

## License

This software is [MIT Licensed](LICENSE).
