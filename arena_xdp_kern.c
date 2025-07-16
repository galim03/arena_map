#include <linux/types.h>
#include </home/eungga/linux-6.15.6/tools/include/uapi/linux/bpf.h>
#include <linux/btf.h>
#include <stdint.h>

//#include "bpf_arena_spin_lock.h" //살리기
#include <bpf/bpf_helpers.h>
//#include "bpf_helper_defs.h"
#include "bpf_arena_common.h"//살리기
//#include "vmlinux.h"

//struct arena_spinlock_t;

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
//int alloc_cnt = 0;

/**
//int arena_init(struct { __u32 cnt; } *ctx) {
SEC("syscall")
int arena_init(void *ctx) {
	if (!mem) {
		__arena void *p = bpf_arena_alloc_pages(&arena_map, NULL, 2, NUMA_NO_NODE, 0);
		if (!p) {
			bpf_printk("bpf_arena_alloc_pages error");
			return -1;
		}
		mem = p;
		bpf_printk("arena_init mem address: %p", mem);
		bpf_printk("arena_init p address: %p", p);
	}
	alloc_cnt++;
	return 0;
}
*/
//struct arena_spinlock_t __arena lock;
//struct __qspinlock __arena lock; //살리기

SEC("xdp")
int xdp_prog(struct xdp_md *ctx) {
    bpf_printk("xdp prog");
    if (mem == NULL) {
        my_kfunc_reg_arena(&arena_map);
		bpf_printk("not seein the memory");
        return XDP_PASS;
    }
    //my_kfunc_reg_arena(&arena_map);
    // 테스트: mem[0] 읽어서 무시
    //volatile uint8_t v = *(uint8_t *)mem[0];
	//e->counter = 1;
	//*(int *)mem = 5;
    //__arena uint8_t v = *(((uint8_t *)mem) + 1);
	__arena entry_t *e = mem; //__arena 를 사용해야 verifier가 arena 주소라고 인식함
    bpf_printk("xdp arena first byte = %p", e);
    //bpf_printk("arena alloc call cnt = %d", alloc_cnt);
	e->counter = 5;
    bpf_printk("arena int = %lld", e->counter);

    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
