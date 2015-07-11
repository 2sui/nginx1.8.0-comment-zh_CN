
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;
/* 链表节点 */
struct ngx_list_part_s {
    void             *elts; /* 数组元素的起始内存地址 */
    ngx_uint_t        nelts; /* 数组元素已有的数量 */
    ngx_list_part_t  *next; /* 下一个链表节点 */
};

/*
 * 链表
 * 每个节点数据空间中类似一个数组，所以相对于常规链表查询速度会快很多。所以初始化
 * 链表时需要指定每个链表元素的数组中每个数组元素大小及数组元素个数
*/
typedef struct {
    ngx_list_part_t  *last;  /* 指向最后一个节点 */
    ngx_list_part_t   part;  /* 首个链表节点 */
    size_t            size;  /* 链表元素的数组中的元素大小 */
    ngx_uint_t        nalloc; /* 链表元素的数组中的最大容量 */
    ngx_pool_t       *pool;
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

/*
 * 初始化链表，传入的list一定要未初始化过
 * ngx_list_t是没有释放链表操作的
*/
static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    /* 为节点中数组申请空间，每个数组元素大小为size，申请n个 */
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    /* 初始化赋值 */
    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
