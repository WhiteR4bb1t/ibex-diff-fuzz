# Ibex-diff-fuzz

Ibex-diff-fuzz is a differential fuzzer for ibex - an open source RISC-V Core. It's
not a production-ready fuzzer, but rather a learning project.

## Status

- Classification of log files
- Classification of timeouts

- Tests for classify

## Usage

```
docker build -t fuzz-env -f docker/Dockerfile docker/
docker run --rm -it .:/work -v docker/bashrc:/tmp/.bashrc -w /work --user
$(id -u):$(id -g) -e HOME=/tmp fuzz-env
make
bin/fuzztests
```

## Decisions
### Environment
### Oracle / Testcases
### Classifier
- [Classify by stdout, not exit code](docs/classify-by-stdout-not-exit-code)
  (2026-10-13): match, timeout and unloadable ELF all exit 0.
### Runner

## Gaps



## Triages



