FROM debian:12-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
 gcc-12 \
 file binutils libc-dev libgcc-12-dev \
 make cmake pkg-config \
 git curl ca-certificates \
 python3 tcl-dev bison flex re2c \
 libcurl4-openssl-dev libssl-dev libexpat1-dev zlib1g-dev libicu-dev \
 libncurses-dev libreadline-dev libpsl-dev libffi-dev libxml2-dev libsqlite3-dev \
 libgmp-dev libmpfr-dev libmpc-dev \
 autoconf autopoint automake gettext texinfo

COPY . /work/slimcc
WORKDIR /work/slimcc

RUN gcc-12 -O3 -flto=auto -march=native *.c -fsanitize=address -o slimcc_asan
RUN apt-get -y autoremove gcc-12 && apt-get clean

ENV ASAN_OPTIONS=detect_leaks=0
ENV CC=/work/slimcc/slimcc_asan

RUN useradd -m non-root -s /bin/bash && \
 su non-root -c "git config --global advice.detachedHead false" && \
 mv scripts/linux*.bash /home/non-root

WORKDIR /home/non-root
