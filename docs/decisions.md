# Decisions

## Environment
### Toolchain in docker image
*2026-10-01*
I had the problem, that different devices had different states of ibex. Using a
docker image solved it and also made it easier to install the fuzzer on new
devices where toolchain and requirements aren't installed.
### Ibex as submodule on my own fork
*2026-10-02*
Verilator 5.052 has a problem with loops per signal, not per bit. I had to add
`-Wno-UNOPTFLAT` to `ibex_cosim`, otherwise it wouldn't compile. This isn't a real
error and not a bug.
## Oracle / Testcases
### 0x30000 is a harness-problem, no RTL-Bug
*2026-10-12*
In the hello world test code ibex loads a value from 0x30000 where the timer device
is connected. The current time is returned. Spike has no device connected to
0x30000 so it reads memory which is initialized to 0. 
### No loads from device addresses, stores allowed
*2026-10-12*
Values loaded from devices are normal memory regions in spike because it has no devices connected. This leads to load mismatches if ibex has one connected. Stores are
allowed because the region is writable in spike and ibex and throws no mismatch.
Stores must be allowed for the simulation end via a store to 0x20008.
Cost: Mtime cannot be read.
### Testcases as assembler, not compiled C
*2026-10-12*
I need full control over each byte and every instruction to install specific
instructions (wfi) the `reset_handler` and the `.vectors` section. 
### All trapvectors jump to exit
*2026-10-12*
Because no traps should exist in my testcases each unwanted trap should stop
the execution instantly.
### Controltests with known output
*2026-10-12*
I need tests so i instantly see a problem in the `classify` or `timeout_classify`
functions.
### No writes to mcycle
*2026-10-21*
The timeout classification depends on `mcycle` and a manipulated version would
result in wrong classification of timeout runs.
## Classifier
### Classify by stdout, not exit code
*2026-10-13*
Measured: match, timeout and unloadable ELF all exit 0; only mismatch exits 1.
Exit 1 means "any `$fatal`/assertion", not specifically a cosim 
mismatch(`verilator_sim_ctrl.cc`, `WasSimulationSuccessful`). Cost: the classifier
depends on log strings and breaks if Ibex changes them. The logs in `tests/logs/`
catch that.
### Second run on timeout with bigger limit
*2026-10-14*
The simulation is deterministic so the growth of certain parameters of the logs
give information about the casue of the timeout.
### Sleep, hang and loop over executed cycles
*2026-10-20*
The clock-gate pauses `mcycle` during sleep. The offset between cycles and
executed cycles stayed constant over 10 run with different timeouts. If
exec_cycles - cycles is bigger than 5 the casue of the timeout is a sleep.
Otherwise if the retired instructions saty the same it is a hang otherwise a loop.
### No wfi in the generator
This leads to every sleep being an error in the rtl becasue the CPU cant 
naturally sleep.
Cost: Bugs in waking the CPU stay hidden
### Only forward jumps
*2026-10-13*
The limit for the runs can be calculated as the instruction count * worst latency
+ some constant. Every timeout should be a finding because there is no reason
for the program to loop.
### Unknown as separate category; the campaign continues
*2026-10-13*
One unknown log file should not end the whole camapign. Instead it should be saved
as an Unknown result and be reviewed by the user.
### ElfError seperated from Unknown
*2026-10-14*
ElfError is an error in the generator or linker, Unknown is an unknown simulator
output.
### Counter as optional
*2026-10-14*
"Missing" is not the same as 0. Mismatch logs don't even have certain counters.
A 0 as missing would lead to misunderstandings with a real 0.
## Runner
### Runner in c++, sim as extern process
*2026-10-13*
I want to improve my skills in C++. And it is easier to use the sim as an extern
process. Thinking longterm i'd rather write my own harness to control both ibex
and spike to get further information but just the information of the logs.
### One directory per testcase; one environment per camapign
*2026-10-13*
The specific assembler decides which bytes are generated from the .S file. So both
the ELF and the .S file are saved. Uninteresting testcases get cleaned up by the
fuzz loop.
