/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "networkio.h"
#include "apr_strings.h"

static apr_status_t soblock(int sd)
{
/* BeOS uses setsockopt at present for non blocking... */
#ifndef BEOS
    int fd_flags;

    fd_flags = fcntl(sd, F_GETFL, 0);
#if defined(O_NONBLOCK)
    fd_flags &= ~O_NONBLOCK;
#elif defined(O_NDELAY)
    fd_flags &= ~O_NDELAY;
#elif defined(FNDELAY)
    fd_flags &= ~O_FNDELAY;
#else
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(sd, F_SETFL, fd_flags) == -1) {
        return errno;
    }
#else
    int on = 0;
    if (setsockopt(sd, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int)) < 0)
        return errno;
#endif /* BEOS */
    return APR_SUCCESS;
}

static apr_status_t sononblock(int sd)
{
#ifndef BEOS
    int fd_flags;

    fd_flags = fcntl(sd, F_GETFL, 0);
#if defined(O_NONBLOCK)
    fd_flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
    fd_flags |= O_NDELAY;
#elif defined(FNDELAY)
    fd_flags |= O_FNDELAY;
#else
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(sd, F_SETFL, fd_flags) == -1) {
        return errno;
    }
#else
    int on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int)) < 0)
        return errno;
#endif /* BEOS */
    return APR_SUCCESS;
}

apr_status_t apr_setsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t on)
{
    int one;
    apr_status_t stat;

    if (on)
        one = 1;
    else
        one = 0;
    if (opt & APR_SO_KEEPALIVE) {
#ifdef SO_KEEPALIVE
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_SNDBUF) {
#ifdef SO_SNDBUF
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
            return errno;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (opt & APR_SO_NONBLOCK) {
        if (on) {
            if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS) 
                return stat;
        }
        else {
            if ((stat = soblock(sock->socketdes)) != APR_SUCCESS)
                return stat;
        }
    }
    if (opt & APR_SO_LINGER) {
#ifdef SO_LINGER
        struct linger li;
        li.l_onoff = on;
        li.l_linger = MAX_SECS_TO_LINGER;
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return errno;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (opt & APR_SO_TIMEOUT) { 
        /* don't do the fcntl foo more than needed */ 
        if (on >= 0 && sock->timeout < 0 
                && (stat = sononblock(sock->socketdes)) != APR_SUCCESS) { 
            return stat; 
        } 
        else if (on < 0 && sock->timeout >= 0 
                && (stat = soblock(sock->socketdes)) != APR_SUCCESS) { 
            return stat; 
        } 
        sock->timeout = on; 
    } 
    if (opt & APR_TCP_NODELAY) {
#if defined(TCP_NODELAY)
        if (setsockopt(sock->socketdes, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(int)) == -1) {
            return errno;
        }
#else
        /* BeOS pre-BONE has TCP_NODELAY set by default.
         * As it can't be turned off we might as well check if they're asking
         * for it to be turned on!
         */
#ifdef BEOS
        if (on == 1)
            return APR_SUCCESS;
        else
#endif
        return APR_ENOTIMPL;
#endif
    }
    return APR_SUCCESS; 
}         

apr_status_t apr_getsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t *on)
{
    switch(opt) {
    case APR_SO_TIMEOUT:
        *on = sock->timeout;
        break;
    default:
        return APR_EINVAL;
    }
    return APR_SUCCESS;
}

apr_status_t apr_gethostname(char *buf, apr_int32_t len, apr_pool_t *cont)
{
    if (gethostname(buf, len) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

