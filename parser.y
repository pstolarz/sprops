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
#include "config.h"
#include "sp_props/parser.h"

/* EOL char code (platform independent) */
#define EOL (EOF-1)
#define is_space(c) (isspace(c) || (c)==EOL)

#define RESERVED_CHRS   "=;{}#"
#define is_nq_id(c) (!is_space(c) && !strchr(RESERVED_CHRS, (c)))

#define unc_clean(unc) ((unc)->inbuf=0)
#define unc_getc(unc, def) ((unc)->inbuf ? (unc)->buf[--((unc)->inbuf)] : (def))
#define unc_ungetc(unc, c) ((unc)->buf[(unc)->inbuf++]=(c))

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

#define YYTOKENTYPE sp_parser_token_t
typedef sp_parser_token_t yytokentype;

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
    sp_errc_t res = p_hndl->cb.prop(p_hndl, (nm), (val), (def)); \
    if (res==SPEC_SUCCESS && \
        (pos==-1L || fseek(p_hndl->in, pos, SEEK_SET))) res=SPEC_ACCS_ERR; \
    if ((int)res>0) { p_hndl->err.code=res; YYABORT; } \
    else if ((int)res<0) { YYACCEPT; } \
}

#define __CALL_CB_SCOPE(typ, nm, bdy, def) { \
    long pos = ftell(p_hndl->in); \
    sp_errc_t res = p_hndl->cb.scope(p_hndl, (typ), (nm), (bdy), (def)); \
    if (res==SPEC_SUCCESS && \
        (pos==-1L || fseek(p_hndl->in, pos, SEEK_SET))) res=SPEC_ACCS_ERR; \
    if ((int)res>0) { p_hndl->err.code=res; YYABORT; } \
    else if ((int)res<0) { YYACCEPT; } \
}

#define __IS_EMPTY(loc) ((loc).beg>(loc).end)
#define __PREP_LOC_PTR(loc) (__IS_EMPTY(loc) ? (sp_loc_t*)NULL : &(loc))

} /* code provides */

%locations
%define api.pure full
%param {sp_parser_hndl_t *p_hndl}

%token SP_TKN_ID
%token SP_TKN_VAL

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
  /* property with a value (EOL/EOF finished)
     NOTE: SP_TKN_VAL may be empty to define property w/o a value
   */
  SP_TKN_ID '=' SP_TKN_VAL
    {
        sp_loc_t lval;
        set_loc(&lval, &$3, &@3);

        $$.beg = $1.beg;
        $$.scope_lev = $1.scope_lev;
        if (__IS_EMPTY(lval)) {
            $$.end = $2.end;
            @$.last_line = @2.last_line;
            @$.last_column = @2.last_column;
        } else {
            $$.end = $3.end;
            @$.last_line = @3.last_line;
            @$.last_column = @3.last_column;
        }

        if (p_hndl->cb.prop && !$$.scope_lev) {
            sp_loc_t lname, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_PROP(&lname, __PREP_LOC_PTR(lval), &ldef);
        }
    }
  /* property with a value (semicolon finished)
     NOTE: valid only if NO_SEMICOL_ENDS_VAL is not defined
     NOTE: SP_TKN_VAL may be empty to define property w/o a value
   */
| SP_TKN_ID '=' SP_TKN_VAL ';'
    {
        $$.beg = $1.beg;
        $$.end = $4.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.prop && !$$.scope_lev) {
            sp_loc_t lname, lval, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&lval, &$3, &@3);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_PROP(&lname, __PREP_LOC_PTR(lval), &ldef);
        }
    }
  /* property w/o a value (alternative) */
| SP_TKN_ID ';'
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
| SP_TKN_ID '{' input '}'
    {
        $$.beg = $1.beg;
        $$.end = $4.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.scope && !$$.scope_lev) {
            sp_loc_t lname, lbody, ldef;
            set_loc(&lname, &$1, &@1);
            set_loc(&lbody, &$3, &@3);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_SCOPE(
                (sp_loc_t*)NULL, &lname, __PREP_LOC_PTR(lbody), &ldef);
        }
    }
  /* scope with properties */
| SP_TKN_ID SP_TKN_ID '{' input '}'
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
                &ltype, &lname, __PREP_LOC_PTR(lbody), &ldef);
        }
    }
  /* scope w/o a body (alternative)
     NOTE: to avoid ambiguity with no value property, the
     only way to define untyped scope w/o a body is: SP_TKN_ID '{' '}'
   */
| SP_TKN_ID SP_TKN_ID ';'
    {
        $$.beg = $1.beg;
        $$.end = $3.end;
        $$.scope_lev = $1.scope_lev;

        if (p_hndl->cb.scope && !$$.scope_lev) {
            sp_loc_t ltype, lname, ldef;
            set_loc(&ltype, &$1, &@1);
            set_loc(&lname, &$2, &@2);
            set_loc(&ldef, &$$, &@$);
            __CALL_CB_SCOPE(
                &ltype, &lname, (sp_loc_t*)NULL, &ldef);
        }
    }
;

%%

#undef __PREP_LOC_PTR
#undef __IS_EMPTY
#undef __CALL_CB_SCOPE
#undef __CALL_CB_PROP

/* Get character from the input (lexer) */
static int lex_getc(sp_parser_hndl_t *p_hndl)
{
    int c;

    if (p_hndl->lex.end==-1L || p_hndl->lex.off<=p_hndl->lex.end)
    {
        c = unc_getc(&p_hndl->lex.unc, fgetc(p_hndl->in));
        if (c=='\r' || c=='\n')
        {
            /* EOL conversion */
            switch (p_hndl->lex.eol_typ)
            {
            case EOL_LF:
                if (c=='\n') c=EOL;
                break;
            case EOL_CRLF:
                if (c=='\r') {
                    if ((c=unc_getc(&p_hndl->lex.unc, fgetc(p_hndl->in)))=='\n')
                    {
                        p_hndl->lex.off++;
                        c = EOL;
                    } else {
                        unc_ungetc(&p_hndl->lex.unc, c);
                        c = '\r';
                    }
                }
                break;
            case EOL_CR:
                if (c=='\r') c=EOL;
                break;
            default:    /* will never happen */
                break;
            }
        }
    } else
        c=EOF;
    return c;
}

/* Lexical parser */
static int yylex(YYSTYPE *p_lval, YYLTYPE *p_lloc, sp_parser_hndl_t *p_hndl)
{
    /* lexical parser SM states */
    typedef enum _lex_state_t
    {
        /* token not recognized yet; initial state */
        LXST_INIT=0,

        /* comment; starts with '#' to the end of a line */
        LXST_COMMENT,

        /* SP_TKN_ID token */
        LXST_ID,            /* non quoted */
        LXST_ID_QUOTED,     /* quoted */

        /* SP_TKN_VAL token; any chars up to the end of a line or semicolon
           (if NO_SEMICOL_ENDS_VAL is not defined); line continuation allowed */
        LXST_VAL_INIT,      /* value tracking initial state */
        LXST_VAL            /* value tracking */
    } lex_state_t;

    long last_off;
    int token=0, endloop=0, c, last_col, last_ln, escaped=0, quot_chr;
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
    last_ln = p_hndl->lex.line; \
    p_lval->beg = p_hndl->lex.off; \
    last_off = p_hndl->lex.off; \
    token = (t);

#define __MCHAR_UPDATE_TAIL() \
    last_col = p_hndl->lex.col; \
    last_ln = p_hndl->lex.line; \
    last_off = p_hndl->lex.off;

#define __MCHAR_TOKEN_END() \
    p_lloc->last_column = last_col; \
    p_lloc->last_line = last_ln; \
    p_lval->end = last_off;

#define __INIT_ESC() \
    int esc = escaped; \
    escaped = (!esc && c=='\\' ? 1 : 0);

    while (!endloop && (c=lex_getc(p_hndl))!=EOF)
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
                __MCHAR_TOKEN_BEG(SP_TKN_ID);
                if (c=='"' || c=='\'') {
                    quot_chr = c;
                    state = LXST_ID_QUOTED;
                } else {
                    state = LXST_ID;
                }
            } else {
                __CHAR_TOKEN(c);
                endloop=1;
            }
            break;

        case LXST_COMMENT:
            if (c==EOL) state=LXST_INIT;
            break;

        case LXST_ID:
            if (!is_nq_id(c)) {
                __MCHAR_TOKEN_END();
                endloop=1;
                unc_ungetc(&p_hndl->lex.unc, c);
                continue;
            } else {
                __MCHAR_UPDATE_TAIL();
            }
            break;

        case LXST_ID_QUOTED:
          {
            __INIT_ESC();
            if (c==EOL) {
                /* error: quoted id need to be finished by quotation mark */
                __MCHAR_TOKEN_END();
                endloop=1;
                token = YYERRCODE;
            } else {
                __MCHAR_UPDATE_TAIL();
                if (c==quot_chr && !esc) {
                    __MCHAR_TOKEN_END();
                    endloop=1;
                }
            }
            break;
          }

        case LXST_VAL_INIT:
          {
            __INIT_ESC();
#ifdef NO_SEMICOL_ENDS_VAL
            if (c==EOL && !esc)
#else
            if ((c==EOL && !esc) || c==';')
#endif
            {
                /* mark token as empty */
                __CHAR_TOKEN(SP_TKN_VAL);
                p_lval->beg++;
                endloop=1;
#ifndef NO_SEMICOL_ENDS_VAL
                if (c==';') {
                    unc_ungetc(&p_hndl->lex.unc, c);
                    continue;
                }
#endif
            } else
#ifdef CUT_VAL_LEADING_SPACES
            if (!isspace(c))
#endif
            {
                __MCHAR_TOKEN_BEG(SP_TKN_VAL);
                state=LXST_VAL;
            }
            break;
          }

        case LXST_VAL:
          {
            __INIT_ESC();
#ifdef NO_SEMICOL_ENDS_VAL
            if (c==EOL && !esc)
#else
            if ((c==EOL && !esc) || c==';')
#endif
            {
                __MCHAR_TOKEN_END();
                endloop=1;
#ifndef NO_SEMICOL_ENDS_VAL
                if (c==';') {
                    unc_ungetc(&p_hndl->lex.unc, c);
                    continue;
                }
#endif
            } else
#ifdef TRIM_VAL_TRAILING_SPACES
            if (!isspace(c))
#endif
            {
                __MCHAR_UPDATE_TAIL();
            }
            break;
          }
        }

        /* track location of the next char to read */
        if (c==EOL) {
            p_hndl->lex.line++;
            p_hndl->lex.col=1;
        } else {
            p_hndl->lex.col++;
        }
        p_hndl->lex.off++;
    }

    if (c==EOF) {
        if (state==LXST_VAL_INIT) {
            /* EOF occurs before SP_TKN_VAL token get started; return SP_TKN_VAL
               empty token */
            __CHAR_TOKEN(SP_TKN_VAL);
            p_lval->beg++;
        } else
        if (token==SP_TKN_ID || token==SP_TKN_VAL) {
            /* EOF finishes SP_TKN_ID/SP_TKN_VAL tokens, except quoted
               SP_TKN_ID which need to be finished by quotation mark */
            if (state==LXST_ID_QUOTED) {
                token = YYERRCODE;
            } else {
                __MCHAR_TOKEN_END();
            }
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

    /* empty SP_TKN_ID tokens are not accepted */
    if (state==LXST_ID_QUOTED && p_lval->end-p_lval->beg+1<=2)
        token = YYERRCODE;

#ifdef DEBUG
    printf("token 0x%03x: lval 0x%02lx|0x%02lx, lloc %d.%d|%d.%d, scope %d\n",
        token, p_lval->beg, p_lval->end, p_lloc->first_line,
        p_lloc->first_column, p_lloc->last_line, p_lloc->last_column,
        p_lval->scope_lev);
#endif

    return token;

#undef __INIT_ESC
#undef __MCHAR_TOKEN_END
#undef __MCHAR_UPDATE_TAIL
#undef __MCHAR_TOKEN_BEG
#undef __CHAR_TOKEN
}

/* Parser error handler */
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
    int c;
    sp_errc_t ret;
    sp_loc_t globsc = {0, -1L, 1, 1, -1, -1};
    if (!p_parsc) p_parsc=&globsc;

    if (!p_hndl || !in) { ret=SPEC_INV_ARG; goto finish; }

    ret=SPEC_ACCS_ERR;

    /* detect EOL */
    p_hndl->lex.eol_typ = EOL_UNDEF;
    if (fseek(in, 0, SEEK_SET)) goto finish;
    while ((c=fgetc(in))!=EOF) {
        if (c=='\n') {
            p_hndl->lex.eol_typ=EOL_LF;
            break;
        } else
        if (c=='\r') {
            p_hndl->lex.eol_typ=EOL_CR;
            if (fgetc(in)=='\n') p_hndl->lex.eol_typ=EOL_CRLF;
            break;
        }
    }

    if (fseek(in, p_parsc->beg, SEEK_SET)) goto finish;

    p_hndl->in = in;

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

    ret=SPEC_SUCCESS;

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_parse(sp_parser_hndl_t *p_hndl)
{
    sp_errc_t ret=SPEC_SUCCESS;
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
                const char *buf;    /* string buffer of chars */
                size_t max_num;     /* max number of chars to be read */
                size_t idx;         /* currently read char index */
            } str;
        };
        unc_cache_t unc;    /* ungetc cache buffer */
    } input;

    sp_parser_token_t tkn;  /* type of token which content is to be escaped */
    eol_t eol_typ;      /* EOL type */
    int quot_chr;       /* quotation char (SP_TKN_ID token) */

    /* set by esc_getc() after each call */
    size_t n_rdc;       /* number of read chars so far */
    int escaped;        /* the returned char is escaped */
} hndl_eschr_t;

/* Initialize esc_getc() handler; stream input */
static void init_hndl_eschr_stream(hndl_eschr_t *p_hndl,
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn, int quot_allowed)
{
    p_hndl->input.is_str = 0;
    p_hndl->input.f = p_phndl->in;
    unc_clean(&p_hndl->input.unc);

    p_hndl->tkn = tkn;
    p_hndl->eol_typ = p_phndl->lex.eol_typ;
    p_hndl->quot_chr = 0;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;

    if (quot_allowed && tkn==SP_TKN_ID) {
        int c=fgetc(p_phndl->in);
        if (c=='"' || c=='\'') {
            p_hndl->quot_chr=c;
            p_hndl->n_rdc++;
        } else {
            unc_ungetc(&p_hndl->input.unc, c);
        }
    }
}

/* Initialize esc_getc() handler; string input */
static void init_hndl_eschr_string(hndl_eschr_t *p_hndl, const char *str,
    size_t max_num, sp_parser_token_t tkn, eol_t eol_typ, int quot_allowed)
{
    p_hndl->input.is_str = 1;
    p_hndl->input.str.buf = str;
    p_hndl->input.str.max_num = max_num;
    p_hndl->input.str.idx = 0;
    unc_clean(&p_hndl->input.unc);

    p_hndl->tkn = tkn;
    p_hndl->eol_typ = eol_typ;
    p_hndl->quot_chr = 0;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;

    if (quot_allowed && tkn==SP_TKN_ID && p_hndl->input.str.max_num) {
        int c = p_hndl->input.str.buf[p_hndl->input.str.idx++];
        if (c=='"' || c=='\'') {
            p_hndl->quot_chr = c;
            p_hndl->n_rdc++;
        } else {
            unc_ungetc(&p_hndl->input.unc, (!c ? EOF : c));
        }
    }
}

/* Get single char from hndl_eschr_t handle */
static int noesc_getc(hndl_eschr_t *p_hndl)
{
    int c;
    if (p_hndl->input.is_str)
    {
        int str_c;
        if (p_hndl->input.str.idx < p_hndl->input.str.max_num) {
            if (!(str_c=(int)p_hndl->input.str.buf[p_hndl->input.str.idx]))
                str_c=EOF;
        } else
            str_c=EOF;

        c = unc_getc(&p_hndl->input.unc,
            (str_c!=EOF ? (p_hndl->input.str.idx++, str_c) : str_c));
    } else {
        c = unc_getc(&p_hndl->input.unc, fgetc(p_hndl->input.f));
    }
    return c;
}

/* Get and escape (token dependent) single char from hndl_eschr_t handle */
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

        /* single/double quotation mark (escaped inside quotations only) */
        case '"':
        case '\'':
            if (!p_hndl->quot_chr) p_hndl->escaped=0;
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

        /* SP_TKN_VAL: line continuation */
        case '\n':
        case '\r':
            if (p_hndl->tkn!=SP_TKN_VAL)
                p_hndl->escaped=0;
            else
            switch (p_hndl->eol_typ)
            {
            case EOL_LF:
                if (c=='\n') {
                    __GETC();
                } else
                    p_hndl->escaped=0;
                break;
            case EOL_CRLF:
                if (c=='\r' && __GETC()=='\n') {
                    __GETC();
                } else
                    p_hndl->escaped=0;
                break;
            case EOL_CR:
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
                unc_ungetc(&p_hndl->input.unc, esc[n_esc]);
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

/* Get, escape single char from hndl_eschr_t handle, if quotation char occurs
   it's re-read
 */
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
sp_errc_t sp_parser_tkn_cpy(
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn,
    const sp_loc_t *p_loc, char *buf, size_t buf_len, long *p_tklen)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    int c=0;
    size_t i=0;
    hndl_eschr_t eh;
    long llen=sp_loc_len(p_loc);

    if (p_tklen) *p_tklen=0;
    if (!llen || (!buf_len && !p_tklen)) {
        ret=SPEC_SUCCESS;
        goto finish;
    }

    if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;

    init_hndl_eschr_stream(&eh, p_phndl, tkn, 1);

    while (eh.n_rdc<(size_t)llen && (buf_len || p_tklen))
    {
        if ((c=esc_getc(&eh))==EOF || eh.n_rdc>(size_t)llen)
            break;

        if (eh.quot_chr && c==eh.quot_chr && !eh.escaped) {
            eh.quot_chr=0;
        } else {
            if (p_tklen) (*p_tklen)++;
            if (buf_len) {
                buf[i++] = (char)c;
                buf_len--;
            }
        }
    }
    if (c!=EOF || eh.n_rdc>=(size_t)llen) ret=SPEC_SUCCESS;

finish:
    if (buf_len) buf[i]=0;
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_parser_tkn_cmp(
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn,
    const sp_loc_t *p_loc, const char *str, size_t max_num, int *p_equ)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    int c_tkn, c_str;
    hndl_eschr_t eh_tkn, eh_str;
    long llen=sp_loc_len(p_loc);

#define __CHK_STREAM() \
    if (eh_tkn.n_rdc<=(size_t)llen) { if (c_tkn==EOF) goto finish; } \
    else c_tkn=EOF;

    if (!max_num && !llen) {
        ret=SPEC_SUCCESS;
        if (p_equ) *p_equ=1;
        goto finish;
    }

    if (llen) {
        if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;
        init_hndl_eschr_stream(&eh_tkn, p_phndl, tkn, 1);
        c_tkn = esc_reqout_getc(&eh_tkn);
        __CHK_STREAM();
    } else
        c_tkn=EOF;

    if (max_num) {
        init_hndl_eschr_string(&eh_str, str, max_num, tkn, EOL_UNDEF, 0);
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

    ret=SPEC_SUCCESS;
    if (p_equ) *p_equ=(c_tkn==c_str ? 1 : 0);

finish:
    return ret;

#undef __CHK_STREAM
}

/* exported; see header for details */
sp_errc_t sp_parser_tokenize_str(
    FILE *out, sp_parser_token_t tkn, const char *str)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    int quot_chr=0, c;
    const char *in;

#define __CHK_FERR(c) if ((c)<0) goto finish;

    if (!out || !str) { ret=SPEC_INV_ARG; goto finish; }
    if (!(strlen(str))) { ret=SPEC_SUCCESS; goto finish; }

    in = str;
    if (tkn==SP_TKN_ID)
    {
        /* check if quotation is needed */
        while ((c=*in++)!=0) {
            if (!is_nq_id(c)) {
                quot_chr='"';
                if (strchr(str, '"') && !strchr(str, '\'')) quot_chr='\'';
                break;
            }
        }

        in = str;
        if (quot_chr) {
            __CHK_FERR(fputc(quot_chr, out));
        } else {
            /* if the first char is a quotation mark - escape it */
            if (*str=='"' || *str=='\'') {
                __CHK_FERR(fprintf(out, "\\x%02x", (int)*in++));
            }
        }
    }

    while ((c=*in++)!=0)
    {
        if (!isprint(c) || c=='\\' || c==quot_chr
#ifndef NO_SEMICOL_ENDS_VAL
            || (tkn==SP_TKN_VAL && c==';')
#endif
#ifdef CUT_VAL_LEADING_SPACES
            /* space char need to be escaped if it's the first
               char in SP_TKN_VAL token to avoid cut leading spaces */
            || (tkn==SP_TKN_VAL && isspace(c) && (in-1)==str)
#endif
#ifdef TRIM_VAL_TRAILING_SPACES
            /* space char need to be escaped if it's the last
               char in SP_TKN_VAL token to avoid trimming */
            || (tkn==SP_TKN_VAL && isspace(c) && !*in)
#endif
           )
        {
            /* char need to escaped */
            __CHK_FERR(fputc('\\', out));

            switch (c)
            {
            case '\a':
                __CHK_FERR(fputc('a', out));
                break;
            case '\b':
                __CHK_FERR(fputc('b', out));
                break;
            case '\f':
                __CHK_FERR(fputc('f', out));
                break;
            case '\n':
                __CHK_FERR(fputc('n', out));
                break;
            case '\r':
                __CHK_FERR(fputc('r', out));
                break;
            case '\t':
                __CHK_FERR(fputc('t', out));
                break;
            case '\v':
                __CHK_FERR(fputc('v', out));
                break;
            case '\\':
                __CHK_FERR(fputc('\\', out));
                break;
            default:
                if (c==quot_chr) {
                    __CHK_FERR(fputc(c, out));
                } else {
                    __CHK_FERR(fprintf(out, "x%02x", c));
                }
                break;
            }
        } else {
            __CHK_FERR(fputc(c, out));
        }
    }

    if (quot_chr) {
        __CHK_FERR(fputc(quot_chr, out));
    }
    ret=SPEC_SUCCESS;

finish:
    return ret;

#undef __CHK_FERR
}
