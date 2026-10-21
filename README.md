# Ibex-diff-fuzz

Ibex-diff-fuzz is a differential fuzzer for [Ibex](https://github.com/lowRISC/ibex),
an open-source RISC-V core. It is a learning project, not a production-ready fuzzer.

## Status

**Done**

- Classification of simulator logs
- Classification of timeouts
- Tests for `classify`

**Missing**

- Run function
- Fuzz loop
- Mutator
- Bug injection

## Usage

```
docker build -t fuzz-env -f docker/Dockerfile docker/
docker run --rm -it .:/work -v docker/bashrc:/tmp/.bashrc -w /work --user
$(id -u):$(id -g) -e HOME=/tmp fuzz-env
make
bin/fuzztests
```

## Decisions

The most important ones. All decisions with reasons: [docs/decisions.md](docs/decisions.md)

**Environment**

- [Toolchain in Docker image](docs/decisions.md#toolchain-in-docker-image) (2026-10-01):
  the same tools on every machine, so a `.S` file assembles to the same bytes everywhere.
- [Ibex as submodule on my own fork](docs/decisions.md#ibex-as-submodule-on-my-own-fork) (2026-10-02):
  carries the `-Wno-UNOPTFLAT` fix for Verilator 5.052 to every machine.

**Oracle / Testcases**

- [No loads from device addresses, stores allowed](docs/decisions.md#no-loads-from-device-addresses-stores-allowed) (2026-10-12):
  Spike has no devices, so a device load always mismatches.
- [Testcases as assembler, not compiled C](docs/decisions.md#testcases-as-assembler-not-compiled-c) (2026-10-12):
  full control over every byte and instruction.

**Classifier**

- [Classify by stdout, not exit code](docs/decisions.md#classify-by-stdout-not-exit-code) (2026-10-13):
  match, timeout and unloadable ELF all exit 0.
- [Second run on timeout with a bigger limit](docs/decisions.md#second-run-on-timeout-with-a-bigger-limit) (2026-10-14):
  the growth of the counters between both runs shows the cause.
- [Sleep, hang and loop via executed cycles](docs/decisions.md#sleep-hang-and-loop-via-executed-cycles) (2026-10-20):
  the clock gate pauses `mcycle` during sleep.
- [No wfi in the generator](docs/decisions.md#no-wfi-in-the-generator) (2026-10-13):
  every sleep is an RTL error.
- [Only forward jumps](docs/decisions.md#only-forward-jumps) (2026-10-13):
  programs can't loop, so every timeout is a finding.

**Runner**

- [One directory per testcase](docs/decisions.md#one-directory-per-testcase-one-environment-per-campaign) (2026-10-13):
  `.S` file and ELF are both kept, because the assembler decides the bytes.

## Gaps

- `mtime` cannot be read ([No loads from device addresses](docs/decisions.md#no-loads-from-device-addresses-stores-allowed))
- Bugs in waking the CPU stay hidden ([No wfi in the generator](docs/decisions.md#no-wfi-in-the-generator))

## Triages

- [0x30000: timer load in `hello_test`](docs/triages.md#0x30000-timer-load-in-hello_test) (2026-10-12):
  harness configuration, not an RTL or Spike bug.
