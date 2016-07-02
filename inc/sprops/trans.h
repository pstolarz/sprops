/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SP_TRANS_H__
#define __SP_TRANS_H__

#include "sprops/props.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Transaction handle struct.
 */
typedef struct _sp_trans_t
{
    /* modified stream */
    FILE *in;
    /* modified parsing scope; zeroed for global scope */
    sp_loc_t parsc;

    /* current index in 'tfs' */
    int ind;
    /* temporary FILE* handles */
    FILE *tfs[2];
} sp_trans_t;

/* Initialize handle and start a transaction.

   If 'in' is NULL it is treated as an empty stream (first read char will be
   EOF). This enables from scratch output creation.
   A parsing scope may be specified by 'p_parsc' to define a modified scope for
   the transaction (or NULL for the global scope). It must be NULL if 'in' is
   NULL (no sense of a parsing scope for an empty input).
 */
sp_errc_t sp_init_tr(sp_trans_t *p_trans, FILE *in, const sp_loc_t *p_parsc);

/* Commit the transaction to a specified output and close its handle.

   The resulting output will be written to 'out'. If 'out' is NULL the function
   simply discards the transaction and closes the 'p_trans' handle.
 */
sp_errc_t sp_commit_tr(sp_trans_t *p_trans, FILE *out);

/* Commit the transaction to a specified output and close its handle.

   The resulting output will be written to a newly created file with 'new_file'
   name (if exists will be overwritten). If 'new_file' is NULL the function
   simply discards the transaction and closes the 'p_trans' handle.
 */
sp_errc_t sp_commit2_tr(sp_trans_t *p_trans, const char *new_file);

/* Transactional wrapper around sp_add_prop().
 */
sp_errc_t sp_add_prop_tr(sp_trans_t *p_trans, const char *name, const char *val,
    int n_elem, const char *path, const char *deftp, unsigned long flags);

/* Transactional wrapper around sp_add_scope().
 */
sp_errc_t sp_add_scope_tr(sp_trans_t *p_trans, const char *type, const char *name,
    int n_elem, const char *path, const char *deftp, unsigned long flags);

/* Transactional wrapper around sp_rm_prop().
 */
sp_errc_t sp_rm_prop_tr(sp_trans_t *p_trans, const char *name, int ind,
    const char *path, const char *deftp, unsigned long flags);

/* Transactional wrapper around sp_rm_scope().
 */
sp_errc_t sp_rm_scope_tr(sp_trans_t *p_trans, const char *type, const char *name,
    int ind, const char *path, const char *deftp, unsigned long flags);

/* Transactional wrapper around sp_set_prop().
 */
sp_errc_t sp_set_prop_tr(sp_trans_t *p_trans, const char *name, const char *val,
    int ind, const char *path, const char *deftp, unsigned long flags);

/* Transactional wrapper around sp_mv_prop().
 */
sp_errc_t sp_mv_prop_tr(sp_trans_t *p_trans, const char *name,
    const char *new_name, int ind, const char *path, const char *deftp,
    unsigned long flags);

/* Transactional wrapper around sp_mv_scope().
 */
sp_errc_t sp_mv_scope_tr(sp_trans_t *p_trans, const char *type,
    const char *name, const char *new_type, const char *new_name, int ind,
    const char *path, const char *deftp, unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_TRANS_H__ */
