/*
   Copyright (c) 2015 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

%code top
{
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "sp_props/parser.h"

/* converted new line char code */
#define NEWLN       (EOF-1)
#define is_space(c) (isspace(c) || (c)==NEWLN)

#define RESERVED_CHRS   "=;{}#"
#define is_nq_id(c) (!is_space(c) && !strchr(RESERVED_CHRS, (c)))

/* lexical contexts */
#define LCTX_GLOBAL     0
#define LCTX_VAL        1

/* lexical value type */
typedef struct _lexval_t {
    long beg;
    long end;
    int scope_lev;
} lexval_t;

#define YYSTYPE lexval_t

#define YYTOKENTYPE token_t
typedef token_t yytokentype;

}   /* code top */

%code provides
{
static int yylex(YYSTYPE*, YYLTYPE*, sp_parser_hndl_t*);
static void yyerror(YYLTYPE*, sp_parser_hndl_t*, char const*);

static void set_loc(
    sp_loc_t *p_loc, const YYSTYPE *p_lval, const YYLTYPE *p_lloc)
{
    p_loc->beg = p_lval->beg;
    p_loc->end = p_lval->end;
    p_loc->first_line = p_lloc->first_line;
    p_loc->first_column = p_lloc->first_column;
    p_loc->last_line = p_lloc->last_line;
    p_loc->last_column = p_lloc->last_column;
}

/* temporary macros indented for use in actions
 */
#define __CALL_CB_PROP(nm, val, def) { \
    long pos = ftell(p_hndl->in); \
    int res = p_hndl->cb.prop(p_hndl, (nm), (val), (def)); \
    if (!res && (pos==-1L || fseek(p_hndl->in, pos, SEEK_SET))) res=SPEC_ACCS_ERR; \
    if (res<0) YYACCEPT; \
    else if (res>0) { p_hndl->err.code=res; YYABORT; } \
}

#define __CALL_CB_SCOPE(typ, nm, bdy, def) { \
    long pos = ftell(p_hndl->in); \
    int res = p_hndl->cb.scope(p_hndl, (typ), (nm), (bdy), (def)); \
    if (!res && (pos==-1L || fseek(p_hndl->in, pos, SEEK_SET))) res=SPEC_ACCS_ERR; \
    if (res<0) YYACCEPT; \
    else if (res>0) { p_hndl->err.code=res; YYABORT; } \
}

#define __PREP_LOC_PTR(loc) ((loc).beg<=(loc).end ? &(loc) : (sp_loc_t*)NULL)

} /* code provides */

%locations
%define api.pure full
%param {sp_parser_hndl_t *p_hndl}

%token TKN_ID
%token TKN_VAL

%%

input:
/* empty */
    {
        /* set to empty scope */
        $$.end = 0;
        $$.beg = $$.end+1;
        $$.scope_lev = 0;
    }
| scoped_props
;

scoped_props:
  prop_scope
| scoped_props prop_scope
    {
        $$.beg = $1.beg;
        $$.end = $2.end;
        $$.scope_lev = $1.scope_lev;
    }
;

prop_scope:
  /* property with value
     NOTE: TKN_VAL may be empty to define property w/o value
   */
  TKN_ID '=' TKN_VAL
    {
        $$.beg = $1.beg;
        $$.end = $3.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.prop && !$$.scope_lev) {
            sp_loc_t lname, lval, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&lval, &$3, &@3);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_PROP(&lname, __PREP_LOC_PTR(lval), &ldef);
        }
    }
  /* property w/o value (alternative) */
| TKN_ID ';'
    {
        $$.beg = $1.beg;
        $$.end = $2.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.prop && !$$.scope_lev) {
            sp_loc_t lname, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_PROP(&lname, (sp_loc_t*)NULL, &ldef);
        }
    }
  /* untyped scope with properties */
| TKN_ID '{' input '}'
    {
        $$.beg = $1.beg;
        $$.end = $4.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.scope && !$$.scope_lev) {
            sp_loc_t lname, lbody, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&lbody, &$3, &@3);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_SCOPE((sp_loc_t*)NULL, &lname, __PREP_LOC_PTR(lbody), &ldef);
        }
    }
  /* scope with properties */
| TKN_ID TKN_ID '{' input '}'
    {
        $$.beg = $1.beg;
        $$.end = $5.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.scope && !$$.scope_lev) {
            sp_loc_t ltype, lname, lbody, ldef;
            set_loc(&ltype, &$1, &@1);
            set_loc(&lname, &$2, &@2);
            set_loc(&lbody, &$4, &@4);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_SCOPE(
                &ltype, __PREP_LOC_PTR(lname), __PREP_LOC_PTR(lbody), &ldef);
        }
    }
  /* scope w/o body (alternative)
     NOTE: to avoid ambiguity with no value property, the
     only way to define untyped scope w/o body is: TKN_ID '{' '}'
   */
| TKN_ID TKN_ID ';'
    {
        $$.beg = $1.beg;
        $$.end = $3.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.scope && !$$.scope_lev) {
            sp_loc_t ltype, lname, ldef;
            set_loc(&ltype, &$1, &@1);
            set_loc(&lname, &$2, &@2);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_SCOPE(&ltype, __PREP_LOC_PTR(lname), (sp_loc_t*)NULL, &ldef);
        }
    }
;

%%

#undef __PREP_LOC_PTR
#undef __CALL_CB_SCOPE
#undef __CALL_CB_PROP

/* get character from the input (lexer) */
static int lex_getc(sp_parser_hndl_t *p_hndl)
{
    int c;

    if (p_hndl->lex.end==-1L || p_hndl->lex.off<=p_hndl->lex.end)
    {
        c = unc_getc(&p_hndl->lex.unc, getc(p_hndl->in));
        if (c=='\r' || c=='\n')
        {
            /* new line marker conversion */
            switch (p_hndl->lex.nl_typ)
            {
            case NL_LF:
                if (c=='\n') c=NEWLN;
                break;
            case NL_CR_LF:
                if (c=='\r') {
                    if ((c=unc_getc(&p_hndl->lex.unc, getc(p_hndl->in)))=='\n') {
                        p_hndl->lex.off++;
                        c = NEWLN;
                    } else {
                        unc_ungetc(&p_hndl->lex.unc, c);
                        c = '\r';
                    }
                }
                break;
            case NL_CR:
                if (c=='\r') c=NEWLN;
                break;

            /* 1st occurrence of a new line marker */
            default:
                if (c=='\n') {
                    p_hndl->lex.nl_typ = NL_LF;
                } else {
                    if ((c=unc_getc(&p_hndl->lex.unc, getc(p_hndl->in)))=='\n') {
                        p_hndl->lex.off++;
                        p_hndl->lex.nl_typ = NL_CR_LF;
                    } else {
                        unc_ungetc(&p_hndl->lex.unc, c);
                        p_hndl->lex.nl_typ = NL_CR;
                    }
                }
                c = NEWLN;
                break;
            }
        }
    } else
        c=EOF;
    return c;
}

/* lexical parser */
static int yylex(YYSTYPE *p_lval, YYLTYPE *p_lloc, sp_parser_hndl_t *p_hndl)
{
    /* lexical parser SM states */
    typedef enum _lex_state_t
    {
        /* token not recognized yet, ignore leading spaces */
        LXST_INIT=0,

        /* comment; starts with '#' to the end of a line */
        LXST_COMMENT,

        /* TKN_ID token */
        LXST_ID,            /* non quoted */
        LXST_ID_QUOTED,     /* quoted */

        /* TKN_VAL token; any chars up to the end of a line or semicolon */
        LXST_VAL_INIT,      /* ignore leading spaces */
        LXST_VAL            /* track the token */
    } lex_state_t;

    long last_off;
    int token=0, finish=0, c, last_col, escaped=0, quot_chr;
    lex_state_t state =
        (p_hndl->lex.ctx==LCTX_GLOBAL ? LXST_INIT : LXST_VAL_INIT);

#define __CHAR_TOKEN(t) \
    p_lloc->first_column = p_lloc->last_column = p_hndl->lex.col; \
    p_lloc->first_line = p_lloc->last_line = p_hndl->lex.line; \
    p_lval->beg = p_lval->end = p_hndl->lex.off; \
    token = (t);

#define __MCHAR_TOKEN_BEG(t) \
    p_lloc->first_column = p_hndl->lex.col; \
    p_lloc->first_line = p_hndl->lex.line; \
    last_col = p_hndl->lex.col; \
    p_lval->beg = p_hndl->lex.off; \
    last_off = p_hndl->lex.off; \
    token = (t);

#define __MCHAR_UPDATE_TAIL() \
    last_col = p_hndl->lex.col; \
    last_off = p_hndl->lex.off;

#define __MCHAR_TOKEN_END() \
    p_lloc->last_column = last_col; \
    p_lloc->last_line = p_hndl->lex.line; \
    p_lval->end = last_off;

#define __INIT_ESC() \
    int esc = escaped; \
    escaped = (!esc && c=='\\' ? 1 : 0);

    while (!finish && (c=lex_getc(p_hndl))!=EOF)
    {
        switch (state)
        {
        case LXST_INIT:
            if (is_space(c));
            else
            if (c=='#') {
                state=LXST_COMMENT;
            } else
            if (is_nq_id(c)) {
                __MCHAR_TOKEN_BEG(TKN_ID);
                if (c=='"' || c=='\'') {
                    quot_chr = c;
                    state = LXST_ID_QUOTED;
                } else {
                    state = LXST_ID;
                }
            } else {
                __CHAR_TOKEN(c);
                finish++;
            }
            break;

        case LXST_COMMENT:
            if (c==NEWLN) state=LXST_INIT;
            break;

        case LXST_ID:
            if (!is_nq_id(c)) {
                __MCHAR_TOKEN_END();
                unc_ungetc(&p_hndl->lex.unc, c);
                finish++;
                continue;
            } else {
                __MCHAR_UPDATE_TAIL();
            }
            break;

        case LXST_ID_QUOTED:
          {
            __INIT_ESC();
            if (c==NEWLN) {
                /* error: quoted id need to be finished by apostrophe */
                __MCHAR_TOKEN_END();
                finish++;
                token = YYERRCODE;
            } else {
                __MCHAR_UPDATE_TAIL();
                if (c==quot_chr && !esc) {
                    __MCHAR_TOKEN_END();
                    finish++;
                }
            }
            break;
          }

        case LXST_VAL_INIT:
          {
            __INIT_ESC();
            if ((c==NEWLN || c==';') && !esc) {
                __CHAR_TOKEN(TKN_VAL);
                /* mark TKN_VAL as empty */
                p_lval->beg++;
                finish++;
            } else
            if (!is_space(c)) {
                __MCHAR_TOKEN_BEG(TKN_VAL);
                state=LXST_VAL;
            }
            break;
          }

        case LXST_VAL:
          {
            __INIT_ESC();
            if ((c==NEWLN || c==';') && !esc) {
                __MCHAR_TOKEN_END();
                finish++;
            } else
            if (!is_space(c)) {
                __MCHAR_UPDATE_TAIL();
            }
            break;
          }
        }

        /* track location of the next char to read */
        if (c==NEWLN) {
            p_hndl->lex.line++;
            p_hndl->lex.col=1;
        } else {
            p_hndl->lex.col++;
        }
        p_hndl->lex.off++;
    }

    if (c==EOF && (token==TKN_ID || token==TKN_VAL))
    {
        /* EOF finishes TKN_ID/TKN_VAL tokens, except
           quoted id which need to be finished by apostrophe */
        if (state==LXST_ID_QUOTED) {
            token = YYERRCODE;
        } else {
            __MCHAR_TOKEN_END();
        }
    }

    /* scope level update */
    p_lval->scope_lev = p_hndl->lex.scope_lev;
    if (token=='{')
        p_hndl->lex.scope_lev++;
    else
    if (token=='}') {
        p_hndl->lex.scope_lev--;
        p_lval->scope_lev--;
    }

    /* lexical context update */
    if (p_hndl->lex.ctx==LCTX_VAL) {
        p_hndl->lex.ctx=LCTX_GLOBAL;
    } else
    if (token=='=')
        p_hndl->lex.ctx=LCTX_VAL;

#ifdef DEBUG
    printf("token 0x%03x: lval 0x%02lx:0x%02lx, lloc %d:%d|%d:%d, scope %d\n",
        token, p_lval->beg, p_lval->end, p_lloc->first_line, p_lloc->first_column,
        p_lloc->last_line, p_lloc->last_column, p_lval->scope_lev);
#endif

    return token;

#undef __INIT_ESC
#undef __MCHAR_TOKEN_END
#undef __MCHAR_UPDATE_TAIL
#undef __MCHAR_TOKEN_BEG
#undef __CHAR_TOKEN
}

/* parser error handler */
static void yyerror(YYLTYPE *p_lloc, sp_parser_hndl_t *p_hndl, char const *msg)
{
    p_hndl->err.code = SPEC_SYNTAX;
    p_hndl->err.loc.line = p_lloc->first_line;
    p_hndl->err.loc.col = p_lloc->first_column;
}

/* exported; see header for details */
sp_errc_t sp_parser_hndl_init(sp_parser_hndl_t *p_hndl,
    FILE *in, const sp_loc_t *p_parsc, sp_parser_cb_prop_t cb_prop,
    sp_parser_cb_scope_t cb_scope, void *cb_arg)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_loc_t globsc = {0, -1L, 1, 1, -1, -1};
    if (!p_parsc) p_parsc=&globsc;

    if (!p_hndl || !in) { ret=SPEC_INV_ARG; goto finish; }
    if (fseek(in, p_parsc->beg, SEEK_SET)) { ret=SPEC_ACCS_ERR; goto finish; }

    p_hndl->in = in;

    p_hndl->lex.nl_typ = NL_UNDEF;
    p_hndl->lex.line = p_parsc->first_line;
    p_hndl->lex.col = p_parsc->first_column;
    p_hndl->lex.off = p_parsc->beg;
    p_hndl->lex.end = p_parsc->end;
    p_hndl->lex.scope_lev = 0;
    p_hndl->lex.ctx = LCTX_GLOBAL;
    unc_clean(&p_hndl->lex.unc);

    p_hndl->cb.arg = cb_arg;
    p_hndl->cb.prop = cb_prop;
    p_hndl->cb.scope = cb_scope;

    p_hndl->err.code = SPEC_SUCCESS;
    p_hndl->err.loc.line = 0;
    p_hndl->err.loc.col = 0;

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_parse(sp_parser_hndl_t *p_hndl)
{
    sp_errc_t ret;

    if (!p_hndl) { ret=SPEC_INV_ARG; goto finish; }

    switch (yyparse(p_hndl))
    {
    case 0:
        ret = p_hndl->err.code = SPEC_SUCCESS;
        break;
    default:
    case 1:
        if (p_hndl->err.code==SPEC_SUCCESS)
            ret = p_hndl->err.code = SPEC_SYNTAX;
        else
            ret = p_hndl->err.code;
        break;
    case 2:
        ret = p_hndl->err.code = SPEC_NOMEM;
        break;
    }
finish:
    return ret;
}

typedef struct _hndl_eschr_t
{
    struct {
        int is_str;
        union {
            /* file; is_str==0 */
            FILE *f;

            /* string; is_str!=0 */
            struct {
                const char *str;
                size_t max_num; /* max number of chars to be read */
                size_t idx;     /* currently read char index */
            } tab;
        };
        unc_cache_t unc;    /* ungetc cache buffer */
    } in;

    token_t tkn;        /* type of token which content is to be escaped */
    newln_t nl_typ;     /* new line type */
    int quot_chr;       /* quotation char (TKN_ID token) */

    /* set by esc_getc() after each call */
    size_t n_rdc;       /* number of read chars so far */
    int escaped;        /* the returned char is escaped */
} hndl_eschr_t;

/* initialize esc_getc() handler; stream input */
static void init_hndl_eschr_stream(hndl_eschr_t *p_hndl,
    const sp_parser_hndl_t *p_phndl, token_t tkn, int id_apost)
{
    p_hndl->in.is_str = 0;
    p_hndl->in.f = p_phndl->in;
    unc_clean(&p_hndl->in.unc);

    p_hndl->tkn = tkn;
    p_hndl->nl_typ = p_phndl->lex.nl_typ;
    p_hndl->quot_chr = 0;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;

    if (id_apost && tkn==TKN_ID) {
        int c=getc(p_phndl->in);
        if (c=='"' || c=='\'') {
            p_hndl->quot_chr=c;
            p_hndl->n_rdc++;
        } else {
            unc_ungetc(&p_hndl->in.unc, c);
        }
    }
}

/* initialize esc_getc() handler; string input */
static void init_hndl_eschr_string(hndl_eschr_t *p_hndl,
    const char *str, size_t max_num, token_t tkn, newln_t nl_typ, int id_apost)
{
    p_hndl->in.is_str = 1;
    p_hndl->in.tab.str = str;
    p_hndl->in.tab.max_num = max_num;
    p_hndl->in.tab.idx = 0;
    unc_clean(&p_hndl->in.unc);

    p_hndl->tkn = tkn;
    p_hndl->nl_typ = nl_typ;
    p_hndl->quot_chr = 0;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;

    if (id_apost && tkn==TKN_ID && p_hndl->in.tab.max_num) {
        int c = p_hndl->in.tab.str[p_hndl->in.tab.idx++];
        if (c=='"' || c=='\'') {
            p_hndl->quot_chr = c;
            p_hndl->n_rdc++;
        } else {
            unc_ungetc(&p_hndl->in.unc, (!c ? EOF : c));
        }
    }
}

/* get single char from hndl_eschr_t handle */
static int noesc_getc(hndl_eschr_t *p_hndl)
{
    int c;
    if (p_hndl->in.is_str)
    {
        int str_c;
        if (p_hndl->in.tab.idx < p_hndl->in.tab.max_num) {
            if (!(str_c=(int)p_hndl->in.tab.str[p_hndl->in.tab.idx])) str_c=EOF;
        } else
            str_c=EOF;

        c = unc_getc(&p_hndl->in.unc,
            (str_c!=EOF ? (p_hndl->in.tab.idx++, str_c) : str_c));
    } else {
        c = unc_getc(&p_hndl->in.unc, getc(p_hndl->in.f));
    }
    return c;
}

/* get and escape (token dependent) single char from hndl_eschr_t handle */
static int esc_getc(hndl_eschr_t *p_hndl)
{
    int c;
    size_t n_esc=0;
    int esc[8];

#define __GETC() ((c=noesc_getc(p_hndl)), (esc[n_esc++]=c))

#define __HEXCHR2BT(chr, out) \
    ((((out)=(chr)-'0')>=0 && (out)<=9) ? 1 : \
    ((((out)=(chr)-'A')>=0 && (out)<=5) ? ((out)+=10, 1) : \
    ((((out)=(chr)-'a')>=0 && (out)<=5) ? ((out)+=10, 1) : 0)))

    p_hndl->escaped=0;
    if (__GETC()=='\\')
    {
        p_hndl->escaped=1;
        switch (__GETC())
        {
        case 'a':
            c='\a';
            break;
        case 'b':
            c='\b';
            break;
        case 'f':
            c='\f';
            break;
        case 'n':
            c='\n';
            break;
        case 'r':
            c='\r';
            break;
        case 't':
            c='\t';
            break;
        case 'v':
            c='\v';
            break;
        case '\\':
            break;

        /* hex encoded char */
        case 'x':
          {
            int c1, c2;
            __GETC(); if (!__HEXCHR2BT(c, c1)) { p_hndl->escaped=0; break; }
            __GETC(); if (!__HEXCHR2BT(c, c2)) { p_hndl->escaped=0; break; }
            c = (c1<<4)|c2;
            break;
          }

        /* TKN_ID: single/double apostrophe */
        case '"':
        case '\'':
            if (p_hndl->tkn!=TKN_ID || p_hndl->quot_chr!=c) p_hndl->escaped=0;
            break;

        /* TKN_VAL: semicolon */
        case ';':
            if (p_hndl->tkn!=TKN_VAL) p_hndl->escaped=0;
            break;

        /* TKN_VAL: line continuation */
        case '\n':
        case '\r':
            if (p_hndl->tkn!=TKN_VAL)
                p_hndl->escaped=0;
            else
            switch (p_hndl->nl_typ)
            {
            case NL_LF:
                if (c=='\n') {
                    __GETC();
                } else
                    p_hndl->escaped=0;
                break;
            case NL_CR_LF:
                if (c=='\r' && __GETC()=='\n') {
                    __GETC();
                } else
                    p_hndl->escaped=0;
                break;
            case NL_CR:
                if (c=='\r') {
                    __GETC();
                } else
                    p_hndl->escaped=0;
                break;
            default:
                p_hndl->escaped=0;
                break;
            }
            break;

        default:
            p_hndl->escaped=0;
            break;
        }
    }

    if (!p_hndl->escaped) {
        while (n_esc) {
            n_esc--;
            if (n_esc) {
                unc_ungetc(&p_hndl->in.unc, esc[n_esc]);
            } else
                c = esc[n_esc];
        }
        if (c!=EOF) p_hndl->n_rdc++;
    } else
        p_hndl->n_rdc += n_esc-(c==EOF ? 1 : 0);

    return c;

#undef __HEXCHR2BT
#undef __GETC
}

/* get, escape single char from hndl_eschr_t handle,
   if quotation char occurs it's re-read */
static int esc_reqout_getc(hndl_eschr_t *p_hndl)
{
    int c;
reread:
    c = esc_getc(p_hndl);
    if (p_hndl->quot_chr && c==p_hndl->quot_chr && !p_hndl->escaped) {
        p_hndl->quot_chr=0;
        goto reread;
    }
    return c;
}

/* exported; see header for details */
sp_errc_t sp_parser_tkn_cpy(const sp_parser_hndl_t *p_phndl, token_t tkn,
    const sp_loc_t *p_loc, char *p_buf, size_t buf_len, long *p_tklen)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int c=0;
    size_t i=0;
    hndl_eschr_t eh;
    long llen=sp_loc_len(p_loc);

    if (!llen || (!buf_len && !p_tklen)) goto finish;
    if (p_tklen) *p_tklen=0;

    ret=SPEC_ACCS_ERR;
    if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;

    init_hndl_eschr_stream(&eh, p_phndl, tkn, 1);

    while (eh.n_rdc<(size_t)llen && (buf_len || p_tklen))
    {
        if ((c=esc_getc(&eh))==EOF) break;
        if (eh.quot_chr && c==eh.quot_chr && !eh.escaped) {
            eh.quot_chr=0;
        } else {
            if (p_tklen) (*p_tklen)++;
            if (buf_len) {
                p_buf[i++] = (char)c;
                buf_len--;
            }
        }
    }
    if (c!=EOF) ret=SPEC_SUCCESS;

finish:
    if (buf_len) p_buf[i]=0;
    return ret;
}

/* exported; see header for details */
int sp_parser_tkn_cmp(const sp_parser_hndl_t *p_phndl, token_t tkn,
    const sp_loc_t *p_loc, const char *str, size_t max_num)
{
    int ret=0, c_tkn, c_str;
    hndl_eschr_t eh_tkn, eh_str;
    long llen=sp_loc_len(p_loc);

#define __CHK_STREAM() \
    if (eh_tkn.n_rdc<=(size_t)llen) { if (c_tkn==EOF) goto finish; } else c_tkn=EOF;

    if (!max_num && !llen) goto finish;

    ret=SPEC_ACCS_ERR;
    if (llen) {
        if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;
        init_hndl_eschr_stream(&eh_tkn, p_phndl, tkn, 1);
        c_tkn = esc_reqout_getc(&eh_tkn);
        __CHK_STREAM();
    } else
        c_tkn=EOF;

    if (max_num) {
        init_hndl_eschr_string(&eh_str, str, max_num, tkn, NL_UNDEF, 0);
        if (eh_str.quot_chr) max_num--;
        c_str = esc_reqout_getc(&eh_str);
    } else
        c_str=EOF;

    do {
        if (c_tkn!=c_str) break;

        c_tkn = esc_reqout_getc(&eh_tkn);
        __CHK_STREAM();
        c_str = esc_reqout_getc(&eh_str);
    } while (c_tkn!=EOF && c_str!=EOF);

    ret = (c_tkn==c_str ? 0 : -1);

finish:
    return ret;

#undef __CHK_STREAM
}
