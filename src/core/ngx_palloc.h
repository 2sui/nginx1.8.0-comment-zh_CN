
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;
/* 资源释放结构 */
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler; /* 回调函数指针 */
    void                 *data;   /* 传入回调函数的参数 */
    ngx_pool_cleanup_t   *next; /* 下一个资源释放结构 */
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;
/* 大数据块 */
struct ngx_pool_large_s {
    ngx_pool_large_t     *next; /* 下一个大数据块 */
    void                 *alloc; /* 大数据块存储内存地址 */
};

/* 小数据块 */
typedef struct {
    u_char               *last;  /* 当前数据块中未使用内存的其实位置 */
    u_char               *end;  /* 当前数据块结束位置 */
    ngx_pool_t           *next; /* 下个内存池 */
    ngx_uint_t            failed; /* 当前数据块分配请求失败次数 */
} ngx_pool_data_t;

/* 内存池 */
struct ngx_pool_s {
    ngx_pool_data_t       d;  /* 小数据块 */
    size_t                max;  /* 小数据块最大值,当要申请的内存大于max时， */
    ngx_pool_t           *current;  /* 当前内存池指针 */
    ngx_chain_t          *chain; /* 链表 */
    ngx_pool_large_t     *large; /* 大数据快 */
    ngx_pool_cleanup_t   *cleanup; /* 内存池资源释放结构 */
    ngx_log_t            *log; /* 日志 */
};


typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;


void *ngx_alloc(size_t size, ngx_log_t *log);
void *ngx_calloc(size_t size, ngx_log_t *log);

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
