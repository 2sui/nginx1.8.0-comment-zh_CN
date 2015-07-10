
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/*
 * 创建内存池。
 * 内存池中各结构体关系如图(img/ngx_pool_t.png,截取自(http://tengine.taobao.org/)).
 * 传入的size至少要大于sizeof(ngx_pool_t),因为申请的空间中前sizeof(ngx_pool_t)用于
 * 存放ngx_pool_t结构，且剩余空间大小还需小于NGX_MAX_ALLOC_FROM_POOL.
 */
ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;

    /* 申请size大小内存，关于NGX_POOL_ALIGNMENT对齐 */
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }
    /*ngx_pool_t.d.last指向申请的内存减去sizeof(ngx_pool_t)的位置*/
    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size; /* ngx_pool_t.d.end指向申请的内存的末尾 */
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t); /* 更新可用于存储数据的空间的大小，所以传入的size至少要大于sizeof(ngx_pool_t) */
    /*
     * ngx_pool_t.max设为当前申请内存可用于存储数据的大小(size)。
     * 当size<NGX_MAX_ALLOC_FROM_POOL时ngx_pool_t.max取size,否则取NGX_MAX_ALLOC_FROM_POOL.
     * 所以ngx_pool_t.size最大为NGX_MAX_ALLOC_FROM_POOL，大于该值的内存就被浪费了。
    */
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}

/*
 * 释放内存池
*/
void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    /* 分别调用cleanup中指定的清理回掉函数进行资源释放操作 */
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

    /* 释放大内存块 */
    for (l = pool->large; l; l = l->next) {

        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);

        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif
    /* 释放内存池 */
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

/*
 * 重置内存池。
 */
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    /* 释放大内存块 */
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    /* 将每块内存池d.last指针指向存放数据部分 内存的起始地址 */
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}

/*
 * 申请size大小内存，申请的内存已对齐(关于NGX_ALIGNMENT对齐)。
 * 当size大于ngx_pool_t.max时会使用大内存块
*/
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;

    /* 当size大于ngx_pool_t.max时，将申请大内存块 */
    if (size <= pool->max) {

        p = pool->current;

        do {
            /* 对齐指针 */
            m = ngx_align_ptr(p->d.last, NGX_ALIGNMENT);

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }
            /* 当内存不足时（对齐后的指针+size 超过当前内存池空间，换用下块内存池 */
            p = p->d.next;

        } while (p);

        /* 如果后面没有内存池了，则申请新的ngx_pool_t结构（实际还是ngx_pool_data_t结构） */
        return ngx_palloc_block(pool, size);
    }

    /* 申请大内存块 */
    return ngx_palloc_large(pool, size);
}


/*
 * 申请size大小内存，非对齐
*/
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    ngx_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ngx_palloc_block(pool, size);
    }

    return ngx_palloc_large(pool, size);
}


static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

    /* 计算内存池需要的内存空间 */
    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

    /* 初始化新内存池结构 */
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    /*
     * 这里将新的内存池ngx_pool_t结构中除了ngx_pool_data_t以外的元素都规划到数据存储部分中。
     * ngx_pool_data_t中next字段虽然是ngx_pool_t型指针，但实际上使用还是作为ngx_pool_data_t来使用的，
     * 而作为ngx_pool_t型指针应该是为了在调用ngx_destroy_pool是方便遍历链表做回收操作。（整个ngx_pool_t链表中只有第一个ngx_pool_t
     * 是完整的ngx_pool_t结构）
    */
    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size; /* 前size空间被分配使用 */

    for (p = pool->current; p->d.next; p = p->d.next) {
        /* 将current指向最后一个申请失败次数小于4的ngx_pool_data_t（ngx_pool_t）结构*/
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}

/* 分配大数据块 */
static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    /* 申请size（>pool.max）大小的内存 */
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    /* 将申请的内存挂在pool.large的最末尾上，当已挂载数大于3时直接插新的 */
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    /* 从内存池中申请ngx_pool_large_t结构 */
    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    /* 在ngx_pool_large_t链表前插入 */
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/*
 * 申请关于特定值对齐的大内存块
*/
void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc(pool, sizeof(ngx_pool_large_t));
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

/*
 * 释放大内存块
*/
ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}

/*
 * 从内存池申请空间并复位
*/
void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/*
 * 向ngx_pool_t.cleanup结构添加cleanup
*/
ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    /* 申请一个ngx_pool_cleanup_t结构 */
    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        /* 为ngx_pool_cleanup_t.data字段申请size大小的空间 */
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;/* 将内存池中最后一个cleanup.next指向这个内存池的cleanup字段（即该内存池cleanup结构链的第一个结构） */

    p->cleanup = c; /* 新的cleanup添加在ngx_pool_t.cleanup后，即每次在连表头插入 */

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}

/*
 * 清理内存池中维护的文件类资源
*/
void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    /* 依次调用内存池中cleanup结构指定的资源回收回调函数，
     * 这里回掉函数只有指向ngx_pool_cleanup_file时才被调用(专门用来清理文件资源)，参数为cleanup.data
     */
    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

/*
 * 关闭文件资源回掉函数
*/
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}

/*
 * 删除文件资源回掉函数
*/
void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
