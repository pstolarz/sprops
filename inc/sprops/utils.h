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
sp_errc_t
    sp_util_cpy_to_out(SP_FILE *in, SP_FILE *out, long beg, long end, long *p_n);

#define EOL_NDETECT EOL_PLAT

/* Detects type of EOL used on the input. If no EOL marker is present, the
   function writes EOL_NDETECT under 'p_eol_typ' (which is an alias to EOL_PLAT).
 */
sp_errc_t sp_util_detect_eol(SP_FILE *in, sp_eol_t *p_eol_typ);

/* Case insensitive strings comparison.
 */
int sp_util_stricmp(const char *str1, const char *str2);

/* Trim trailing spaces of a string pointed by *p_str (NULL terminator is
   written). If 'trim_lead' is !=0 then *p_str pointer is updated by setting
   it on the first non-white space char (the result is written under 'p_str').

   The function returns length of the trimmed string (starting from possibly
   updated *p_str).
 */
size_t sp_util_strtrim(char **p_str, int trim_lead);

/* Parse string 'str' as an integer and return the value under 'p_val'. In case
   of string format problem SPEC_VAL_ERR error is returned.
 */
sp_errc_t sp_util_parse_int(const char *str, long *p_val);

/* Parse string 'str' as a float number and return the value under 'p_val'. In
   case of string format problem SPEC_VAL_ERR error is returned.
 */
sp_errc_t sp_util_parse_float(const char *str, double *p_val);

/* Parse string 'str' as an enumeration and return the value under 'p_val'.
   Matching enumeration names to their values is done via 'p_evals' table with
   the last element filled with zeros. The matching is case insensitive if
   'igncase' is !=0.

   If the parsed string doesn't match any of the names in 'p_evals' SPEC_VAL_ERR
   error is returned.
 */
sp_errc_t sp_util_parse_enum(
    const char *str, const sp_enumval_t *p_evals, int igncase, int *p_val);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_UTILS_H__ */
