/*
 * Copyright 2015-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_CRYPTO_ASYNC_POSIX_H
#define OSSL_CRYPTO_ASYNC_POSIX_H
#include <openssl/e_os2.h>

#if defined(OPENSSL_SYS_UNIX) \
    && defined(OPENSSL_THREADS) && !defined(OPENSSL_NO_ASYNC) \
    && !defined(__ANDROID__) && !defined(__OpenBSD__)

# include <unistd.h>

# if _POSIX_VERSION >= 200112L \
     && (_POSIX_VERSION < 200809L || defined(__GLIBC__))

# include <pthread.h>

#  define ASYNC_POSIX
#  define ASYNC_ARCH

#  ifdef __CET__
/*
 * When Intel CET is enabled, makecontext will create a different
 * shadow stack for each context.  async_fibre_swapcontext cannot
 * use _longjmp.  It must call swapcontext to swap shadow stack as
 * well as normal stack.
 */
#   define USE_SWAPCONTEXT
#  endif
#  include <ucontext.h>
#  ifndef USE_SWAPCONTEXT
#   include <setjmp.h>
#  endif

typedef struct async_fibre_st {
    ucontext_t fibre;
#  ifndef USE_SWAPCONTEXT
    jmp_buf env;
    int env_init;
#  endif
} async_fibre;

static ossl_inline int async_fibre_swapcontext(async_fibre *o, async_fibre *n, int r)
{
#  ifdef USE_SWAPCONTEXT
    swapcontext(&o->fibre, &n->fibre);
#  else
    o->env_init = 1;

    if (!r || !_setjmp(o->env)) {
        if (n->env_init)
            _longjmp(n->env, 1);
        else
            setcontext(&n->fibre);
    }
#  endif

    return 1;
}

#  define async_fibre_init_dispatcher(d)

int async_fibre_makecontext(async_fibre *fibre);
void async_fibre_free(async_fibre *fibre);

# endif
#endif
#endif /* OSSL_CRYPTO_ASYNC_POSIX_H */
