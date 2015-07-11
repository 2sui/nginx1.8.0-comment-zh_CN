
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef _NGX_QUEUE_H_INCLUDED_
#define _NGX_QUEUE_H_INCLUDED_

/*
 * 运行机制图参考 img/ngx_queue_t.gif (图片截取自http://blog.csdn.net/livelylittlefish/article/details/6607324)
 *
 * 其中双向链表表头的prev指针指向链表尾元素，表尾元素的next指针指向表头元素，形成了环形链表。
 * 每个双向列表都有一个哨兵节点，哨兵节点后才是数据节点。
 * 所有使用双向链表的数据结构都要包含一个ngx_queue_t元素，这样这个数据结构就能够通过ngx_queue_t这个元素
 * 自动组织链表；同时ngx_queue_t元素可以通过offsetof（即该成员相对于这个数据结构的首地址的偏移）来确定
 * 对应的数据结构其实地址（ngx_queue_t元素的地址减去它相对于对应数据结构的偏移就是这个数据结构的地址）
 * 其具备下列特点：
 * - 可以高效的执行插入、删除、合并等操作
 * - 具有排序功能
 * - 支持两个链表间的合并
 * - 支持将一个链表一分为二的拆分动作
*/

typedef struct ngx_queue_s  ngx_queue_t;
/* 双向链表 */
struct ngx_queue_s {
    ngx_queue_t  *prev;
    ngx_queue_t  *next;
};

/*
 * 初始化双向链表，prev和next都指向自己（空队列）
*/
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q

/*
 * 判断双向链表是否为空（即表头的prev是否指向自己，如果不为空的话
 * prev指针应指向双向链表最后一个元素）
*/
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)

/*
 * 向表头插入元素
*/
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x

/*
 * 插在某元素之后，与ngx_queue_insert_head一样，不过传入的参数不是哨兵元素而是某个需要被插入的元素位置
*/
#define ngx_queue_insert_after   ngx_queue_insert_head

/*
 * 向表尾插入元素
*/
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x

/*
 * 返回表中第一个数据元素
*/
#define ngx_queue_head(h)                                                     \
    (h)->next

/*
 * 返回表中最后一个数据元素
*/
#define ngx_queue_last(h)                                                     \
    (h)->prev

/*
 * 返回当前双向链表
*/
#define ngx_queue_sentinel(h)                                                 \
    (h)

/*
 * 返回队列中当前元素的下一个元素
*/
#define ngx_queue_next(q)                                                     \
    (q)->next

/*
 * 返回队列中当前元素的上一个元素
*/
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else
/*
 * 将元素从队列中移除
*/
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif

/*
 * h为队列头(即链表头指针)，将该队列从q节点将队列(链表)分割为两个队列(链表)，
 * q之后的节点组成的新队列的头节点为n
*/
#define ngx_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;          /* n的队尾指针指向队h列尾 */                                          \
    (n)->prev->next = n;         /* 将n队列尾的next指向n */                                             \
    (n)->next = q;               /* n->next指向q */                                             \
    (h)->prev = (q)->prev;        /*  h队列尾指针指向q的前一个元素 */                                            \
    (h)->prev->next = h;          /* n最后一个元素的next指向队h */                                            \
    (q)->prev = n;              /* q的prev指向n */

/*
 * 将队列n合并到h的末尾
*/
#define ngx_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

/*
 * 获取队列元素q所在的数据结构
*/
#define ngx_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


/*
 * 获取队列中部元素，若队列有奇数个(除哨兵节点外)节点，则返回中间的节点；若
 * 队列有偶数个节点，则返回后半个队列的第一个节点
*/
ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);
/*
 * 排序（排序规则为cmp指定的对调函数）
*/
void ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *));


#endif /* _NGX_QUEUE_H_INCLUDED_ */
