/*
   Copyright (c) 2016,2019 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* More advanced scopes usage.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sprops/props.h"
#include "sprops/utils.h"

/* example specific errors */
#define ERR_EMPTY_STORE_ITM  SPEC_CB_BASE
#define ERR_UNKWN_STORE_ITM  (SPEC_CB_BASE+1)

/* body location of "books" and "movies" scopes in "items" super-scope */
static sp_loc_t sc_books;
static sp_loc_t sc_movies;

/* Iterate callback to read store items.
 */
static sp_errc_t store_iter_cb(
    void *arg, SP_FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
    const char *name, const sp_tkn_info_t *p_tkname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    long price=0, stock=0, year=0;
    char title[48], audir[32];
    char *audir_nm=NULL;
    sp_loc_t *p_sc=NULL;

    if (!p_lbody)
        /* empty store item - finish with error */
        return ERR_EMPTY_STORE_ITM;

    /* get properties from the iterated item (book or movie)
     */
    sp_get_prop_int(in,
        p_lbody,            /* parse iterated scope */
        "price", 0,
        NULL,               /* root path */
        NULL, &price, NULL);

    sp_get_prop_int(in,
        p_lbody,
        "stock", 0,
        NULL,
        NULL, &stock, NULL);

    /* scope type specifies if it's a book or movie */
    if (!strcmp(type, "book")) {
        p_sc = &sc_books;   /* preserved scope to parse: "items/books" */
        audir_nm = "author";
        printf("  Book:\n");
    } else
    if (!strcmp(type, "movie")) {
        p_sc = &sc_movies;  /* preserved scope to parse: "items/movies" */
        audir_nm = "director";
        printf("  Movie:\n");
    } else
        /* unknown scope - finish with error */
        return ERR_UNKWN_STORE_ITM;

    /* Name of the scope is an id used to fetch an item from "items"
       scopes: "movies" or "books" (depending on the item's type). Since
       location of these scopes have been preserved, we may use them
       to get an item related props with more efficient way (there is no
       need to parse the whole configuration to find a scope of interest).
     */
    if (sp_get_prop(in,
        p_sc,               /* parse preserved scope */
        "title", 0,
        name,               /* iterated scope name as an item id */
        NULL, title, sizeof(title), NULL)!=SPEC_SUCCESS) *title=0;

    if (sp_get_prop(in,
        p_sc,
        audir_nm, 0,
        name,
        NULL, audir, sizeof(audir), NULL)!=SPEC_SUCCESS) *audir=0;

    sp_get_prop_int(in,
        p_sc,
        "year", 0,
        name,
        NULL, &year, NULL);

    printf(
        "    title: %s\n    %s: %s\n    year: %d\n"
        "    price: %d\n    stock: %d\n",
        title, audir_nm, audir, (int)year, (int)price, (int)stock);

    /* continue iteration */
    return SPEC_SUCCESS;
}

int main(void)
{
    SP_FILE in;
    long pr, pr2;
    char title[48];

    sp_scope_info_ex_t sci;
    sp_synerr_t synerr;

    /* temp bufs for scopes iteration callback */
    char tp_buf[32], nm_buf[32];

    if (sp_fopen(&in, "store.conf", SP_MODE_READ) != SPEC_SUCCESS) {
        printf("Can't open the confing: %s\n", strerror(errno));
        return 1;
    }

    /* check syntax of the config file before further processing */
    if (sp_check_syntax(&in, NULL, &synerr)==SPEC_SYNTAX) {
        printf("Syntax error %d at line: %d\n", synerr.code, synerr.loc.line);
        sp_close(&in);
        return 1;
    }

    /* get sub-scopes body locations of "items" super-scope to
       effectively reach for their content in further part of the example */
    assert(sp_get_scope_info(&in,
            NULL,           /* global scope */
            NULL, "books",  /* untyped scope "books" */
            0,
            "items",        /* path to super-scope */
            NULL, &sci)==SPEC_SUCCESS &&
        sci.body_pres);
    sc_books = sci.lbody;

    assert(sp_get_scope_info(&in,
            NULL,
            NULL, "movies",
            0,
            "items",
            NULL, &sci)==SPEC_SUCCESS &&
        sci.body_pres);
    sc_movies = sci.lbody;

    /* iterate store items */
    printf("Store items:\n");
    sp_iterate(&in,
        NULL,               /* global scope */
        "store",
        NULL,
        NULL,               /* no prop-iteration callback */
        store_iter_cb,
        NULL,               /* no custom args */
        tp_buf, sizeof(tp_buf), nm_buf, sizeof(nm_buf));

    /* The library allows to reach for specific properties inside
       their scopes on various ways. Some of examples go below.
     */

    assert(sp_get_prop_int(&in,
        NULL,           /* global scope */
        "price", 0,
        "store/book:1", /* scope of type "book" with name "1" inside "store" */
        NULL,           /* no default scope type */
        &pr, NULL) == SPEC_SUCCESS);

    assert(sp_get_prop_int(&in,
        NULL,
        "price", 0,
        ":store/1",     /* "store" scope is untyped therefore need to be
                           preceded by ':' to avoid default scope type usage */
        "book",         /* "book" as default scope type */
        &pr2, NULL) == SPEC_SUCCESS);

    assert(pr==pr2);
    printf("\nPrice of book with id 1 is: %d\n", (int)pr);

    assert(sp_get_prop_int(&in,
        NULL,
        "price", 0,
        "store/movie:1",
        NULL, &pr, NULL) == SPEC_SUCCESS);
    printf("Price of movie with id 1 is: %d\n", (int)pr);

    /* reach for the books inventory directly from the global scope
       (not via the preserved scope as in store_iter_cb() callback) */
    assert(sp_get_prop(&in,
        NULL,
        "title", 0,
        "items/books/2",    /* book with id "2" in books items scope */
        NULL, title, sizeof(title), NULL)==SPEC_SUCCESS);

    printf("Title of book with id 2: %s\n", title);

    sp_close(&in);
    return 0;
}
