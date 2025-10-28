/*
 leaky_kv_client.c

 单文件示例：程序自身不断发送（模拟）KV 请求，每次都会产生内存泄漏。
 - 默认行为：无限循环执行 SET key value（value 大小可配），不断分配新 cache entry 与 request log（均不释放）。
 - 可通过参数调整：循环次数（0 表示无限）、value 大小（KB）、打印间隔等。

 编译:
   gcc -g -O0 -Wall -o leaky_kv_client leaky_kv_client.c

 运行示例（在受控环境中）:
   ./leaky_kv_client 0 256 10000
   参数:
     arg1: iterations (0 = 无限)
     arg2: value_size_kb (每个 value 大小，KB)
     arg3: print_interval (每多少次打印一次统计)

 示例:
   # 无限循环，每条 value 256KB，每 10000 次打印一次统计
   ./leaky_kv_client 0 256 10000

 强烈建议：
 - 在 Docker 容器或 VM 中运行，或先用 ulimit 限制虚拟内存：
     ulimit -v 300000   # KB
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>

/* ---- 数据结构（与之前示例类似） ---- */
typedef struct CacheEntry {
    char *key;
    char *value;
    size_t vlen;
    struct CacheEntry *next;
} CacheEntry;

typedef struct RequestLog {
    char *raw;
    time_t ts;
    struct RequestLog *next;
} RequestLog;

/* 全局容器（永不释放，用于演示泄漏） */
static CacheEntry *g_cache = NULL;
static RequestLog *g_logs = NULL;

/* 统计 */
static uint64_t g_total_requests = 0;
static uint64_t g_total_cache_entries = 0;
static uint64_t g_total_leaked_bytes = 0;

/* ---- 工具函数 ---- */
static void print_stats(void) {
    /* 计算链表长度（可能比较慢，但偶尔打印一次可以接受） */
    size_t cache_count = 0;
    for (CacheEntry *e = g_cache; e; e = e->next) cache_count++;
    size_t log_count = 0;
    for (RequestLog *l = g_logs; l; l = l->next) log_count++;
    // fprintf(stderr,
    //     "[STATS] requests=%" PRIu64 " cache_entries=%zu log_nodes=%zu approx_leaked_bytes=%" PRIu64 "\n",
    //     g_total_requests, cache_count, log_count, g_total_leaked_bytes);
    fprintf(stderr,
        "[STATS] requests=%" PRIu64 " cache_entries=%zu log_nodes=%zu\n",
        g_total_requests, cache_count, log_count);
}

/* 记录请求 */
static void log_request(const char *raw) {
    size_t rawlen = strlen(raw);
    RequestLog *l = malloc(sizeof(RequestLog));
    if (!l) return;
    l->raw = malloc(rawlen + 1);
    if (l->raw) {
        memcpy(l->raw, raw, rawlen + 1);
    }
    l->ts = time(NULL);
    l->next = g_logs;
    g_logs = l;

    g_total_requests++;
    g_total_leaked_bytes += sizeof(RequestLog);
    if (l->raw) g_total_leaked_bytes += rawlen + 1;
}

/* 插入 cache */
static void cache_set(const char *key, const char *value, size_t vlen) {
    CacheEntry *e = malloc(sizeof(CacheEntry));
    if (!e) return;
    e->key = malloc(strlen(key) + 1);
    if (e->key) strcpy(e->key, key);
    e->value = malloc(vlen);
    if (e->value) memcpy(e->value, value, vlen);
    e->vlen = vlen;
    e->next = g_cache;
    g_cache = e;

    g_total_cache_entries++;
}

/* 简单格式化 key（例如 key0000001） */
static void make_key(char *buf, size_t buflen, uint64_t idx) {
    snprintf(buf, buflen, "key%012" PRIu64, idx);
}

/* 生成一个 value buffer（重复模式，大小为 vlen） */
static char* make_value(size_t vlen) {
    char *b = malloc(vlen);
    if (!b) return NULL;
    /* 用某些可见模式填充，以满足“写入物理内存”的演示需要 */
    for (size_t i = 0; i < vlen; ++i) b[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26];
    return b;
}

/* ---- 主循环：模拟高频 SET 请求并泄漏 ---- */
int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <iterations(0=infinite)> <value_kb> <print_interval>\n", argv[0]);
        fprintf(stderr, "Example: %s 0 256 10000\n", argv[0]);
        return 1;
    }

    uint64_t iterations = (uint64_t) atoll(argv[1]);
    size_t value_kb = (size_t) atoll(argv[2]);
    uint64_t print_interval = (uint64_t) atoll(argv[3]);

    if (value_kb == 0) value_kb = 256;
    size_t vlen = value_kb * 1024ULL;

    fprintf(stderr, "leaky_kv_client start: iterations=%" PRIu64 ", value_kb=%zu, print_interval=%" PRIu64 "\n",
            iterations, value_kb, print_interval);
    fprintf(stderr, "pid=%d (send SIGUSR1 to print stats anytime)\n", getpid());

    /* 安装 SIGUSR1 处理用于随时打印统计 */
    struct sigaction sa;
    sa.sa_handler = (void(*)(int))print_stats;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    uint64_t i = 0;
    uint64_t key_idx = 0;

    /* 事先创建一个 value 模板以减少每次生成的开销（但仍要 malloc copy 到 cache） */
    char *value_template = NULL;
    if (vlen > 0) {
        value_template = malloc(vlen);
        if (!value_template) {
            fprintf(stderr, "failed to allocate value_template of %zu bytes\n", vlen);
            return 1;
        }
        for (size_t j = 0; j < vlen; ++j) value_template[j] = 'x' + (j % 23);
    }

    while (iterations == 0 ? 1 : (i < iterations)) {
        /* 构造命令：SET key value */
        char keybuf[64];
        make_key(keybuf, sizeof(keybuf), key_idx++);

        /* 模拟请求文本（用于日志） */
        /* 注意：原始请求字符串长度在 log_request 中会计入泄漏 */
        char rawbuf[128];
        int rawlen = snprintf(rawbuf, sizeof(rawbuf), "SET %s <value%zuKB>\n", keybuf, value_kb);

        /* 记录请求（会 leak RequestLog 与 raw string） */
        log_request(rawbuf);

        /* 生成 value（这里我们直接使用模板并复制到 cache 中） */
        cache_set(keybuf, value_template, vlen);

        /* 可选：有时也模拟 GET（这里暂不做额外分配） */

        /* 周期性打印 */
        if ((i > 0 && (i % print_interval) == 0)) {
            print_stats();
            /* small sleep to let you observe memory growth in htop (可去掉以更快泄漏) */
            usleep(1000);
        }

        i++;

        /* 简单节流：每次循环稍微 sleep 一点（可调或设为 0）以控制增长速率 */
        /* usleep(100); */ /* 注释掉可以更快上涨 */
    }

    /* 程序通常不会到这里（会自然无限增长或终止），但为了完整性释放模板 */
    free(value_template);

    fprintf(stderr, "done main loop, exiting\n");
    return 0;
}
