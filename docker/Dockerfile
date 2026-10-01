FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y git build-essential autoconf flex bison help2man \
    perl python3 ccache libfl2 libfl-dev zlib1g-dev libgoogle-perftools-dev numactl
RUN git clone --depth 1 -b v5.052 https://github.com/verilator/verilator /tmp/verilator \
 && cd /tmp/verilator && autoconf && ./configure --prefix=/tools/verilator \
 && make -j$(nproc) && make install && rm -rf /tmp/verilator
ENV PATH=/tools/verilator/bin:$PATH
