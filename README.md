emForth
=======

This is an implementation of forth written in C, as a hobby project
to learn about forth. Jonesforth has been used as the biggest reference.
A few starting goals about this project:

- easy to target on multiple (modern) architectures without
  much focus on being portable to several compilers (gcc is assumed)
- embeddable in other code, including on microcontrollers
- readability, however if someone studies this to learn about forth,
  jonesforth.S familiariaty is assumed

This forth is implementating what is normally called a Direct threaded
code.

Note that this is probably not doing anything too differently from another
implementation of a forth in C. Originally the goal was to implement this
in a way that makes it portable, while also be able to sandbox it. During
the implementation, the sandboxing aspect was basically dropped and maybe
I'll try re-implementing it in a way that allows this. That kind of
implementation probably requires either an emulator of a CPU, or a small
CPU like virtual machine, and the forth targetting the CPU, something for
another time.

## Compiling and running

```shell
$ make
$ ./build/emforth
```

## Try it

Forth works on a **stack**. Numbers you type are pushed onto it; words take
their inputs from the stack and push results back. `.` pops the top number and
prints it. `.s` shows the whole stack (top first) without changing it.

**Arithmetic is written operator-last:**

```
2 3 + .           → 5
2 3 + 4 * .       → 20        that's (2 + 3) * 4, no brackets needed
17 5 mod .        → 2
```

**Moving things around on the stack:**

```
1 2 3 .s          → STACK > 3 2 1
drop drop drop
5 dup .s          → STACK > 5 5      dup copies the top
drop drop
1 2 swap .s       → STACK > 1 2      swap exchanges the top two
drop drop
```

**Defining new words:** `:` starts a definition, `;` ends it. Your words work
exactly like the built-in ones.

```
: square dup * ;
7 square .        → 49
: cube dup square * ;
3 cube .          → 27
see cube          → shows how cube is defined
```

**Printing characters:** `emit` prints a character by its code.

```
: cr 13 emit 10 emit ;
72 emit 105 emit cr       → Hi
```

**Comparisons** leave 1 for true and 0 for false:

```
3 4 < .           → 1
5 5 = .           → 1
```

**Teaching the language `if`.** emforth has no `if` built in. These three lines
add it, using only the two jump primitives `branch` and `0branch`. Type them
in first:

```
: if immediate ' 0branch , here 0 , ;
: then immediate dup here swap - swap ! ;
: else immediate ' branch , here 0 , swap dup here swap - swap ! ;
```

Now you can use them in your own words:

```
: max over over < if swap then drop ;
3 9 max .         → 9
: bigger? 10 > if 66 emit else 83 emit then cr ;
50 bigger?        → B
3 bigger?         → S
```

**Loops, the same way:**

```
: begin immediate here ;
: until immediate ' 0branch , here - , ;

: countdown begin dup . 1- dup 0= until drop ;
5 countdown       → 5 4 3 2 1  (one per line)
: stars begin 42 emit 1- dup 0= until drop cr ;
5 stars           → *****
: fact 1 swap begin swap over * swap 1- dup 0= until drop ;
5 fact .          → 120
```

Things to know: numbers are unsigned 32-bit (there's no `-5`), loops need a
starting number of 1 or more, `\` starts a comment, and anything you define is
gone when you reload the page.

