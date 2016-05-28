/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <stdio.h>
#include "sp_props/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

sp_errc_t add_elem(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *prop_nm, const char *prop_val, const char *sc_typ,
    const char *sc_nm, int n_elem, const char *path, const char *defsc,
    unsigned flags);

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;

    FILE *in = fopen("c03.conf", "rb");
    if (!in) goto finish;

    EXEC_RG(add_elem(
        in, stdout,
        NULL,
        "prop", NULL,
        NULL, NULL,
        0,
        "/:2@$", NULL,
        0));

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);
    return 0;
}
