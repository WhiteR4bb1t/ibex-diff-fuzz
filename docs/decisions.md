# Decisions
## Environment
### Toolchain in Docker image

*2026-10-01* 

A Docker image makes it easy to install the fuzzer on new devices where the 
toolchain and its requirements aren't installed. It also gives every system exactly 
the same tools, so a `.S` file isn't assembled differently on one machine than on another. 

### Ibex as submodule on my own fork

*2026-10-02*

Verilator 5.052 checks loops per signal, not per bit, and reports a false
`UNOPTFLAT` warning in Ibex. I had to add `-Wno-UNOPTFLAT` to the Ibex
co-simulation core, otherwise it wouldn't compile. This is not a real error and
not a bug. Without a fork and a submodule the fix would only exist locally, so I
forked Ibex and added the fork as a submodule to get the fix onto every new device.

## Oracle / Testcases

### No loads from device addresses

*2026-10-12*

Spike has no devices connected, so device addresses are plain memory there. A
load from a device address therefore mismatches whenever Ibex has a real device
at that address. Stores were allowed at first; since 2026-11-07 generated stores
use the same range as loads → [Loads and stores only in the RAM buffer](#loads-and-stores-only-in-the-ram-buffer).
The wrapper still ends the simulation with a store to `0x20008`.

**Cost:** `mtime` cannot be read.

### Testcases as assembler, not compiled C

*2026-10-12*

I need full control over every byte and every instruction to place specific
instructions (`wfi`), the `reset_handler` and the `.vectors` section.

### All trap vectors jump to exit

*2026-10-12*

My testcases should not trap, so every unexpected trap stops the execution
immediately.

### Sleep loop after the exit

*2026-11-08*

After the exit store the core keeps running for a few cycles into zero-initialized
memory, which caused an exception at the end of every program. A `wfi` loop after
the exit fixes that. It breaks the [no-wfi rule](#no-wfi-in-the-generator), but only
after the exit: if a run ends up sleeping there, the exit didn't work, which is
still a finding.

### Control tests with known output

*2026-10-12*

I need tests so I instantly see a problem in the `classify` or `timeout_classify`
functions. They also show what the output logs look like, so I could design
those functions around them.

### No writes to mcycle or mcountinhibit

*2026-10-21*

The timeout classification depends on `mcycle`; a manipulated value would lead
to a wrong classification of timeout runs.

## Generator

### Own instruction generator instead of an external one

*2026-11-06*

This is a learning project, and the generator is one of the biggest parts of the
fuzzer, if not the biggest, so I don't want to depend on external sources here.
For now I also only want to generate instructions I fully control and really
understand.

### Uniform choice over all instructions, not over formats

*2026-11-07*

At first the generator picked a format and then an instruction from it. That
caused a strong bias. The idea was to test every format equally, but the fuzzer
tests how instructions work, not instruction formats, so a uniform distribution
over all instructions is more efficient.

### No ecall, ebreak or fence

*2026-11-07*

`ecall` and `ebreak` both raise an exception, which isn't checked for now. Since
there is no exception handling, the program simply exits
([All trap vectors jump to exit](#all-trap-vectors-jump-to-exit)). `fence` adds
little: there are no devices except the timer, so it has no effect.

### jalr only to labels

*2026-11-07*

To prevent loops, `jalr` jumps to labels like the other jumps: `lla` loads the
address of the label, then `jalr` jumps with offset 0.
→ [Only forward jumps](#only-forward-jumps)

### All registers, never `x0` as base register

*2026-11-07, updated 2026-11-08*

No register can break the surrounding program, so all registers from `x0` to `x31`
are used. Both Spike and Ibex start with every register set to 0.

Loads and stores need a base register plus an offset. The generator picks a random
address, writes the upper 20 bits into a register with `lui` and uses the lower 12
bits as the offset, so it keeps full control over the address.

Since 2026-11-08 the base register of `jalr`, loads and stores is never `x0`:
`x0` can't hold the address, so the access or the jump doesn't go where it should.
A load or store should hit valid memory. Found in
[Jump to `0x00000000`](triages.md#jump-to-0x00000000).

### Loads and stores only in the RAM buffer

*2026-11-07*

Loads and stores use `0x110000`–`0x1FFFF0`. This is where Ibex and Spike have the
same memory: Spike's memory is zero almost everywhere except where the program is
loaded, Ibex only has device-free memory from `0x100000`, and both have the program
at `0x100080`, which must not be overwritten. Starting at `0x110000` leaves a
generous buffer, so large programs still fit. A bigger range also has a downside:
a store and a load to the same region become less likely.

Stores use the same range as loads. A store to an address Ibex doesn't have
traps, so the program exits early. A store to SimCtrl could end the simulation
early as well, and stores to devices have no use. A shared range also makes loads
and stores to the same address more likely.

### Unaligned memory accesses allowed

*2026-11-07*

They lead to exactly the edge cases I want to look at, and my environment allows
them.

## Classifier

### Classify by stdout, not exit code

*2026-10-13*

Measured: match, timeout and unloadable ELF all exit 0; only a mismatch exits 1.
Exit 1 means "any `$fatal`/assertion", not specifically a co-simulation mismatch
(`verilator_sim_ctrl.cc`, `WasSimulationSuccessful`).

**Cost:** The classifier depends on log strings and breaks if Ibex changes them.
The logs in `tests/logs/` catch that.

### Second run on timeout with a bigger limit

*2026-10-14*

The simulation is deterministic, so the way the counters in the log grow between
the two runs shows the cause of the timeout.

### Sleep, hang and loop via executed cycles

*2026-10-20*

The clock gate pauses `mcycle` during sleep. The offset between cycles and
executed cycles stayed constant over 10 runs with different limits. The second
run decides the cause of the timeout:

- `exec_cycles - cycles` greater than 5 in the second run → **sleep**
- otherwise, retired instructions equal in both runs → **hang**
- otherwise → **loop**

### No wfi in the generator

*2026-10-13*

Without `wfi` the CPU has no legitimate way to sleep before the exit, so every
sleep before the exit is an RTL error. The only `wfi` is the
[sleep loop after the exit](#sleep-loop-after-the-exit).

### Only forward jumps

*2026-10-13*

The limit for a run can be calculated as instruction count × worst-case latency + a constant. Every timeout should be a finding, because there is no reason for
the program to loop.

### Unknown as separate category; the campaign continues

*2026-10-13*

One unknown log should not end the whole campaign. Instead it is saved as an
`Unknown` result and reviewed by the user.

### ElfError separated from Unknown

*2026-10-14*

`ElfError` is an error in the generator or linker, `Unknown` is simulator output
the classifier doesn't recognize.

### Counters as optional

*2026-10-14*

"Missing" is not the same as 0. Mismatch logs don't contain some counters at
all, and storing 0 for a missing counter would be indistinguishable from a real 0.

## Runner

### Runner in C++, simulator as external process

*2026-10-13*

I want to improve my C++ skills, and running the simulator as an external process
is easier. In the long term I'd rather write my own harness that controls Ibex
and Spike directly, to get more information than just the logs.

### One directory per testcase

*2026-10-13*

The assembler decides which bytes are generated from the `.S` file, so both the
`.S` file and the ELF are saved. The output of gcc and of the simulator go into
`.out` files: the classifier reads the simulator output, and the gcc output helps
debugging instead of just ending in a crash. Matches are deleted right away, so
after a campaign only the interesting IDs are left.

### Testcase ID and directory come from the loop

*2026-11-03*

`run_one` doesn't create its ID, because it doesn't know at which position of the
campaign it runs. The fuzz loop gives it the ID, which is also the name of the
testcase directory, because it is unique. The loop creates the directory with the
`.S` file inside; `run_one` gets the directory and the file name without a path.
A fixed file name would have to be changed in `run_one` as well, and this way test
`.S` files can be used without renaming them.

### Run directory relative to the working directory, Ibex build relative to the binary

*2026-11-03*

The run directory is relative to the directory the fuzzer is started in, so it can
be started from anywhere and different campaigns can live in different places. The
Ibex build belongs to the fuzzer and has to be found no matter where the fuzzer is
started; relative to the working directory would require a fixed working directory.

## Loop

### Errors: DUT, fuzzer, own code

*2026-11-03, updated 2026-11-08*

An error of the DUT is found through a mismatch. An error of the fuzzer at runtime
is an exception. An error in my own code, like calling a function wrongly, is an
`assert`.

There are two exception classes: `FatalError` breaks the whole campaign, `RunError`
only the current run. Almost all errors are `RunError`s: the run is skipped and its
directory is kept for review, so a small error doesn't crash the whole campaign.

## Tests

### One test function per function

*2026-10-22*

One test function for all functions would give every function added to it a chance
to bring a bug into the test. With one test per function, a test stays as it is, is
not influenced by other tests and always checks its function in exactly the same way.

### timeout_classify is tested with synthetic results

*2026-10-23*

The function takes results, not logs, and synthetic results cover many more edge
cases. Most important: going through `classify` would break
[one test function per function](#one-test-function-per-function), because a bug in
`classify` would show up as a bug in `timeout_classify`.

### timeout_classify tests only the oracle

*2026-11-03*

Tested:

- The second run is not a timeout: `Unknown` or `ElfError` throw an exception,
  anything else (e.g. `Match`) is a bad timeout.
- `exec_cycles < cycles` throws an exception. I removed the `assert` for it, because
  this might become an error of the DUT later.

Not tested:

- The first run is not a timeout: that is an error of the caller.
- Missing statistics: that is an error of the runtime before, and it is checked.
  It has nothing to do with the oracle.
