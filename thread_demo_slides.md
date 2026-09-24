# POSIX Threads Demo in C
### Operating Systems Assignment Walkthrough

---

# What this project is for

- demonstrate **thread creation**
- demonstrate **unsynchronized vs. synchronized execution**
- investigate **thread scheduling / priority behavior**
- provide a small example that is easy to explain in class

---

# Audience assumptions

- you know the OS ideas of **process**, **thread**, **race condition**, and **scheduler**
- you do **not** need prior C experience
- this deck explains the code in plain language first, then connects it to C and POSIX threads

---

# Files to know

- `/home/runner/work/Operating-System-Demos/Operating-System-Demos/thread_demo.c`
  - the actual C program
- `/home/runner/work/Operating-System-Demos/Operating-System-Demos/Makefile`
  - build instructions
- `/home/runner/work/Operating-System-Demos/Operating-System-Demos/README.md`
  - short usage notes
- `/home/runner/work/Operating-System-Demos/Operating-System-Demos/thread_demo_slides.md`
  - this presentation

---

# How to build and run

```bash
make
./thread_demo
```

- `make` compiles the program
- `./thread_demo` runs all three demos in sequence

---

# Very small C primer

- `#include ...` imports library features
- `main()` is the program entry point
- a `struct` groups related fields together
- a function marked `static` is used only inside this file
- `printf(...)` prints text to the terminal
- `void *` is a generic pointer type often used by the pthread API

---

# Why POSIX threads?

- POSIX threads are the standard thread API on Unix-like systems
- the code uses real OS-managed threads, not a classroom simulation
- that makes the demo good for discussing what the scheduler actually does

---

# High-level program flow

`main()` runs three sections:

1. `run_thread_creation_demo()`
2. `run_counter_demo(0)` and `run_counter_demo(1)`
3. `run_scheduling_demo()`

This structure matches the three required assignment concepts.

---

# Section 1: thread creation

The program creates **five threads**.

Each thread:
- starts
- performs a distinct task
- prints its result
- finishes

This makes the work easy to attribute to a specific thread.

---

# The five thread tasks

1. sum a small list of integers
2. count vowels in a sentence
3. reverse a word
4. compute a factorial
5. find a maximum value in a list

These tasks are simple, but they are not identical.

---

# How thread creation works in the code

`run_thread_creation_demo()`:

- prepares an array of `DistinctTaskArgs`
- calls `pthread_create(...)` five times
- later calls `pthread_join(...)` five times

Meaning:
- `pthread_create` starts a new thread
- `pthread_join` waits for that thread to finish

---

# Function: `run_distinct_task`

This is the worker function used by the five creation threads.

It:
- receives a pointer to that thread's input data
- prints that the thread started
- chooses one task with a `switch` statement
- prints the result of the task
- prints that the thread finished

---

# Why the start/finish messages matter

They let us show:
- thread lifetime
- overlapping execution
- which output belongs to which thread

They also make it easier to discuss nondeterminism:
- threads may start in one order
- finish in another order

---

# Section 2: unsynchronized execution

Goal:
- show a **real concurrency bug**

Shared variable:
- `shared_counter`

Worker function:
- `run_counter_task()`

Unsynchronized mode:
- multiple threads update the same shared value without protection

---

# The race condition

Unsynchronized increment logic is conceptually:

1. read `shared_counter`
2. add 1
3. write the new value back

If two threads do this at nearly the same time:
- both may read the same old value
- one update can overwrite the other

That is a **lost update** race condition.

---

# Why the unsynchronized result is nondeterministic

- thread interleavings depend on the scheduler
- interleavings can vary from run to run
- therefore the final counter value can change from run to run

The program prints:
- expected counter value
- actual counter value

If they differ, the race is visible.

---

# Function: `run_counter_demo`

This function runs the counter experiment.

It:
- resets the shared counter
- creates five worker threads
- chooses unsynchronized or synchronized behavior
- waits for all workers to finish
- prints expected vs. actual results

This is the clearest side-by-side comparison in the program.

---

# Section 2: synchronized execution

Synchronization tool:
- `pthread_mutex_t counter_mutex`

Protected logic:
- lock the mutex
- increment `shared_counter`
- unlock the mutex

Only one thread can be inside that critical section at a time.

---

# What a mutex does

Mutex = **mutual exclusion**

Purpose:
- protect shared state from simultaneous modification

In this program:
- the mutex serializes access to `shared_counter`
- that prevents lost updates
- the final value should match the expected total

---

# Function: `run_counter_task`

This worker function supports **two modes**:

- `use_mutex == 0`
  - unsafe increment
- `use_mutex == 1`
  - locked increment

This is useful because:
- the same worker behavior is reused
- only the synchronization strategy changes

That makes the comparison fair and easy to explain.

---

# What output to expect from section 2

Unsynchronized run:
- thread completion order may vary
- final counter is often too low

Synchronized run:
- thread completion order may still vary
- final counter should be correct

Important point:
- synchronization fixes data correctness
- it does **not** force a fixed print order

---

# Section 3: scheduling and priority

Goal:
- inspect what scheduling information the OS exposes
- attempt to change thread scheduling behavior
- report what actually happens

Threads used:
- three scheduling-test threads

---

# Scheduling APIs used

- `pthread_getschedparam(...)`
  - asks the OS which policy and priority a thread currently has
- `pthread_setschedparam(...)`
  - requests a different scheduling policy / priority
- `sched_get_priority_min(...)`
- `sched_get_priority_max(...)`
  - ask for valid priority ranges for a policy

---

# Policies mentioned in the code

- `SCHED_OTHER`
  - the normal Linux timesharing policy
- `SCHED_RR`
  - round-robin realtime policy

The demo:
- leaves one thread at default settings
- tries to move two threads to `SCHED_RR` with different priorities

---

# Function: `run_scheduling_task`

Each scheduling-test thread:

- prints its initial scheduling policy and priority
- optionally attempts a policy/priority change
- prints the effective policy and priority afterward
- performs CPU-heavy busy work
- records when it finished

This gives us both configuration data and observed behavior.

---

# Function: `run_scheduling_demo`

This function:

- chooses requested priorities
- starts three scheduling threads
- waits for them to finish
- sorts them by finish time
- prints the observed completion order

This supports a careful claim:
- priority may influence scheduling
- it does not guarantee execution order

---

# What happened in our test environment

Observed behavior:

- threads started with `SCHED_OTHER` and priority `0`
- attempts to switch to `SCHED_RR` failed with `Operation not permitted`

Interpretation:

- Linux often restricts realtime scheduling changes to privileged users
- therefore thread priority control may be unavailable in normal student environments

---

# Why this still satisfies the investigation requirement

The assignment asks for investigation, not false certainty.

This demo shows:
- what information the runtime exposes
- what changes were attempted
- whether the OS accepted those changes
- that priority control may be limited or advisory

That is a valid and honest conclusion.

---

# Key functions in plain English

- `sleep_for_ms`
  - pauses a thread briefly to make output easier to observe
- `policy_name`
  - converts a numeric scheduling policy into readable text
- `compare_finish_times`
  - helps sort scheduling threads by observed finish time
- `main`
  - runs all demos in a clear order

---

# Data structures in plain English

- `DistinctTaskArgs`
  - identifies a creation-thread's job
- `CounterTaskArgs`
  - tells a counter thread whether to use the mutex
- `SchedulingTaskArgs`
  - stores requested and effective scheduling information

These structs make each thread's inputs explicit and organized.

---

# There are no classes here

This is C, not Java or C++.

So instead of classes, the design uses:
- functions for behavior
- `struct` types for grouped data
- global shared state only where the demo needs shared memory

That is normal for a small C program.

---

# Strengths of this design

- very small and readable
- directly uses real POSIX thread APIs
- clearly separates the three assignment concepts
- easy to run repeatedly and compare outputs
- honest about scheduling limits on normal systems

---

# Limitations

- output order is intentionally unstable, so exact lines vary by run
- scheduling behavior is environment-dependent
- realtime priority changes may fail without admin privileges
- busy-wait work is artificial and used only for demonstration
- the demo emphasizes clarity more than performance

---

# Risks and caveats

- students might confuse synchronization with deterministic print order
- a rare unsynchronized run could accidentally produce the correct count
- different OSes may expose different scheduling policies
- aggressive compiler optimization could affect timing-sensitive demos

These are reasons the deck should explain the concepts, not just show output.

---

# Why threads were the right choice

- the assignment explicitly asks for threads
- shared memory between threads makes race conditions easy to demonstrate
- mutexes are a standard synchronization tool in thread-based programs
- scheduler behavior is naturally discussed at the thread level here

---

# How to present the demo live

1. explain the three assignment requirements
2. show the five distinct thread tasks
3. run the unsynchronized counter demo and discuss the wrong result
4. run the synchronized demo and discuss the mutex
5. show the scheduling output and explain why priority changes may fail
6. close with strengths, limitations, and risks

---

# Suggested conclusion

This program demonstrates that:

- creating threads is straightforward with POSIX threads
- unsynchronized shared state leads to race conditions and nondeterministic results
- a mutex can make the shared update safe
- scheduling and priority features are real OS mechanisms, but they may be restricted and do not guarantee exact execution order

---

# End

Questions?
