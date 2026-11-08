# Triages

One section per mismatch that was traced to its root cause.

## 0x30000: timer load in `hello_test`

*2026-10-12* · **Verdict:** harness configuration, not an RTL or Spike bug

### Symptom

Running the upstream `hello_test` under co-simulation (config `maxperf`):

- output is correct, 416 instructions match
- mismatch at time 1402: the DUT loads `0x2b7` from `0x30000`, Spike expected `0`
- Spike expects a synchronous trap (ISS PC `0x100000`), the DUT reports none (PC `0x100244`)

### Hypotheses

1. RTL bug in Ibex
2. Spike bug
3. Ibex and Spike are configured differently

### Root cause

- `c.lw a0, 0(a5)` at `0x100244` reads `0x30000`, which is `mtime` of the Simple System timer.
  The RTL returns the running counter (`0x2b7` = 695).
- The harness registers a single memory region with Spike (`add_memory(0x0, 0xFFFF0000)`) and
  copies the RAM image to `0x100000`. Spike has no timer at `0x30000`, only zeroed memory.
- `notify_dside_access` does not feed load values into Spike. The DUT's access is only *compared*
  against Spike's own load, so the data differs (line 1 of the message).
- On that mismatch `mmio_load` returns `false`, Spike raises a load access fault and jumps to
  `mtvec` (`0x100000`), while the DUT keeps running (line 2, same step).
- The load before it (`0x30004`, `mtimeh`) only matched by coincidence: the counter was still
  below 2³², and Spike's memory is 0.

### Consequence

Every value that only the hardware knows (MMIO such as `mtime`) produces a false mismatch until
the harness handles it. → [No loads from device addresses](decisions.md#no-loads-from-device-addresses)

## Jump to `0x00000000`

*2026-11-08* · **Verdict:** fuzzer generator, not an RTL or Spike bug

### Symptom

Running the fuzzer produces several mismatches per run:

- Spike expects a synchronous trap (ISS PC `0x100000`), the DUT reports none (PC `0x00000000`)

### Hypotheses

1. Problem with the generator

### Root cause

- `jalr x8, 0(x0)` at `0x1000e0` jumps to `0x00000000`. This is caused by a convention in the
  generator: a JALR first loads the address of a label into a random register and then uses that
  register as its base. The problem is that the random register can also be `x0`.
- Spike has `0x00000000` at `0x00000000`, which is an illegal instruction, so it raises
  `trap_illegal_instruction`.
- The DUT fetches instructions via the RAM's b port, which only uses bits `[19:2]` and then
  addresses the RAM with them. So `0x100000` and `0x000000` both become `0` and address the first
  instruction in memory. The first instruction in memory at `0x100000` is `jal x0, 84`, which is
  a valid instruction and doesn't trap.

### Consequence

`x0` can't hold a value, so every register except `x0` is allowed as the base register for jumps
to a label loaded with `lla`.
→ [All registers, never `x0` as base register](decisions.md#all-registers-never-x0-as-base-register)
