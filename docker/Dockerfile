FROM ubuntu:22.04@sha256:b8b6ee6aa931ecd9d0d952abc34dc0e5f7c6a30c6bb71b079fe399fde0329c02
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y git build-essential autoconf flex bison help2man \
    perl python3 ccache libfl2 libfl-dev zlib1g-dev libgoogle-perftools-dev numactl
RUN git clone --depth 1 -b v5.052 https://github.com/verilator/verilator /tmp/verilator \
 && cd /tmp/verilator && autoconf && ./configure --prefix=/tools/verilator \
 && make -j$(nproc) && make install && rm -rf /tmp/verilator
ENV PATH=/tools/verilator/bin:$PATH
RUN apt-get update && apt-get install -y device-tree-compiler libboost-system-dev \
    libboost-regex-dev pkg-config
ARG SPIKE_COMMIT=aadf648d742de54f0a50eec01ceffd13ed12a1d1
RUN git init /tmp/spike && cd /tmp/spike \
 && git fetch --depth 1 https://github.com/lowRISC/riscv-isa-sim $SPIKE_COMMIT \
 && git checkout FETCH_HEAD && mkdir build && cd build \
 && ../configure --enable-commitlog --enable-misaligned --prefix=/tools/spike-cosim \
 && make -j$(nproc) && make install && rm -rf /tmp/spike
ENV PKG_CONFIG_PATH=/tools/spike-cosim/lib/pkgconfig
