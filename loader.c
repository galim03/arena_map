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

// libbpf 1.1.0+에선 <bpf/xdp.h>가 필요할 수도 있음
//#include <bpf/xdp.h>

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
    struct bpf_object *obj = NULL;
    struct bpf_program *prog = NULL;
    int prog_fd = -1, arena_fd = -1;
    const char *xdp_prog_name = "xdp_prog";      // arena_xdp_kern.c에서 SEC("xdp")의 함수명
    const char *syscall_prog_name = "arena_init";      // arena_xdp_kern.c에서 SEC("syscall")의 함수명
    const char *o_filename = "arena_xdp_kern.o";
    const char *map_name = "arena_map";
    const char *iface = "ens1np0";

    // 1. 오브젝트 파일 열기
    obj = bpf_object__open_file(o_filename, NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }

    // 2. 오브젝트 로드 (커널로 프로그램/맵 등록)
    if (bpf_object__load(obj)) {
        fprintf(stderr, "Failed to load BPF object\n");
        bpf_object__close(obj);
        return 1;
    }

	struct bpf_program *init_prog = bpf_object__find_program_by_name(obj, syscall_prog_name);
    if (!init_prog) {
        fprintf(stderr, "Couldn't find syscall program %s\n", syscall_prog_name);
        bpf_object__close(obj);
        return 1;
    }
    int init_fd = bpf_program__fd(init_prog);
	printf("init_fd: %d\n", init_fd);
	
	run_ebpf_prog(init_fd); //syscall test run 호출

    // 3. arena_map FD 찾기
    //arena_fd = bpf_object__find_map_fd_by_name(obj, map_name);
    //size_t sz = 4096 * 2;
	struct bpf_map *map = bpf_object__find_map_by_name(obj, map_name);
	int fd = bpf_map__fd(map);
    printf("arena_fd %d\n", fd);
	/**
	void *arena_data = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fd < 0) {
        fprintf(stderr, "arena_map not found\n");
        bpf_object__close(obj);
        return 1;
    }
    // 4. Arena mmap 및 이지 fault-in
    //size_t sz = 4096 * 2;
    //uint8_t *ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, arena_fd, 0);
    //if (ptr == MAP_FAILED) {
    if (arena_data == MAP_FAILED) {
        perror("mmap");
        bpf_object__close(obj);
        return 1;
    }
    //++ptr;
	//ptr = 123;
	*/

    // 5. XDP 프로그램 FD 추출 (함수명 주의: arena_xdp_kern.c에서 함수명 확인!)
    prog = bpf_object__find_program_by_name(obj, xdp_prog_name);
    if (!prog) {
        fprintf(stderr, "Couldn't find program %s\n", xdp_prog_name);
        //munmap(ptr, sz);
        //munmap(arena_data, sz);
        bpf_object__close(obj);
        return 1;
    }
    prog_fd = bpf_program__fd(prog);

    // 6. NIC에 XDP 프로그램 attach
    int ifindex = if_nametoindex(iface);
    if (!ifindex) {
        fprintf(stderr, "Invalid interface: %s\n", iface);
        //munmap(ptr, sz);
        //munmap(arena_data, sz);
        bpf_object__close(obj);
        return 1;
    }

	/**
    // 여기서 bpf_xdp_attach 사용
    struct bpf_xdp_attach_opts opts;
    memset(&opts, 0, sizeof(opts));
    // opts의 세부 필드는 필요에 따라 조정 가능

    int ret = bpf_xdp_attach(ifindex, prog_fd, 0, &opts);
	*/
	__u32 xdp_flags = 0;
	xdp_flags |= XDP_FLAGS_DRV_MODE;
    int ret = bpf_xdp_attach(ifindex, prog_fd, xdp_flags, NULL);
    if (ret < 0) {
        perror("bpf_xdp_attach");
        //munmap(ptr, sz);
        //munmap(arena_data, sz);
        bpf_object__close(obj);
        return 1;
    }

    //printf("XDP loaded. Initial arena byte = %u\n", ptr[0]);
    //printf("XDP loaded. Initial arena byte = %p\n", arena_data);
    while (1) sleep(10);

    // detach/clean-up (생략)
    return 0;
}

