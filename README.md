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
code. The code also implements what is called the inner interpreter as
a trampoline, which means it handles nested function calls in a loop
rather than using a call stack. (there is ofcourse still the forth return
stack).

## Compiling and running

```
make
./emforth
```
