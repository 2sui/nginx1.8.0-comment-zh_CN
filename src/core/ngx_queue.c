
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */

ngx_queue_t *
ngx_queue_middle(ngx_queue_t *queue)
{
    ngx_queue_t  *middle, *next;

    /* middle指向第一个个元素 */
    middle = ngx_queue_head(queue);

    /* 队列为空或只有一个元素 */
    if (middle == ngx_queue_last(queue)) {
        return middle;
    }

    next = ngx_queue_head(queue);

    /*
     * 这里next移动速度是middle的2倍，所以当next指向队尾时middle正好指向中间
    */
    for ( ;; ) {
        middle = ngx_queue_next(middle);

        next = ngx_queue_next(next);

        /* 如果next到队尾，返回middle */
        if (next == ngx_queue_last(queue)) {
            return middle;
        }

        /* 否则next继续指向下一个元素 */
        next = ngx_queue_next(next);

        /* 如果新next指向队尾， 则返回 */
        if (next == ngx_queue_last(queue)) {
            return middle;
        }
    }
}


/* the stable insertion sort */

void
ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
    ngx_queue_t  *q, *prev, *next;

    q = ngx_queue_head(queue);

    if (q == ngx_queue_last(queue)) {
        return;
    }

    for (q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {

        /* 保存q的前后元素 */
        prev = ngx_queue_prev(q);
        next = ngx_queue_next(q);

        /* 先将q从队列取出 */
        ngx_queue_remove(q);

        do {
            /* 与q前一个元素比较，如果<=0 表示符合排序顺序 */
            if (cmp(prev, q) <= 0) {
                break;
            }

            /* 取q前一个元素之前的元素，知道找到符合排序顺序的正确位置 */
            prev = ngx_queue_prev(prev);

        } while (prev != ngx_queue_sentinel(queue));

        /* 将q插在正确位置之后 */
        ngx_queue_insert_after(prev, q);
    }
}
