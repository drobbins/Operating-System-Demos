# Operating-System-Demos

Demo code for OS class.

## Threading Demo

This repository now includes a small POSIX threads demo in
`/home/runner/work/Operating-System-Demos/Operating-System-Demos/thread_demo.c`.

### Build

```bash
make
```

### Run

```bash
./thread_demo
```

### What it demonstrates

- creation of five threads that each perform a distinct task
- unsynchronized access to shared data that produces a race condition
- synchronized access to the same shared data using a mutex
- default scheduling information for threads
- an attempted priority/scheduling change and the observed effect, or lack of
  effect, on thread behavior
