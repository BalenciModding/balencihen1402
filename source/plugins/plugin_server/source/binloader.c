/* BalenciHEN raw userland payload receiver. Firmware patches must already
 * permit executable mappings. This is not a kernel exploit or ELF loader.
 * Wire protocol: raw position-independent .bin, then TCP write shutdown. */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define BH_PAYLOAD_LIMIT (4u * 1024u * 1024u)
struct bh_job { void *code; size_t size; };
static void *bh_run_payload(void *arg) {
    struct bh_job *job = arg;
    void *(*entry)(void *) = (void *(*)(void *))job->code;
    entry(NULL);
    munmap(job->code, job->size);
    free(job);
    return NULL;
}
static void bh_receive(int fd) {
    unsigned char *buf = malloc(BH_PAYLOAD_LIMIT + 1u);
    size_t n = 0;
    struct timeval timeout = { 15, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (!buf) return;
    for (;;) {
        ssize_t got = recv(fd, buf + n, BH_PAYLOAD_LIMIT + 1u - n, 0);
        if (got < 0 && errno == EINTR) continue;
        if (got < 0) goto done;
        if (!got) break;
        n += (size_t)got;
        if (n > BH_PAYLOAD_LIMIT) goto done;
    }
    if (n < 5 || buf[0] != 0xe9) goto done;
    size_t size = (n + 0x3fffu) & ~(size_t)0x3fffu;
    void *code = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
    if (code == MAP_FAILED) goto done;
    memcpy(code, buf, n);
    if (mprotect(code, size, PROT_READ | PROT_EXEC) != 0) {
        munmap(code, size); goto done;
    }
    struct bh_job *job = malloc(sizeof(*job));
    if (!job) { munmap(code, size); goto done; }
    job->code = code; job->size = size;
    pthread_t thread;
    if (pthread_create(&thread, NULL, bh_run_payload, job) != 0) {
        munmap(code, size); free(job); goto done;
    }
    pthread_detach(thread);
    printf("BalenciHEN: dispatched %zu-byte raw payload\n", n);
done:
    free(buf);
}
void *pthread_binloader(void *unused) {
    (void)unused;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return NULL;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(fd, 2) != 0) {
        close(fd); return NULL;
    }
    printf("BalenciHEN: raw payload receiver listening on 9090\n");
    for (;;) {
        int client = accept(fd, NULL, NULL);
        if (client < 0) { if (errno == EINTR) continue; break; }
        bh_receive(client);
        close(client);
    }
    close(fd);
    return NULL;
}
