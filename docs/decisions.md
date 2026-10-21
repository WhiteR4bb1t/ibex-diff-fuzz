# Decisions

## Environment

### Toolchain in Docker image

*2026-10-01*

A Docker image makes it easy to install the fuzzer on new devices where the
toolchain and its requirements aren't installed. It also gives every system
exactly the same tools, so a `.S` file isn't assembled differently on one machine
than on another.

### Ibex as submodule on my own fork

*2026-10-02*

Verilator 5.052 checks loops per signal, not per bit, and reports a false
`UNOPTFLAT` warning in Ibex. I had to add `-Wno-UNOPTFLAT` to the Ibex
co-simulation core, otherwise it wouldn't compile. This is not a real error and
not a bug. Without a fork and a submodule the fix would only exist locally, so I
forked Ibex and added the fork as a submodule to get the fix onto every new device.

## Oracle / Testcases

### No loads from device addresses, stores allowed

*2026-10-12*

Spike has no devices connected, so device addresses are plain memory there. A
load from a device address therefore mismatches whenever Ibex has a real device
at that address. Stores are allowed: the stored value comes from the same
register on both sides, so there is no reason for a mismatch. Stores are also
required, because the simulation ends with a store to `0x20008`.

**Cost:** `mtime` cannot be read.

### Testcases as assembler, not compiled C

*2026-10-12*

I need full control over every byte and every instruction to place specific
instructions (`wfi`), the `reset_handler` and the `.vectors` section.

### All trap vectors jump to exit

*2026-10-12*

My testcases should not trap, so every unexpected trap stops the execution
immediately.

### Control tests with known output

*2026-10-12*

I need tests so I instantly see a problem in the `classify` or `timeout_classify`
functions. They also show what the output logs look like, so I could design
those functions around them.

### No writes to mcycle or mcountinhibit

*2026-10-21*

The timeout classification depends on `mcycle`; a manipulated value would lead
to a wrong classification of timeout runs.

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

Without `wfi` the CPU has no legitimate way to sleep, so every sleep is an RTL
error.

**Cost:** Bugs in waking the CPU stay hidden.

### Only forward jumps

*2026-10-13*

The limit for a run can be calculated as instruction count × worst-case latency
+ a constant. Every timeout should be a finding, because there is no reason for
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
`.S` file and the ELF are saved. Uninteresting testcases get cleaned up by the
fuzz loop.
