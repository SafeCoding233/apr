/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define APR_WANT_STRFUNC
#define APR_WANT_MEMFUNC
#include "apr_want.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_tables.h"

#include "apr_private.h"

#if defined(WIN32) || defined(OS2) || defined(NETWARE)
#define PATH_SEPARATOR ';'
#define PATH_SEPARATOR_STRING ";"
#else
#define PATH_SEPARATOR ':'
#define PATH_SEPARATOR_STRING ":"
#endif

APR_DECLARE(apr_status_t) apr_filepath_list_split(apr_array_header_t **pathelts,
                                                  const char *liststr,
                                                  apr_pool_t *p)
{
    char *path, *part, *ptr;
    apr_array_header_t *elts;
    int nelts;

    /* Count the number of path elements. We know there'll be at least
       one even if path is an empty string. */
    path = apr_pstrdup(p, liststr);
    for (nelts = 0, ptr = path; ptr != NULL; ++nelts)
    {
        ptr = strchr(ptr, PATH_SEPARATOR);
        if (ptr)
            ++ptr;
    }

    /* Split the path into the array. */
    elts = apr_array_make(p, nelts, sizeof(char*));
    while ((part = apr_strtok(path, PATH_SEPARATOR_STRING, &ptr)) != NULL)
    {
        if (*part == '\0')      /* Ignore empty path components. */
            continue;

        *(char**)apr_array_push(elts) = part;
        path = NULL;            /* For the next call to apr_strtok */
    }

    *pathelts = elts;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_filepath_list_merge(char **liststr,
                                                  apr_array_header_t *pathelts,
                                                  apr_pool_t *p)
{
    apr_size_t path_size = 0;
    char *path;
    int i;

    /* This test isn't 100% certain, but it'll catch at least some
       invalid uses... */
    if (pathelts->elt_size != sizeof(char*))
        return APR_EINVAL;

    /* Calculate the size of the merged path */
    for (i = 0; i < pathelts->nelts; ++i)
        path_size += strlen(((char**)pathelts->elts)[i]);

    if (path_size == 0)
    {
        *liststr = NULL;
        return APR_SUCCESS;
    }

    if (i > 0)                  /* Add space for the separators */
        path_size += (i - 1);

    /* Merge the path components */
    path = *liststr = apr_palloc(p, path_size + 1);
    if (path == NULL) {
        return APR_ENOMEM;
    }
    for (i = 0; i < pathelts->nelts; ++i)
    {
        /* ### Hmmmm. Calling strlen twice on the same string. Yuck.
               But is is better than reallocation in apr_pstrcat? */
        const char *part = ((char**)pathelts->elts)[i];
        apr_size_t part_size = strlen(part);
        if (part_size == 0)     /* Ignore empty path components. */
            continue;

        if (i > 0)
            *path++ = PATH_SEPARATOR;
        memcpy(path, part, part_size);
        path += part_size;
    }
    *path = '\0';
    return APR_SUCCESS;
}
