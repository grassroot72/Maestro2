/* Red Black balanced tree
 *
 * > Created (Julienne Walker): August 23, 2003
 * > Modified (Julienne Walker): March 14, 2008

 * license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _RBTREE_H_
#define _RBTREE_H_


#define RB_MAX_HEIGHT 64 /* Tallest allowable tree */

/* User-defined item handling */
typedef int (*rb_cmp_fn) (const void *p1, const void *p2);
typedef void (*rb_del_fn) (void *p);
typedef void (*rb_prt_fn)(const void *p);

typedef struct _rbnode {
  int red;                 /* Color (1=red, 0=black) */
  void *data;              /* User-defined content */
  struct _rbnode *link[2]; /* Left (0) and right (1) links */
} rbnode_t;

typedef struct {
  rbnode_t *root; /* Top of the tree */
  rb_cmp_fn cmp;  /* Compare two items */
  rb_del_fn del;  /* Destroy an item (user-defined) */
  rb_prt_fn prt;  /* Print an item (user-defined) */
  size_t size;    /* Number of items (user-defined) */

  pthread_mutex_t mutex;
} rbtree_t;

typedef struct {
  rbtree_t *tree;                /* Paired tree */
  rbnode_t *it;                  /* Current node */
  rbnode_t *path[RB_MAX_HEIGHT]; /* Traversal path */
  size_t top;                    /* Top of stack */
} rbtrav_t;


/* Red Black tree functions */
rbtree_t *rbtree_new(rb_cmp_fn cmp,
                     rb_del_fn del,
                     rb_prt_fn prt);

void rbtree_delete(rbtree_t *tree);

void rbtree_set_cmp(rbtree_t *tree,
                    rb_cmp_fn cmp);

void rbtree_set_del(rbtree_t *tree,
                    rb_del_fn del);

void *rbtree_search(rbtree_t *tree,
                    void *data);

int rbtree_insert(rbtree_t *tree,
                  void *data);

int rbtree_remove(rbtree_t *tree,
                  void *data);

void rbtree_print(rbtree_t *tree);


/* Traversal functions */
rbtrav_t *rbtrav_new(void);

void rbtrav_delete(rbtrav_t *trav);

void *rbtrav_first(rbtrav_t *trav,
                   rbtree_t *tree);

void *rbtrav_last(rbtrav_t *trav,
                  rbtree_t *tree);

void *rbtrav_next(rbtrav_t *trav);

void *rbtrav_prev(rbtrav_t *trav);


#endif
