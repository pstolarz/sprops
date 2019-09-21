/*
   Copyright (c) 2016,2019 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Transactional update example.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sprops/props.h"
#include "sprops/trans.h"

int main(void)
{
    SP_FILE in, out;
    sp_trans_t trans;

    /* update flags */
    unsigned long indf =
        SP_F_SPIND(4) | /* 4 spaces as an indent (instead of default tab) */
        SP_F_NOSEMC;    /* avoid semicolons */

    /* update will be done later via a separate stream, therefore the updated
       config file is opened in the reading mode only.
     */
    if (sp_fopen(&in, "store.conf", SP_MODE_READ) != SPEC_SUCCESS) {
        printf("Can't open the confing: %s\n", strerror(errno));
        return 1;
    }

    /* start the transaction */
    if (sp_init_tr(&trans, &in, NULL, NULL) != SPEC_SUCCESS) {
        printf("Can't initialize update-transaction\n");
        sp_close(&in);
        return 1;
    }

    /* add a new book to inventory
     */

    /* new scope (representing a book) */
    assert(sp_add_scope_tr(&trans,
        NULL,               /* untyped scope */
        "3",                /* scope name: book id */
        SP_ELM_LAST,        /* add as the last element */
        "items/books",      /* books inventory path */
        NULL, indf) == SPEC_SUCCESS);

    assert(sp_add_prop_tr(&trans,
        "title", "General Topology",
        SP_ELM_LAST,
        "items/books/3",    /* already created scope */
        NULL, indf) == SPEC_SUCCESS);

    assert(sp_add_prop_tr(&trans,
        "author", "Nicolas Bourbaki",
        SP_ELM_LAST,
        "items/books/3",
        NULL, indf) == SPEC_SUCCESS);

    assert(sp_add_prop_tr(&trans,
        "year", "1995",
        SP_ELM_LAST,
        "items/books/3",
        NULL, indf) == SPEC_SUCCESS);

    /* add a new store item for already created book
     */

    assert(sp_add_scope_tr(&trans,
        "book", "3",        /* scope type and name */
        SP_ELM_LAST,
        "store",
        NULL, indf) == SPEC_SUCCESS);

    assert(sp_add_prop_tr(&trans,
        "price", "150",
        SP_ELM_LAST,
        "store/book:3",     /* already created item */
        NULL, indf) == SPEC_SUCCESS);

    assert(sp_add_prop_tr(&trans,
        "stock", "5",
        SP_ELM_LAST,
        "store/book:3",
        NULL, indf) == SPEC_SUCCESS);

    sp_close(&in);

    /* For the purpose of this example the updated config will be printed on
       stdio. In usual usage the original input file shall be re-opened in
       SP_MODE_WRITE_NEW mode to discard the original content and replace it
       with the new one.
     */
    sp_fopen2(&out, stdout);

    /* commit and finish the transaction */
    sp_commit_tr(&trans, &out);

    return 0;
}
