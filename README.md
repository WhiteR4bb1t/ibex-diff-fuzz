# Ibex-diff-fuzz

Ibex-diff-fuzz is a differential fuzzer for [Ibex](https://github.com/lowRISC/ibex),
an open-source RISC-V core. It is a learning project, not a production-ready fuzzer.

## Status

**Done**

- Generator for random RV32I assembler programs
- Runner: assemble, simulate under co-simulation, classify
- Fuzz loop with live status
- Classification of simulator logs and timeouts
- Tests for `classify` and `timeout_classify`

**Missing**

- Mutator
- Bug injection

## Usage

```
git clone --recursive https://github.com/WhiteR4bb1t/ibex-diff-fuzz.git
cd ibex-diff-fuzz
docker build -t fuzz-env -f docker/Dockerfile docker/
docker run --rm -it -v "$PWD":/work -v "$PWD/docker/bashrc":/tmp/.bashrc:ro \
  -w /work --user "$(id -u):$(id -g)" -e HOME=/tmp fuzz-env
```

Inside the container, build Ibex (config `maxperf`) and the fuzzer, then start a campaign:

```
fusesoc --cores-root=third_party/ibex run --target=sim --setup --build \
  lowrisc:ibex:ibex_simple_system_cosim \
  $(third_party/ibex/util/ibex_config.py --config_filename third_party/ibex/ibex_configs.yaml maxperf fusesoc_opts)
make all
bin/fuzztest all   # tests for the oracle
bin/fuzz       # campaign of 1000 testcases
```

`bin/fuzz` writes one directory per testcase into `run/` (relative to the working
directory). Matches are deleted, so every directory left in `run/` is a finding or an error.

Start every campaign in a new directory: a second campaign in the same directory starts
again at ID 1 and overwrites or deletes the findings of the first one. Continuing a campaign
is not supported yet.

## Decisions

The most important ones. All decisions with reasons: [docs/decisions.md](docs/decisions.md)

**Environment**

- [Toolchain in Docker image](docs/decisions.md#toolchain-in-docker-image) (2026-10-01):
  the same tools on every machine, so a `.S` file assembles to the same bytes everywhere.
- [Ibex as submodule on my own fork](docs/decisions.md#ibex-as-submodule-on-my-own-fork) (2026-10-02):
  carries the `-Wno-UNOPTFLAT` fix for Verilator 5.052 to every machine.

**Oracle / Testcases**

- [No loads from device addresses](docs/decisions.md#no-loads-from-device-addresses) (2026-10-12):
  Spike has no devices, so a device load always mismatches.
- [Testcases as assembler, not compiled C](docs/decisions.md#testcases-as-assembler-not-compiled-c) (2026-10-12):
  full control over every byte and instruction.

**Generator**

- [Own instruction generator instead of an external one](docs/decisions.md#own-instruction-generator-instead-of-an-external-one) (2026-11-06):
  only instructions I fully control and understand.
- [Loads and stores only in the RAM buffer](docs/decisions.md#loads-and-stores-only-in-the-ram-buffer) (2026-11-07):
  the only range where Ibex and Spike have the same memory.
- [All registers, never `x0` as base register](docs/decisions.md#all-registers-never-x0-as-base-register) (2026-11-08):
  `x0` can't hold an address.

**Classifier**

- [Classify by stdout, not exit code](docs/decisions.md#classify-by-stdout-not-exit-code) (2026-10-13):
  match, timeout and unloadable ELF all exit 0.
- [Second run on timeout with a bigger limit](docs/decisions.md#second-run-on-timeout-with-a-bigger-limit) (2026-10-14):
  the growth of the counters between both runs shows the cause.
- [Sleep, hang and loop via executed cycles](docs/decisions.md#sleep-hang-and-loop-via-executed-cycles) (2026-10-20):
  the clock gate pauses `mcycle` during sleep.
- [No wfi in the generator](docs/decisions.md#no-wfi-in-the-generator) (2026-10-13):
  every sleep before the exit is an RTL error.
- [Only forward jumps](docs/decisions.md#only-forward-jumps) (2026-10-13):
  programs can't loop, so every timeout is a finding.

**Runner / Loop**

- [One directory per testcase](docs/decisions.md#one-directory-per-testcase) (2026-10-13):
  `.S` file and ELF are both kept, because the assembler decides the bytes.
- [Errors: DUT, fuzzer, own code](docs/decisions.md#errors-dut-fuzzer-own-code) (2026-11-03):
  a broken run is skipped, only a `FatalError` stops the campaign.

**Tests**

- [timeout_classify tests only the oracle](docs/decisions.md#timeout_classify-tests-only-the-oracle) (2026-11-03):
  only the cases that belong to the oracle are tested.

## Gaps

- `mtime` cannot be read ([No loads from device addresses](docs/decisions.md#no-loads-from-device-addresses))
- Bugs in waking the CPU stay hidden ([No wfi in the generator](docs/decisions.md#no-wfi-in-the-generator))
- Ibex config `maxperf` was chosen without a specific reason; other configs are untested
- Errors can't be told apart yet: if a tool is missing (e.g. `riscv32-unknown-elf-gcc` not in
  `PATH`), the campaign only shows many errors without the cause

## Triages

- [0x30000: timer load in `hello_test`](docs/triages.md#0x30000-timer-load-in-hello_test) (2026-10-12):
  harness configuration, not an RTL or Spike bug.
- [Jump to `0x00000000`](docs/triages.md#jump-to-0x00000000) (2026-11-08):
  fuzzer generator, not an RTL or Spike bug.
