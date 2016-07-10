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

/* Temporary stream open/close handlers.
 */
typedef struct _sp_trans_ths_t
{
    /* Open a temporary stream and write its handle under 'f'.
       If success SPEC_SUCCESS is returned */
    sp_errc_t (*open)(void *arg, SP_FILE *f);

    /* Close a temporary stream.
       NOTE: If the temporary stream is represented by a permanent,
       one-time-use file, the handler should remove it. */
    void (*close)(void *arg, SP_FILE *f);

    /* User defined argument passed untouched by the API to the handlers */
    void *arg;
} sp_trans_ths_t;

/* Transaction handle struct.
 */
typedef struct _sp_trans_t
{
    /* modified stream */
    SP_FILE *in;
    /* modified parsing scope; zeroed for the global scope */
    sp_loc_t parsc;

    /* EOL type detected on the input */
    sp_eol_t eol_typ;

    /* current index in fs[] table */
    unsigned fs_i;
    /* stream handles */
    struct {
        SP_FILE f;
        int state;
    } fs[2];

    /* number of bytes to skip from a partial
       commit while copying the final result */
    int skip_in;

    /* number of successful partial commits for the transaction */
    unsigned n_commits;

    sp_trans_ths_t ths;
} sp_trans_t;

/* Initialize handle and start a transaction.

   If 'in' is NULL it is treated as an empty stream (first read char will be
   EOF). This enables from scratch output creation.

   A parsing scope may be specified by 'p_parsc' to define a modified scope for
   the transaction (or NULL for the global scope). It must be NULL if 'in' is
   NULL (there is no sense of a parsing scope for an empty input).

   A caller may specify its own temporary stream open/close handlers by passing
   'p_ths' argument. It could be especially useful for MS Windows platform
   where C-standard implementation of the temporary files is particularly lame.
   If 'p_ths' is NULL, C-standard tmpfile(3), fclose(3) are used.
   NOTE: The API guarantees not to maintain more than 2 unclosed temporary
   streams during the transaction processing. After the final commit all opened
   handles are closed.

   The initialized handle must be committed or discarded. See sp_commit_tr(),
   sp_commit2_tr(), sp_discard_tr().
 */
sp_errc_t sp_init_tr(sp_trans_t *p_trans, SP_FILE *in,
    const sp_loc_t *p_parsc, const sp_trans_ths_t *p_ths);

/* Commit the transaction to a specified output.

   The resulting output will be written to 'out'. If 'out' is NULL the function
   simply discards the transaction.

   NOTE 1: The function closes the transaction handle unless an error occurs
   (in which case allowing to fix a problem and re-commit the transaction).
   NOTE 2: If 'out' is NULL (transaction discarding) the function always closes
   the handle.
 */
sp_errc_t sp_commit_tr(sp_trans_t *p_trans, SP_FILE *out);

/* Commit the transaction to a specified file.

   The resulting output will be written to a newly created file with 'new_file'
   name (if exists will be overwritten). If 'new_file' is NULL the function
   simply discards the transaction.

   See also sp_commit_tr() notes.
 */
sp_errc_t sp_commit2_tr(sp_trans_t *p_trans, const char *new_file);

/* Discard the transaction and close its handle.
   This is an alias to sp_commit_tr() with NULL output.
 */
#define sp_discard_tr(t) sp_commit_tr((t), NULL)

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
