#include <bpf/libbpf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <net/if.h>

#include <bpf/bpf.h>

#include <sys/syscall.h>

#include <signal.h>

#include "arena_xdp_kern.skel.h"
#include "arena_syscall_kern.skel.h"
// libbpf 1.1.0+에선 <bpf/xdp.h>가 필요할 수도 있음
//#include <bpf/xdp.h>

static volatile bool exiting = false;

//struct arena_syscall_kern *sskel = NULL;
//struct arena_xdp_kern *xskel = NULL;

static void sig_handler(int sig)
{
    exiting = true;
}

static int run_ebpf_prog(int prog_fd)
{
    /* Look here: https://docs.kernel.org/bpf/bpf_prog_run.html */
    /* time_t before, after; */
    int ret;
    struct bpf_test_run_opts test_opts;
    memset(&test_opts, 0, sizeof(struct bpf_test_run_opts));
    /* TODO: optionally pass a context to it */
    test_opts.sz = sizeof(struct bpf_test_run_opts);
    test_opts.data_in = NULL;
    test_opts.data_size_in = 0;
    test_opts.ctx_in = NULL;
    test_opts.ctx_size_in = 0;
    test_opts.repeat = 0;
    test_opts.cpu = 0;
    /* before = read_tsc(); */
    ret = bpf_prog_test_run_opts(prog_fd, &test_opts); //BPF_PROG_TEST_RUN wrapper임
    /* after = read_tsc(); */
    if (ret < 0) {
        perror("something went wrong\n");
        return -1;
    }
    /*last_test_duration = test_opts.duration;*/
    return test_opts.retval;
}

int main(int argc, char **argv) {
    const char *iface = "ens1np0";
	
	signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int ifindex = if_nametoindex(iface);
    if (!ifindex) {
        fprintf(stderr, "Invalid interface: %s\n", iface);
        return 1;
    }
	
	struct arena_syscall_kern *sskel = NULL;
	struct arena_xdp_kern *xskel = NULL;
	
	//skel을 위한 추가
	sskel = arena_syscall_kern__open();
	if(!sskel) {
		fprintf(stderr, "Failed to open syscall skeleton");
		return 1;
	}
	arena_syscall_kern__load(sskel);
	
	xskel = arena_xdp_kern__open();
	if(!xskel) {
		fprintf(stderr, "Failed to open xdp skeleton");
		return 1;
	}

	//bpf_program__set_flags(sskel->progs.arena_init, BPF_F_SLEEPABLE);

	int syscall_prog_fd = bpf_program__fd(sskel->progs.arena_init);
	printf("syscall prog fd = %d\n", syscall_prog_fd);
	run_ebpf_prog(syscall_prog_fd); // TEST 한번 실행
	
	//int xdp_prog_fd = bpf_program__fd(xskel->progs.xdp_prog);
	//printf("xdp prog fd = %d\n", xdp_prog_fd);

	int arena_fd = bpf_map__fd(sskel->maps.arena_map);
	printf("syscall arena map fd = %d\n", arena_fd);
	if(bpf_map__reuse_fd(xskel->maps.arena_map, arena_fd) < 0) {
		printf("Failed to reuse map fd\n");
		return 1;
	}
	//bpf_map__reuse_fd(xskel->maps.arena_map, 3);
	xskel->bss->mem = sskel->bss->mem;

	//adding for test
	int xdp_arena_fd = bpf_map__fd(xskel->maps.arena_map);
	printf("xdp arena map fd = %d\n", xdp_arena_fd);
	
	if(arena_xdp_kern__load(xskel)) {
		fprintf(stderr, "Failed to load xdp program\n");
		//arena_syscall_kern__detach(sskel);
		arena_syscall_kern__destroy(sskel);
		return 1;
	}
	
	int xdp_prog_fd = bpf_program__fd(xskel->progs.xdp_prog);
	printf("xdp prog fd = %d\n", xdp_prog_fd);
	
	__u32 xdp_flags = 0;
	xdp_flags |= XDP_FLAGS_DRV_MODE;
    int ret = bpf_xdp_attach(ifindex, xdp_prog_fd, xdp_flags, NULL);
    if (ret < 0) {
        perror("bpf_xdp_attach");
        //munmap(ptr, sz);
        //munmap(arena_data, sz);
        //bpf_object__close(sys_obj);
        //bpf_object__close(xdp_obj);
        return 1;
    }
	else {
		printf("program attach %d\n", ifindex);
	}
    
	while (!exiting) sleep(10);

	bpf_xdp_detach(ifindex, xdp_flags, NULL);
	//bpf_object__close(xdp_obj);
    //bpf_object__close(sys_obj);

	printf("program detach %d\n", ifindex);
    
	return 0;
}

