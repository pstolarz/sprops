/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../config.h"
#include "sprops/parser.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    
    char fbuf[64];
    FILE *f=NULL;

#define __TEST(tkn, in, exp) \
    fflush(f); memset(fbuf, 0, sizeof(fbuf)); \
    EXEC_RG(sp_parser_tokenize_str(f, (tkn), (in))); \
    printf("IN<%s> OUT<%s>\n", (in), fbuf); \
    assert(!strcmp(fbuf, (exp)));

    if (!(f=fopen(
#if defined(_WIN32) || defined(_WIN64)
        "nul"
#else
        "/dev/null"
#endif
        , "wb")))
    {
        printf("Can't open NULL output\n");
        goto finish;
    }
    setbuf(f, fbuf);

    printf("\n--- SP_TKN_ID tokenizing\n");

    /* simplest case; no escaping required */
    __TEST(SP_TKN_ID, "abc", "abc");

    /* various printable, non-space, non-reserved chars;
       no escaping/quotation required */
    __TEST(SP_TKN_ID, "+-*/%()[]|&:@/,.!?\"'$^", "+-*/%()[]|&:@/,.!?\"'$^");

    /* similar to previous, but first quotation char need to be escaped */
    __TEST(SP_TKN_ID, "\"abc\"", "\\\"abc\"");
    __TEST(SP_TKN_ID, "'abc'", "\\'abc'");

    /* space chars + backslash; escaping inside quotation */
    __TEST(SP_TKN_ID, "\n\t\\", "\"\\n\\t\\\\\"");

    /* space; quotation inside ' since " is present */
    __TEST(SP_TKN_ID, "a b\"c\"", "'a b\"c\"'");

    /* reserved chars; quotation inside " */
    __TEST(SP_TKN_ID, "{=}#", "\"{=}#\"");


    printf("\n--- SP_TKN_VAL tokenizing\n");

    /* no escaping required */
    __TEST(SP_TKN_VAL, "\"abc'd !?/*", "\"abc'd !?/*");

    /* non printable chars + backslash; escaping */
    __TEST(SP_TKN_VAL, "\n\t\a\\", "\\n\\t\\a\\\\");

    /* semicolon escaping */
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
    __TEST(SP_TKN_VAL, "abc;", "abc\\;");
#else
    __TEST(SP_TKN_VAL, "abc;", "abc;");
#endif

    /* val with leading spaces */
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
    __TEST(SP_TKN_VAL, "  abc", "\\  abc");
#else
    __TEST(SP_TKN_VAL, "  abc", "  abc");
#endif

    /* val with trailing spaces (last escaped by \x) */
    __TEST(SP_TKN_VAL, "abc  ", "abc \\x20");

finish:
    if (ret) printf("Error: %d\n", ret);
    if (f) fclose(f);
    return 0;

#undef __TEST
}
