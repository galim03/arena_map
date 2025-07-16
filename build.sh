#!/bin/bash
set -e

/home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/clang -O2 -target bpf -g -I/home/eungga/libbpf/src -I/home/eungga/linux-6.15.6/usr/include -I/home/eungga/linux-6.15.6/include/linux -I/home/eungga/linux-6.15.6/tools/include/uapi -I/home/eungga/libbpf/include -I/home/eungga/libbpf/include/uapi  -S -emit-llvm arena_xdp_kern.c -o arena_xdp_kern.ll
/home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/llc -march=bpf -filetype=obj -o arena_xdp_kern.o arena_xdp_kern.ll
/home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/clang -O2 -target bpf -g -I/home/eungga/libbpf/src -I/home/eungga/linux-6.15.6/usr/include -I/home/eungga/linux-6.15.6/include/linux -I/home/eungga/linux-6.15.6/tools/include/uapi -I/home/eungga/libbpf/include -I/home/eungga/libbpf/include/uapi  -S -emit-llvm arena_syscall_kern.c -o arena_syscall_kern.ll
/home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/llc -march=bpf -filetype=obj -o arena_syscall_kern.o arena_syscall_kern.ll
/usr/local/sbin/bpftool gen skeleton arena_xdp_kern.o > arena_xdp_kern.skel.h
/usr/local/sbin/bpftool gen skeleton arena_syscall_kern.o > arena_syscall_kern.skel.h
gcc loader.c -o loader -L/home/eungga/libbpf/src -lbpf

echo "build success!"
