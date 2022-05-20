/*
   Copyright (c) 2016,2022 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include <string.h>
#include "../config.h"
#include "sprops/parser.h"

#if (SPAR_MIN_CV_LEN != 10)
# error SPAR_MIN_CV_LEN must be 10 for this test
#endif

#define WIN_EOL  "\x0d" "\x0a"
#define UNX_EOL  "\x0a"

#if defined(_WIN32) || defined(_WIN64)
# define PLT_EOL WIN_EOL
#else
# define PLT_EOL UNX_EOL
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    
    char buf[64];
    SP_FILE out;

#define __TEST(tkn, in, exp, flgs) \
    memset(buf, 0, sizeof(buf)); \
    sp_mopen(&out, buf, sizeof(buf)); \
    EXEC_RG(sp_parser_tokenize_str(&out, (tkn), (in), (flgs))); \
    printf("IN<%s> OUT<%s>\n", (in), buf); \
    assert(!strcmp(buf, (exp)));

    printf("\n--- SP_TKN_ID tokenizing\n");

    /* simplest case; no escaping required */
    __TEST(SP_TKN_ID, "abc", "abc", 0);

    /* various printable, non-space, non-reserved chars;
       no escaping/quotation required */
    __TEST(SP_TKN_ID, "+-*/%()[]|&:@/,.!?\"'$^", "+-*/%()[]|&:@/,.!?\"'$^", 0);

    /* similar to previous, but first quotation char need to be escaped */
    __TEST(SP_TKN_ID, "\"abc\"", "\\\"abc\"", 0);
    __TEST(SP_TKN_ID, "'abc'", "\\'abc'", 0);

    /* space chars + backslash; escaping inside quotation */
    __TEST(SP_TKN_ID, "\n\t\\", "\"\\n\\t\\\\\"", 0);

    /* space; quotation inside ' since " is present */
    __TEST(SP_TKN_ID, "a b\"c\"", "'a b\"c\"'", 0);

    /* reserved chars; quotation inside " */
    __TEST(SP_TKN_ID, "{=}#", "\"{=}#\"", 0);


    printf("\n--- SP_TKN_VAL tokenizing\n");

    /* no escaping required */
    __TEST(SP_TKN_VAL, "\"abc'd !?/*", "\"abc'd !?/*", 0);

    /* non printable chars + backslash; escaping */
    __TEST(SP_TKN_VAL, "\n\t\a\\", "\\n\\t\\a\\\\", 0);

    /* semicolon escaping */
#if !CONFIG_NO_SEMICOL_ENDS_VAL
    __TEST(SP_TKN_VAL, "abc;", "abc\\;", 0);
#else
    __TEST(SP_TKN_VAL, "abc;", "abc;", 0);
#endif

    /* val with leading spaces */
#if CONFIG_CUT_VAL_LEADING_SPACES
    __TEST(SP_TKN_VAL, "  abc", "\\  abc", 0);
#else
    __TEST(SP_TKN_VAL, "  abc", "  abc", 0);
#endif

    /* val with trailing spaces (last escaped by \x) */
    __TEST(SP_TKN_VAL, "abc  ", "abc \\x20", 0);

    /* val cuts tests */
    __TEST(SP_TKN_VAL, "0123456789", "0123456789", SPAR_F_CVLEN(10));

    __TEST(SP_TKN_VAL, "0123456789012345",
        "0123456789\\" UNX_EOL "012345",
        SPAR_F_CVLEN(10)|SPAR_F_CVEOL(EOL_LF));

    __TEST(SP_TKN_VAL, "01234567890123456789",
        "0123456789\\" WIN_EOL "0123456789",
        SPAR_F_CVLEN(10)|SPAR_F_CVEOL(EOL_CRLF));

    __TEST(SP_TKN_VAL, "01234567890123456789012",
        "0123456789\\" PLT_EOL "0123456789\\" PLT_EOL "012",
        SPAR_F_CVLEN(10)|SPAR_F_CVEOL(EOL_PLAT));

    __TEST(SP_TKN_VAL, "012345678\n12345 ",
        "012345678\\" PLT_EOL "\\n12345\\" PLT_EOL "\\x20",
        SPAR_F_CVLEN(10)|SPAR_F_CVEOL(EOL_PLAT));

finish:
    if (ret) printf("Error: %d\n", ret);
    return 0;

#undef __TEST
}
