/* Red Black balanced tree
 *
 * > Created (Julienne Walker): August 23, 2003
 * > Modified (Julienne Walker): March 14, 2008

 * license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "xmalloc.h"
#include "rbtree.h"

#define DEBUG
#include "debug.h"


/* Checks the color of a red black node */
static int _is_red(rbnode_t *root)
{
  return root != NULL && root->red == 1;
}

/* Performs a single red black rotation in the specified direction
 * This function assumes that all nodes are valid for a rotation
 * dir: 0 => left, 1 => right
 * return: the new root ater rotation */
static rbnode_t *_single_rotation(rbnode_t *root,
                                  int dir)
{
  rbnode_t *save = root->link[!dir];
  root->link[!dir] = save->link[dir];
  save->link[dir] = root;

  root->red = 1;
  save->red = 0;
  return save;
}

/* Performs a double red black rotation in the specified direction
 * This function assumes that all nodes are valid for a rotation
 * dir: 0 => left, 1 => right
 * return: the new root ater rotation */
static rbnode_t *_double_rotation(rbnode_t *root,
                                  int dir)
{
  root->link[!dir] = _single_rotation(root->link[!dir], !dir);
  return _single_rotation(root, dir);
}

/* Creates an initializes a new red black node with a copy of
 * the data. This function does not insert the new node into a tree
 * return: a pointer to the new node */
static rbnode_t *_new_node(rbtree_t *tree,
                           void *data)
{
  rbnode_t *n = xmalloc(sizeof(rbnode_t));
  if (n == NULL) return NULL;

  n->red = 1;
  n->data = data;
  n->link[0] = n->link[1] = NULL;
  return n;
}

/* Creates and initializes an empty red black tree with
 * user-defined comparison, data copy, and data release operations
 * return: a pointer to the new tree */
rbtree_t *rbtree_new(rb_cmp_fn cmp,
                     rb_del_fn del,
                     rb_prt_fn prt)
{
  rbtree_t *t = xmalloc(sizeof(rbtree_t));
  if (t == NULL) return NULL;

  t->root = NULL;
  t->cmp = cmp;
  t->del = del;
  t->prt = prt;
  t->size = 0;
  pthread_mutex_init(&t->mutex, NULL);
  return t;
}

/* Releases a valid red black tree */
void rbtree_delete(rbtree_t *tree)
{
  rbnode_t *it = tree->root;
  rbnode_t *save;
  /* Rotate away the left links so that we can treat this like
   * the destruction of a linked list */
  while (it != NULL) {
    if (it->link[0] == NULL) {
      /* No left links, just kill the node and move on */
      save = it->link[1];
      tree->del(it->data);
      xfree(it);
    }
    else {
      /* Rotate away the left link and check again */
      save = it->link[0];
      it->link[0] = save->link[1];
      save->link[1] = it;
    }
    it = save;
  }
  xfree(tree);
}

void rbtree_set_cmp(rbtree_t *tree,
                    rb_cmp_fn cmp)
{
  tree->cmp = cmp;
}

void rbtree_set_del(rbtree_t *tree,
                    rb_del_fn del)
{
  tree->del = del;
}

/* Search for a copy of the specified node data in a red black tree
 * return: A pointer to the data value stored in the tree,
           or a null pointer if no data could be found */
void *rbtree_search(rbtree_t *tree,
                    void *data)
{
  rbnode_t *it = tree->root;
  while (it != NULL) {
    int cmp = tree->cmp(it->data, data);
    if (cmp == 0) break;
    /* If the tree supports duplicates, they should be
     * chained to the right subtree for this to work */
    it = it->link[cmp < 0];
  }
  return it == NULL ? NULL : it->data;
}

/* Insert a copy of the user-specified data into a red black tree
 * return: 1 if the value was inserted successfully,
           0 if the insertion failed for any reason */
int rbtree_insert(rbtree_t *tree,
                  void *data)
{
  if (tree->root == NULL) {
    /* We have an empty tree; attach the new node directly to the root */
    tree->root = _new_node(tree, data);
    if (tree->root == NULL) return 0;
  }
  else {
    rbnode_t head = {0}; /* False tree root */
    rbnode_t *g, *t;     /* Grandparent & parent */
    rbnode_t *p, *q;     /* Iterator & parent */
    int dir = 0, last = 0;
    /* Set up our helpers */
    t = &head;
    g = p = NULL;
    q = t->link[1] = tree->root;
    /* Search down the tree for a place to insert */
    for (;;) {
      if (q == NULL) {
        /* Insert a new node at the first null link */
        p->link[dir] = q = _new_node(tree, data);
        if (q == NULL) return 0;
      }
      else if (_is_red(q->link[0]) && _is_red(q->link[1])) {
        /* Simple red violation: color flip */
        q->red = 1;
        q->link[0]->red = 0;
        q->link[1]->red = 0;
      }

      if (_is_red(q) && _is_red(p)) {
        /* Hard red violation: rotations necessary */
        int dir2 = t->link[1] == g;

        if (q == p->link[last])
          t->link[dir2] = _single_rotation(g, !last);
        else
          t->link[dir2] = _double_rotation(g, !last);
      }
      /* Stop working if we inserted a node.
       * This check also disallows duplicates in the tree */
      if (tree->cmp(q->data, data) == 0) break;

      last = dir;
      dir = (tree->cmp(q->data, data) < 0);

      /* Move the helpers down */
      if (g != NULL) t = g;

      g = p, p = q;
      q = q->link[dir];
    }
    /* Update the root (it may be different) */
    tree->root = head.link[1];
  }
  /* Make the root black for simplified logic */
  tree->root->red = 0;
  ++tree->size;
  return 1;
}

/* Remove a node from a red black tree that matches the user-specified data
 * return: 1 if the value was removed successfully,
           0 if the removal failed for any reason */
int rbtree_remove(rbtree_t *tree,
                  void *data)
{
  int rc = 0;
  if (tree->root != NULL) {
    rbnode_t head = {0}; /* False tree root */
    rbnode_t *q, *p, *g; /* Helpers */
    rbnode_t *f = NULL;  /* Found item */
    int dir = 1;
    /* Set up our helpers */
    q = &head;
    g = p = NULL;
    q->link[1] = tree->root;
    /* Search and push a red node down to fix red violations as we go */
    while (q->link[dir] != NULL) {
      int last = dir;
      /* Move the helpers down */
      g = p, p = q;
      q = q->link[dir];
      dir = (tree->cmp(q->data, data) < 0);
      /* Save the node with matching data and keep going;
       * we'll do removal tasks at the end */
      if (tree->cmp(q->data, data) == 0) f = q;
      /* Push the red node down with rotations and color flips */
      if (!_is_red(q) && !_is_red(q->link[dir])) {
        if (_is_red(q->link[!dir]))
          p = p->link[last] = _single_rotation(q, dir);
        else if (!_is_red(q->link[!dir])) {
          rbnode_t *s = p->link[!last];
          if (s != NULL) {
            if (!_is_red(s->link[!last]) && !_is_red(s->link[last])) {
              /* Color flip */
              p->red = 0;
              s->red = 1;
              q->red = 1;
            }
            else {
              int dir2 = (g->link[1] == p);
              if (_is_red(s->link[last]))
                g->link[dir2] = _double_rotation(p, last);
              else if (_is_red(s->link[!last]))
                g->link[dir2] = _single_rotation(p, last);
              /* Ensure correct coloring */
              q->red = g->link[dir2]->red = 1;
              g->link[dir2]->link[0]->red = 0;
              g->link[dir2]->link[1]->red = 0;
            }
          }
        }
      }
    }
    /* Replace and remove the saved node */
    if (f != NULL) {
      tree->del(f->data);
      f->data = q->data;
      p->link[(p->link[1] == q)] = q->link[(q->link[0] == NULL)];
      xfree(q);
      rc = 1;
    }
    /* Update the root (it may be different) */
    tree->root = head.link[1];
    /* Make the root black for simplified logic */
    if (tree->root != NULL) tree->root->red = 0;
    --tree->size;
  }
  return rc;
}

void rbtree_print(rbtree_t *tree)
{
  if (tree->size == 0) return;
  int i = 0;
  rbtrav_t *trav = rbtrav_new();
  void *data = rbtrav_first(trav, tree);
  do {
    tree->prt(data);
    data = rbtrav_next(trav);
    i++;
  } while (i < tree->size);
  xfree(trav);
}

/* Create a new traversal object */
rbtrav_t *rbtrav_new(void)
{
  return (rbtrav_t *)xmalloc(sizeof(rbtrav_t));
}

/* Release a traversal object */
void rbtrav_delete(rbtrav_t *trav)
{
  xfree(trav);
}

/* Initialize a traversal object. The user-specified
 * direction determines whether to begin traversal at the
 * smallest or largest valued node
 * return: a pointer to the smallest or largest data value */
static void *_start(rbtrav_t *trav,
                    rbtree_t *tree,
                    int dir)
{
  trav->tree = tree;
  trav->it = tree->root;
  trav->top = 0;
  /* Save the path for later traversal */
  if (trav->it != NULL) {
    while (trav->it->link[dir] != NULL) {
      trav->path[trav->top++] = trav->it;
      trav->it = trav->it->link[dir];
    }
  }
  return trav->it == NULL ? NULL : trav->it->data;
}

/* Traverse a red black tree in the user-specified direction
 * The direction to traverse (0 = ascending, 1 = descending)
 * return: a pointer to the next data value in the specified direction */
static void *_move(rbtrav_t *trav,
                   int dir)
{
  if (trav->it->link[dir] != NULL) {
    /* Continue down this branch */
    trav->path[trav->top++] = trav->it;
    trav->it = trav->it->link[dir];

    while (trav->it->link[!dir] != NULL) {
      trav->path[trav->top++] = trav->it;
      trav->it = trav->it->link[!dir];
    }
  }
  else {
    /* Move to the next branch */
    rbnode_t *last;
    do {
      if (trav->top == 0) {
        trav->it = NULL;
        break;
      }
      last = trav->it;
      trav->it = trav->path[--trav->top];
    } while (last == trav->it->link[dir]);
  }

  return trav->it == NULL ? NULL : trav->it->data;
}

/* Initialize a traversal object to the smallest valued node
 * return: a pointer to the smallest data value */
void *rbtrav_first(rbtrav_t *trav,
                   rbtree_t *tree)
{
  return _start(trav, tree, 0); /* Min value */
}

/* Initialize a traversal object to the largest valued node
 * return: a pointer to the largest data value */
void *rbtrav_last(rbtrav_t *trav,
                  rbtree_t *tree)
{
  return _start(trav, tree, 1); /* Max value */
}

/* Traverse to the next value in ascending order
 * return: a pointer to the next value in ascending order */
void *rbtrav_next(rbtrav_t *trav)
{
  return _move(trav, 1); /* Toward larger items */
}

/* Traverse to the next value in descending order
 * returns>A pointer to the next value in descending order */
void *rbtrav_prev(rbtrav_t *trav)
{
  return _move(trav, 0); /* Toward smaller items */
}
