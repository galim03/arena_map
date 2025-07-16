#include <linux/types.h>
#include </home/eungga/linux-6.15.6/tools/include/uapi/linux/bpf.h>
#include <linux/btf.h>
#include <stdint.h>

#include <bpf/bpf_helpers.h>
//#include "bpf_helper_defs.h"
#include "bpf_arena_common.h"

long my_kfunc_reg_arena(void *map) __ksym;

typedef struct {
	__u64 counter;
} entry_t;

struct {
    __uint(type, BPF_MAP_TYPE_ARENA);
    __uint(map_flags, BPF_F_MMAPABLE);
    __uint(max_entries, 2);       // 최대 페이지 2개
} arena_map SEC(".maps");

__arena void *mem = NULL; //포인터가 arena를 가리킨다는 걸 나타내기 위해 사용
int alloc_cnt = 0;

SEC("syscall")
//int arena_init(struct { __u32 cnt; } *ctx) {
int arena_init(void *ctx) {
	bpf_printk("arena init__");
	if (!mem) {
		__arena void *p = bpf_arena_alloc_pages(&arena_map, NULL, 2, NUMA_NO_NODE, 0);
		if (!p) {
			bpf_printk("bpf_arena_alloc_pages error");
			return -1;
		}
		else {
			bpf_printk("arena init bpf_arena_alloc_pages success!");
			mem = p;
			//*((int *)mem) = 5;
			bpf_printk("sys arena first bytes = %p", mem);
			//bpf_printk("sys arena first bytes = %p", p);
		}
	}
	
	//bpf_printk("sys arena first bytes = %p", mem);
	alloc_cnt++;
	return 0;
}

char LICENSE[] SEC("license") = "GPL";
