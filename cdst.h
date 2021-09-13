/*
Copyright (c) 2021 Christian Döring
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef CDST_H
#define CDST_H

#include <stddef.h>
#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif //NULL

#ifndef container_of
#define container_of(_ptr, _type, _member) ((_type *)((char*)(_ptr)-(char*)(&((_type*)0)->_member)))
#endif //container_of

#ifndef CDST_MEMCPY
#include <string.h>
#define CDST_MEMCPY(_dst_p, _src_p, _size) memcpy(_dst_p, _src_p, _size)
#endif //CDST_MEMCPY

#ifndef MIN
#define MIN(_x, _y) (((_x) <= (_y)) ? (_x) : (_y))
#endif //MIN

#ifndef MAX
#define MAX(_x, _y) (((_x) >= (_y)) ? (_x) : (_y))
#endif //MAX

#define buf_of(_src) buf((uint8_t *)&(_src), sizeof(_src))
#define buf_ofarr(_arr) buf((uint8_t *)(_arr), sizeof(_arr))

#define stack_ofarr(_arr) stack(buf_ofarr(_arr));
#define fifo_ofarr(_arr) stack(buf_ofarr(_arr))

/*
 * Buf describes a area of memory.
 */
struct buf{
    uint8_t *mem;
    size_t size;
};

struct fifo{
    struct buf buf;
    size_t tail, head;
};

struct stack{
    struct buf buf;
    size_t size;
};

static inline struct buf buf(uint8_t *buf, size_t size){
    return (struct buf){.mem = buf, .size = size};
}

static inline struct fifo fifo(struct buf buf){
    return (struct fifo){.buf = buf, .tail = 0, .head = 0};
}

static inline struct stack stack(struct buf buf){
    return (struct stack){.buf = buf, .size = 0};
}

static inline size_t stack_push(struct stack *dst, const struct buf src){
    if(dst->size + src.size > dst->buf.size)
        return 0;
    CDST_MEMCPY(&dst->buf.mem[dst->size], src.mem, src.size);
    dst->size += src.size;
    return src.size;
}
static inline size_t stack_pop(struct stack *src, struct buf dst){
    if(src->size < dst.size)
        return 0;
    CDST_MEMCPY(dst.mem, &src->buf.mem[src->size - dst.size], dst.size);
    src->size -= dst.size;
    return dst.size;
}
static inline size_t stack_size(struct stack *src){
    return src->size;
}

static inline size_t fifo_size(const struct fifo *src){
    return src->head >= src->tail ? src->head - src->tail : src->head + (src->buf.size - src->tail);
}
static inline size_t fifo_push(struct fifo *dst, const struct buf src){
    if(fifo_size(dst) + src.size >= dst->buf.size)
        return 0;
    if(dst->head + src.size >= dst->buf.size){
        CDST_MEMCPY(&dst->buf.mem[dst->head], src.mem, dst->buf.size - dst->head);
        CDST_MEMCPY(&dst->buf.mem[0], &src.mem[dst->buf.size - dst->head], src.size - (dst->buf.size - dst->head));
    }
    else{
        CDST_MEMCPY(&dst->buf.mem[dst->head], src.mem, src.size);
    }
    dst->head = (dst->head + src.size) % dst->buf.size;
    return src.size;
}
static inline size_t fifo_peek(struct fifo *src, struct buf dst){
    if(fifo_size(src) < dst.size)
        return 0;
    if(src->tail + dst.size >= src->buf.size){
        CDST_MEMCPY(&dst.mem[0], &src->buf.mem[src->tail], src->buf.size -src->tail);
        CDST_MEMCPY(&dst.mem[src->buf.size - src->tail], &src->buf.mem[0], dst.size - (src->buf.size - src->tail));
    }
    else{
        CDST_MEMCPY(dst.mem, &src->buf.mem[src->tail], dst.size);
    }
    return dst.size;
}
static inline size_t fifo_pop(struct fifo *src, struct buf dst){
    if(fifo_peek(src, dst) <= 0)
        return 0;
    src->tail = (src->tail + dst.size) % src->buf.size;
    return dst.size;
}



/*
 * Iterate over the list with external pointer.
 *
 * @param _list_p: pointer to the list to iterate over
 * @param _iter_p: iterator pointer.
 */
#define dlist_foreach(_list_p, _iter_p)\
    for((_iter_p) = (_list_p)->next; (_iter_p) != (_list_p); (_iter_p) = (_iter_p)->next)

/*
 * Iterate over the list in reverse with external pointer.
 *
 * @param _list_p: pointer to the list to iterate over
 * @param _iter_p: iterator pointer.
 */
#define dlist_foreach_reverse(_list_p, _iter_p)\
    for((_iter_p) = (_list_p)->next; (_iter_p) != (_list_p); (_iter_p) = (_iter_p)->next)

/*
 * Iterate over the list and pop each element with external pointer.
 *
 * @param _list_p: pointer to the list to iterate over
 * @param _iter_p: iterator pointer.
 */
#define dlist_popeach(_list_p, _iter_p)\
    for((_iter_p) = dlist_pop((_list_p)->next); (_iter_p) != (_list_p); (_iter_p) = dlist_pop((_list_p)->next))

/*
 * Iterate over the list in reverse and pop each element with external pointer.
 *
 * @param _list_p: pointer to the list to iterate over
 * @param _iter_p: iterator pointer.
 */
#define dlist_popeach_reverse(_list_p, _iter_p)\
    for((_iter_p) = dlist_pop((_list_p)->prev); (_iter_p) != (_list_p); (_iter_p) = dlist_pop((_list_p)->prev))

/*
 * dlist is the node as well as the head of a doubly linked list.
 */
struct dlist{
    void *cont;
    struct dlist *next, *prev;
};

/*
 * dlist_init initializes list (next = prev = self)
 *
 * @param dst: pointer to the node
 * @param cont: pointer to the container struct NULL in case of list_head
 * @return: dst
 */
static inline struct dlist *dlist_init(struct dlist *dst, void *cont){
    dst->cont = cont;
    dst->next = dst;
    dst->prev = dst;
    return dst;
}

/*
 * initializes dlist as head
 *
 * @param dst: pointer to the lsit to be initialized
 * @return: dst
 */
static inline struct dlist *dlist_head_init(struct dlist *dst){
    return dlist_init(dst, NULL);
}

/*
 * dlist_pop pops the target node from the list
 *
 * if the target is the list then it will not change it
 *
 * @param target to be poped from the list
 * @return target if success NULL else
 */
static inline struct dlist *dlist_pop(struct dlist *target){
    if(target != NULL && target->prev != NULL && target->next != NULL){
        target->prev->next = target->next;
        target->next->prev = target->prev;
        return target;
    }
    return NULL;
}

/*
 * dlist_push_after pushes node after the dst node in the list
 *
 * @param dst destination node after which to push src
 * @param src source node to push after dst
 * @return src if success NULL else
 */
static inline struct dlist *dlist_push_after(struct dlist *dst, struct dlist *src){
    if(src != NULL && dst != NULL && dst->next != NULL){
        dst->next->prev = src;
        src->next = dst->next;
        dst->next = src;
        src->prev = dst;
        return src;
    }
    return NULL;
}

/*
 * dlist_push_before pushes node before the dst node in the list
 *
 * @param dst destination node before which to push src
 * @param src source node to push before dst
 * @return src if success NULL else
 */
static inline struct dlist *dlist_push_before(struct dlist *dst, struct dlist *src){
    if(src != NULL && dst != NULL && dst->prev != NULL){
        dst->prev->next = src;
        src->prev = dst->prev;
        dst->prev = src;
        src->next = dst;
        return src;
    }
    return NULL;
}

/*
 * dlist_push_back pushes node at the back of the list
 *
 * @param self pointer to the list
 * @param src list node to be pushed to the lsit
 * @return returns self if succes, NULL else
 */
static inline struct dlist *dlist_push_back(struct dlist *self, struct dlist *src){
    return dlist_push_after(self->prev, src);
}

/*
 * dlist_push_back pushes node at the front of the list
 *
 * @param self pointer to the list
 * @param src list node to be pushed to the lsit
 * @return returns self if succes, NULL else
 */
static inline struct dlist *dlist_push_front(struct dlist *self, struct dlist *src){
    return dlist_push_before(self->next, src);
}

/*
 * dlist_splice_after inserts the nodes of src after the dst node. Src will be empty.
 *
 * @param dst node after which to insert the nodes of src
 * @param src list, whichs nodes are to be inserted after dst
 * @return first element that has been inserted, if src is empty dst
 */
static inline struct dlist *dlist_splice_after(struct dlist *dst, struct dlist *src){
    if(dst != NULL && src != NULL && src->next != src){
        dst->next->prev = src->prev;
        src->prev->next = dst->next;
        src->next->prev = dst;
        dst->next = src->next;
        src->next = src->prev = src;
    }
    return dst->next;
}

/*
 * dlist_splice_before inserts the nodes of src before the dst node
 *
 * @param dst node after which to insert the nodes of src
 * @param src list, which nodes are to be inserted after dst
 * @retrun returns last inserted element if src is empty returns dst
 */
static inline struct dlist *dlist_splice_before(struct dlist *dst, struct dlist *src){
    if(dst != NULL && src != NULL && src->next != src){
        dst->prev->next = src->next;
        src->next->prev = dst->prev;
        src->prev->next = dst;
        dst->prev = src->prev;
        src->next = src->prev = src;
    }
    return dst->next;
}

/*
 * dlist_length returns length of list
 *
 * @param self pointer to list
 * @return length of list returns 0 if empty
 */
static inline size_t dlist_length(struct dlist *self){
    size_t i = 0;
    struct dlist *n;
    dlist_foreach(self, n) i++;
    return i;
}

/*
 * dlist_reverse reverses the content of a list
 *
 * @param self pointer to list that should be reversed
 * @return void
 */
static inline void dlist_reverse(struct dlist *self){
    struct dlist *tmp = self->next;
    self->next = self->prev;
    self->prev = tmp;
    for(struct dlist *node = self->prev; node != self; node = node->prev){
        tmp = node->next;
        node->next = node->prev;
        node->prev = tmp;
    }
}


#define slist_foreach(_list_p, _iter)\
    for(struct slist *_iter = (_list_p)->next; (_iter) != NULL; (_iter) = (_iter)->next)

#define slist_foreach_ext(_list_p, _iter_p)\
    for((_iter_p) = (_list_p)->next; (_iter_p) != NULL; (_iter_p) = (_iter_p)->next)

#define slist_popeach(_list_p, _iter)\
    for(struct slist *_iter = slist_pop_after(_list_p);_iter != NULL && _iter = slist_pop_after(_list_p))

#define slist_popeach_ext(_list_p, _iter_p)\
    for((_iter_p) = slist_pop_after(_list_p);(_iter_p) != NULL && (_iter_p) = slist_pop_after(_list_p))

struct slist{
    struct slist *next;
    void *cont;
};

static inline struct slist *slist_init(struct slist *dst, void *cont){
    dst->next = NULL;
    dst->cont = cont;
    return dst;
}

static inline struct slist *slist_pop_after(struct slist *src){
    if(src != NULL && src->next != NULL){
        struct slist *tmp = src->next;
        src->next = src->next->next;
        return tmp;
    }
    return NULL;
}

static inline struct slist *slist_push_after(struct slist *dst, struct slist *src){
    if(src != NULL && dst != NULL){
        src->next = dst->next;
        dst->next = src;
        return src;
    }
    return NULL;
}

static inline struct slist *slist_push_front(struct slist *self, struct slist *src){
    return slist_push_after(self, src);
}

static inline size_t slist_length(struct slist *self){
    size_t i = 0;

    return i;
}


#endif //CDST_H
