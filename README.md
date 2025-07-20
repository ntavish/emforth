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

There is a `test.forth` file which contains the implementation of basic
control flow words if/then/else and a test word. This is just for demoing
the capability, but a more feature-full init.forth is WIP.

```shell
$ make && ./build/emforth <test.forth
make: Nothing to be done for 'all'.
emForth initialized
: print-if-true 0branch 96 lit 65 emit lit 13 emit lit 10 emit branch 80 lit 66 emit lit 13 emit lit 10 emit lit 67 emit lit 13 emit lit 10 emit ;
STACK >
A
C
STACK >
B
C
STACK >
Error or EOF. Exiting.
```
