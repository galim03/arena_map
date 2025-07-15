.SUFFIXES:
KERNEL_SRC ?= /home/eungga/linux-6.15.6

BPF_CLANG   ?= clang
#BPF_CLANG   ?= /home/eungga/llvm/llvm18.1.7/llvm-project/llvm/build/bin/clang
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

# CO-RE object
BPF_OBJ = arena_xdp_kern.o

all: loader xdp_object

xdp_object: $(BPF_OBJ)

#%.o: %.c
#	$(BPF_CLANG) $(BPF_CFLAGS) $(LIBBPF_CFLAGS) -c $< -o $@.ll
#	$(BPF_LLC) -march=bpf -filetype=obj -o $@ $@.ll

# BPF 전용 빌드 플로우
%.ll: %.c
	$(BPF_CLANG) $(BPF_CFLAGS) $(LIBBPF_CFLAGS) -S -emit-llvm $< -o $@

%.o: %.ll
	$(BPF_LLC) -march=bpf -filetype=obj -o $@ $<

loader: loader.c $(BPF_OBJ)
	gcc loader.c -o loader $(LIBBPF_LDFLAGS) -lelf -lz

clean:
	rm -f *.o *.ll loader

.PHONY: all clean

