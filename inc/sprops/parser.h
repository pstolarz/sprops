/*
   Copyright (c) 2015,2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SP_PARSER_H__
#define __SP_PARSER_H__

#include "sprops/props.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _sp_parser_token_t
{
    SP_TKN_ID = 258,
    SP_TKN_VAL
} sp_parser_token_t;

struct _sp_parser_hndl_t;

/* Property callback provides location of a property name ('p_lname'), value
   ('p_lval'; may be NULL for property w/o a value) and the overall definition
   of the property (p_ldef).

   Callback return codes:
       SPEC_CB_FINISH: success; finish further parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_parser_cb_prop_t)(const struct _sp_parser_hndl_t *p_hndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef);

/* Scope callback provides location of a scope type ('p_ltype'; may be NULL
   for scope w/o a type), name ('p_lname'), body ('p_lbody'; may be NULL for
   scope w/o a body), body with enclosing brackets ('lbdyenc') and the overall
   definition of the scope ('p_ldef').
 */
typedef sp_errc_t (*sp_parser_cb_scope_t)(const struct _sp_parser_hndl_t *p_hndl,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef);

typedef
struct _unc_cache_t
{
    size_t inbuf;   /* number of cached chars */
    int buf[16];    /* cache buffer */
} unc_cache_t;

typedef struct _sp_parser_hndl_t
{
    /* parsed input */
    SP_FILE *in;

    struct {
        /* next char to read */
        int line;
        int col;
        long off;       /* input offset */

        /* last input offset to parse; -1: end */
        long end;

        /* currently scope level (0-based) */
        int scope_lev;

        /* lexical context */
        int ctx;

        /* unget chars cache */
        unc_cache_t unc;
    } lex;

    struct {
        /* argument passed untouched */
        void *arg;

        /* parser callbacks */
        sp_parser_cb_prop_t prop;
        sp_parser_cb_scope_t scope;
    } cb;

    struct {
        sp_errc_t code;

        /* syntax error (SPEC_SYNTAX) detailed code and location */
        sp_errsyn_t syn;
        struct {
            int line;
            int col;
        } loc;
    } err;
} sp_parser_hndl_t;

/* Initialize parser handle under 'p_hndl' for an input 'in' to parse. Parsing
   scope is constrained to 'p_parsc' (if NULL: the entire input). Property/scope
   callbacks are provided by 'cb_prop' and 'cb_scope' respectively, with caller
   specific argument passed untouched to these functions ('cb_arg').
 */
sp_errc_t sp_parser_hndl_init(sp_parser_hndl_t *p_hndl,
    SP_FILE *in, const sp_loc_t *p_parsc, sp_parser_cb_prop_t cb_prop,
    sp_parser_cb_scope_t cb_scope, void *cb_arg);

/* Parser method */
sp_errc_t sp_parse(sp_parser_hndl_t *p_hndl);

/* Copy a token of type 'tkn' from location 'p_loc' into buffer 'p_buf' with
   length as set in 'buf_len'. If there is enough space the copied string is
   NULL terminated. If 'p_tklen' is not NULL it will be provided with token's
   content length occupied in the stream.
 */
sp_errc_t sp_parser_tkn_cpy(
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn,
    const sp_loc_t *p_loc, char *buf, size_t buf_len, long *p_tklen);

/* Compare a token of type 'tkn' from location 'p_loc' with string 'str'.
   'num' specifies maximum number of 'str' chars to compare. If stresc!=0
   then 'str' may contain backslash escaped chars.

   In case of success (SPEC_SUCCESS) the function sets 'p_equ' to the comparison
   result: 1:equal, 0:not equal.
 */
sp_errc_t sp_parser_tkn_cmp(const sp_parser_hndl_t *p_phndl,
    sp_parser_token_t tkn, const sp_loc_t *p_loc, const char *str,
    size_t num, int stresc, int *p_equ);

#define SPAR_MIN_CV_LEN     10

/* Set cut SP_TKN_VAL token length. 0: no length constraint, otherwise 'n' must
   be >= SPAR_MIN_CV_LEN and <= 255.
 */
#define SPAR_F_CVLEN(n)     ((unsigned)(n) & 0xff)

/* internal use only */
#define SPAR_F_GET_CVLEN(f) SPAR_F_CVLEN(f)

/* Set type of EOL used for SP_TKN_VAL token cut.
 */
#define SPAR_F_CVEOL(eol)   (((unsigned)(eol) & 3) << 8)

/* internal use only */
#define SPAR_F_GET_CVEOL(f) ((sp_eol_t)(((unsigned)(f)>>8) & 3))

/* Tokenize string 'str' into a token of type 'tkn' and write it to the output
   'out'. The function allows to specify SPAR_F_CV* flags controlling SP_TKN_VAL
   token cuts if its length exceeds some threshold.
 */
sp_errc_t sp_parser_tokenize_str(
    SP_FILE *out, sp_parser_token_t tkn, const char *str, unsigned cv_flags);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PARSER_H__ */
