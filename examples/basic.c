/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Basic example presenting properties retrieval via get methods and iteration.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sprops/props.h"

static long def_volt;
static long def_freq;

/* Iterate callback to read voltage spec. of countries defined in their scopes.
 */
static sp_errc_t country_iter_cb(
    void *arg, SP_FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
    const char *name, const sp_tkn_info_t *p_tkname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    long volt=0, freq=0;

    if (p_lbody) {
        sp_get_prop_int(in, p_lbody, "voltage", 0, NULL, NULL, &volt, NULL);
        sp_get_prop_int(in, p_lbody, "freq", 0, NULL, NULL, &freq, NULL);
    }

    printf("%s:\n", name);
    printf("  Voltage: %d, Frequency: %d\n",
        (int)(volt>0 ? volt : def_volt), (int)(freq>0 ? freq : def_freq));

    /* iterate for the next scope */
    return SPEC_SUCCESS;
}

int main(void)
{
    SP_FILE in;
    long volt=0;

    /* temp buf for scopes iteration callback */
    char nm_buf[32];

    /* an input file must be opened in the binary mode */
    if (sp_fopen(&in, "basic.conf", "rb") != SPEC_SUCCESS) {
        printf("Can't open the confing: %s\n", strerror(errno));
        return 1;
    }

    /* get default values defined in the global scope */
    sp_get_prop_int(&in, NULL, "voltage", 0, NULL, NULL, &def_volt, NULL);
    sp_get_prop_int(&in, NULL, "freq", 0, NULL, NULL, &def_freq, NULL);

    printf("Default values are:\n  Voltage: %d, Frequency: %d\n\n",
        (int)def_volt, (int)def_freq);

    /* iterate over countries (scopes in the config) */
    sp_iterate(&in, NULL, NULL, NULL, NULL, country_iter_cb, NULL,
        NULL, 0, nm_buf, sizeof(nm_buf));

    /* there is possible to get property located in a specific place */
    sp_get_prop_int(&in, NULL, "voltage", 0, "USA", NULL, &volt, NULL);
    printf("\nVoltage in USA: %d\n", (int)volt);

    sp_fclose(&in);
    return 0;
}
