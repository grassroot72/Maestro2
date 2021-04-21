/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _XMALLOC_H_
#define _XMALLOC_H_


#define _USE_MIMALLOC_

#ifdef _USE_MIMALLOC_
#include <mimalloc.h>

#define xmalloc mi_malloc
#define xcalloc mi_calloc
#define xrealloc mi_realloc
#define xstrdup mi_strdup
#define xfree mi_free

#else

#include <stdlib.h>
#define xmalloc malloc
#define xcalloc calloc
#define xrealloc realloc
#define xstrdup strdup
#define xfree free

#endif


#endif
