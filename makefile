.SUFFIXES:
KERNEL_SRC ?= /home/eungga/linux-6.15.6

#BPF_CLANG   ?= clang
#BPF_CLANG   ?= /home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/clang
BPF_CLANG   ?= clang-18
BPF_LLC     ?= llc
BPF_CFLAGS  := -O2 -target bpf -g
LIBBPF_DIR  ?= /home/eungga/libbpf
LIBBPF_CFLAGS += \
				 -I$(LIBBPF_DIR)/src \
				 -I$(KERNEL_SRC)/usr/include \
				 -I$(KERNEL_SRC)/include/linux \
				 -I$(KERNEL_SRC)/tools/include/uapi \
				 -I$(LIBBPF_DIR)/include \
				 -I$(LIBBPF_DIR)/include/uapi 

LIBBPF_LDFLAGS := -L$(LIBBPF_DIR)/src -lbpf
#export LD_LIBRARY_PATH := $(LIBBPF_DIR):$(LD_LIBRARY_PATH)

#KERNEL_SRC ?= /home/eungga/linux-6.15.6# Galim add

BPF_SRCS := arena_xdp_kern.c arena_syscall_kern.c
BPF_OBJS := $(BPF_SRCS:.c=.o)

all: loader $(BPF_OBJS)

# BPF IR 생성 (.ll)
%.ll: %.c
	$(BPF_CLANG) $(BPF_CFLAGS) $(LIBBPF_CFLAGS) -S -emit-llvm $< -o $@

# BPF ELF .o 생성
%.o: %.ll
	$(BPF_LLC) -march=bpf -filetype=obj -o $@ $<

# loader 바이너리 생성 (BPF .o는 링크하지 않음)
loader: loader.c
	gcc loader.c -o $@ $(LIBBPF_LDFLAGS)

clean:
	rm -f *.ll *.o loader

.PHONY: all clean
