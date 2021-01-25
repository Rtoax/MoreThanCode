/*
   Copyright (C) 2007,2008 Qualcomm Incorporated. All rights reserved.
   Written by Max Krasnyansky <maxk@qualcomm.com>

   This file is part the Bones library. It is licensed under
   Boost Software License - Version 1.0 - August 17th, 2003

   Permission is hereby granted, free of charge, to any person or organization
   obtaining a copy of the software and accompanying documentation covered by
   this license (the "Software") to use, reproduce, display, distribute,
   execute, and transmit the Software, and to prepare derivative works of the
   Software, and to permit third-parties to whom the Software is furnished to
   do so, all subject to the following:

   The copyright notices in the Software and this entire statement, including
   the above license grant, this restriction and the following disclaimer,
   must be included in all copies of the Software, in whole or in part, and
   all derivative works of the Software, unless such copies or derivative
   works are solely in the form of machine-executable object code generated by
   a source language processor.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/
#include <stdint.h>
#include <assert.h>
    
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "nanomq.h"


// POD for header data
struct nmq_header {
    unsigned int nodes;
    unsigned int rings;
    unsigned int size;
    size_t msg_size;
};

// POD for nmq_ring
struct nmq_ring {
    unsigned int _size;
    size_t _msg_size;
    size_t _offset;

    char _pad1[128];
    // R/W access by the reader
    // R/O access by the writer
    volatile unsigned int _head;

    char _pad2[128];    
    // R/W access by the writer
    // R/O access by the reader
    volatile unsigned int _tail;
};


/**
 * Compiler barrier
 */
static inline void force_inline comp() { asm volatile("": : :"memory"); }

// Memory barriers
// This version requires SSE capable CPU.
static inline void force_inline memrw() { asm volatile("mfence":::"memory"); }
static inline void force_inline memr()  { asm volatile("lfence":::"memory"); }
static inline void force_inline memw()  { asm volatile("sfence":::"memory"); }

// mfence ensures that tsc reads are properly serialized
// On Intel chips it's actually enough to just do lfence but
// that would require some conditional logic. 

#if defined __x86_64__
static inline uint64_t __tsc(void)
{
   unsigned int a, d;
   asm volatile ("rdtsc" : "=a" (a), "=d"(d));
   return ((unsigned long) a) | (((unsigned long) d) << 32);
}

#else

static inline uint64_t __tsc(void)
{
   uint64_t c;
   asm volatile ("rdtsc" : "=A" (c));
   return c;
}

#endif

static inline void __relax()  { asm volatile ("pause":::"memory"); } 
static inline void __lock()   { asm volatile ("cli" ::: "memory"); }
static inline void __unlock() { asm volatile ("sti" ::: "memory"); }



force_inline static unsigned int power_of_2(unsigned int size) {
    unsigned int i;
    for (i=0; (1U << i) < size; i++);
    return 1U << i;
}


force_inline  bool ctx_create(struct nmq_context *self, 
#ifndef ANONYMOUS
                                const char *fname, 
#endif
                                unsigned int nodes, unsigned int size, unsigned int msg_size) {

    int fd = 0;

#ifndef ANONYMOUS
    strncpy(self->fname_, fname, CTX_FNAME_LEN);

    fd = open(self->fname_, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
    if (fd == -1) 
        return false;
#endif

    unsigned int i;
    unsigned int real_size = power_of_2(size);
    unsigned int n_rings = 2*(nodes * (nodes - 1)) / 2;
    unsigned int file_size = sizeof(struct nmq_header) + sizeof(struct nmq_ring)*n_rings + n_rings*real_size*msg_size;
    int flags = MAP_SHARED|MAP_ANONYMOUS;
    
#ifndef ANONYMOUS
    if (ftruncate(fd, file_size) == -1) 
        return false;
    
    flags &= ~MAP_ANONYMOUS;
#endif

    self->p_ = mmap(NULL, file_size, PROT_READ|PROT_WRITE, flags, fd, 0);
    if (self->p_ == NULL) 
        return false;

    memset(self->p_, 0, file_size);

    self->header_ = (struct nmq_header*)self->p_;
    self->ring_ = (struct nmq_ring*)(self->header_ + 1);
    self->data_ = (char*)(self->ring_ + n_rings);

    self->header_->nodes = nodes;
    self->header_->rings = n_rings;
    self->header_->size = real_size - 1;
    self->header_->msg_size = msg_size + sizeof(size_t);

    for (i = 0; i < self->header_->rings; i++) {
        self->ring_[i]._size = real_size - 1;
        self->ring_[i]._msg_size = self->header_->msg_size;
        self->ring_[i]._offset = &self->data_[i*real_size*msg_size] - self->data_;
    }

    return true;
}

#ifndef ANONYMOUS
force_inline  bool ctx_open(struct nmq_context *self, const char *fname, unsigned int nodes, unsigned int size, unsigned int msg_size) {

    int fd = open(self->fname_, O_RDWR);
    if (fd == -1)
      return ctx_create(self, fname, nodes, size, msg_size);
    
    struct stat buf;
    if (fstat(fd, &buf) == -1) 
      return false;
    unsigned int file_size = buf.st_size;
    
    if (ftruncate(fd, file_size) == -1)
      return false;

    self->p_ = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (self->p_ == NULL)
      return false;
    
    self->header_ = (struct nmq_header*)self->p_;
    self->ring_ = (struct nmq_ring*)(self->header_ + 1);
    self->data_ = (char*)(self->ring_ + self->header_->rings);    
    
    return true;
}
#endif

// Node pair to nmq_ring
force_inline static unsigned int ctx_np2r(struct nmq_context *self, unsigned int from, unsigned int to) {
    assert(from != to);
    assert(from < self->header_->nodes);
    assert(to < self->header_->nodes);
    if (from > to) {
        return to * (self->header_->nodes - 1) + from - 1;
    } else {
        return to * (self->header_->nodes - 1) + from;
    }
}

force_inline  void ctx_print(struct nmq_context *self) {
    printf("nodes: %u, size: %u, msgsz: %lu\n", self->header_->nodes, self->header_->size, self->header_->msg_size);
    unsigned int i;
    for (i = 0; i < self->header_->rings; i++) {
        printf("%3i: %10u %10u\n", i, self->ring_[i]._head, self->ring_[i]._tail);
    }
}
force_inline static struct nmq_ring* ctx_get_ring(struct nmq_context *self, unsigned int from, unsigned int to) {
    // TODO set errno and return error condition
    assert(self->p_ != NULL);
    return &self->ring_[ctx_np2r(self, from, to)];
}

force_inline static bool ctx_send(struct nmq_context *self, struct nmq_ring *ring, const void *msg, size_t size) {
    assert(size <= (ring->_msg_size - sizeof(size_t)));

    unsigned int h = (ring->_head - 1) & ring->_size;
    unsigned int t = ring->_tail;
    if (t == h)
        return false;

    char *d = &self->data_[self->ring_->_offset + t*ring->_msg_size];
    
    memcpy(d, &size, sizeof(size));
    memcpy(d + sizeof(size), msg, size);

    // Barrier is needed to make sure that item is updated 
    // before it's made available to the reader
    memw();

    ring->_tail = (t + 1) & ring->_size;
    return true;
}

force_inline  bool ctx_sendto(struct nmq_context *self, unsigned int from, unsigned int to, const void *msg, size_t size) {
    struct nmq_ring *ring = ctx_get_ring(self, from, to);
    while (!ctx_send(self, ring, msg, size)) {__relax();}
    return true;
}

force_inline  bool ctx_sendnb(struct nmq_context *self, unsigned int from, unsigned int to, const void *msg, size_t size) {
    struct nmq_ring *ring = ctx_get_ring(self, from, to);
    return ctx_send(self, ring, msg, size);
}

force_inline static bool ctx_recv(struct nmq_context *self, struct nmq_ring *ring, void *msg, size_t *size) {
    unsigned int t = ring->_tail;
    unsigned int h = ring->_head;
    if (h == t)
        return false;

    char *d = &self->data_[self->ring_->_offset + h*ring->_msg_size];

    size_t recv_size;
    memcpy(&recv_size, d, sizeof(size_t));
    assert(recv_size <= *size && "buffer too small");
    *size = recv_size;
    memcpy(msg, d + sizeof(size_t), recv_size);

    // Barrier is needed to make sure that we finished reading the item
    // before moving the head
    comp();

    ring->_head = (h + 1) & self->ring_->_size;
    return true;
}

force_inline  bool ctx_recvfrom(struct nmq_context *self, unsigned int from, unsigned int to, void *msg, size_t *size) {
    struct nmq_ring *ring = ctx_get_ring(self, from, to);
    while (!ctx_recv(self, ring, msg, size)) {__relax();}
    return true;
}

force_inline  bool ctx_recvnb(struct nmq_context *self, unsigned int from, unsigned int to, void *s, size_t *size) {
    return ctx_recv(self, ctx_get_ring(self, from, to), s, size);
}


force_inline static bool ctx_recv2(struct nmq_context *self, unsigned int to, void *msg, size_t *size) {
    // TODO "fair" receiving
    unsigned int i;
    while (true) {
        for (i = 0; i < self->header_->nodes; i++) {
            if (to != i && ctx_recvnb(self, i, to, msg, size)) 
                return true;
        }
        __relax();
    }
    return false;
}
force_inline static ssize_t ctx_recvnb2(struct nmq_context *self, unsigned int to, void *msg, size_t *size) {
    // TODO "fair" receiving
    unsigned int i;
    for (i = 0; i < self->header_->nodes; i++) {
        if (to != i && ctx_recvnb(self, i, to, msg, size)) 
            return true;
    }
    return false;
}
