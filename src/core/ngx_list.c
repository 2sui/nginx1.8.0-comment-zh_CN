
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/*
 * 创建链表
*/
ngx_list_t *
ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    ngx_list_t  *list;

    list = ngx_palloc(pool, sizeof(ngx_list_t));
    if (list == NULL) {
        return NULL;
    }

    /* 初始化链表，节点中数组元素大小为size，个数为n */
    if (ngx_list_init(list, pool, n, size) != NGX_OK) {
        return NULL;
    }

    return list;
}


/*
 * 向链表尾部追加元素，返回可用元素空间
*/
void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last;

    /* 如果最后一个节点的数组已满 */
    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */

        /* 新申请一个ngx_list_part_t */
        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        /* 给新申请的ngx_list_part_t的数组部分申请空间 */
        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    /* 定位到数组中未使用的位置 */
    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}
