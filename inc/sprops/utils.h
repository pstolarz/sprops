/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Functions from this header, although not part of the API main set,
   may be useful on various occasions while working with the library.
 */

#ifndef __SP_UTILS_H__
#define __SP_UTILS_H__

#include "sprops/props.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies 'in' stream bytes to 'out' stream starting from 'beg' up to 'end'
   (exclusive). If 'end' is EOF: copies up to the end of input. If 'p_n' is not
   NULL it will get number of copied bytes.

   NOTE: Usage of this function may be helpful when playing with a parsing scope
   ('p_parsc' argument) in write access API, to restore modified input after
   the scope modification is finished. See the transactional API implementation
   for an example of usage.
 */
sp_errc_t sp_util_cpy_to_out(FILE *in, FILE *out, long beg, long end, long *p_n);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_UTILS_H__ */
