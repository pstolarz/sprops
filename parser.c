/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* "%code top" blocks.  */
#line 14 "parser.y" /* yacc.c:316  */

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "sprops/parser.h"

/* EOL char code (platform independent) */
#define EOL (EOF-1)
#define is_space(c) (isspace(c) || (c)==EOL)

/* NOTE: semicolon char is reserved even though CONFIG_NO_SEMICOL_ENDS_VAL
   is defined, due to property w/o a value and scope w/o a body alternative
   grammar rules. */
#define RESERVED_CHRS   "={};#"

#define is_nq_idc(c) (!is_space(c) && !strchr(RESERVED_CHRS, (c)))

#define unc_clean(unc) ((unc)->inbuf=0)
#define unc_getc(unc, def) ((unc)->inbuf ? (unc)->buf[--((unc)->inbuf)] : (def))
#define unc_ungetc(unc, c) ((unc)->buf[(unc)->inbuf++]=(c))

/* lexical contexts */
#define LCTX_GLOBAL     0
#define LCTX_VAL        1

/* lexical value type */
typedef struct _lexval_t {
    long beg;
    long end;   /* inclusive */
    int scope_lev;
} lexval_t;

#define YYSTYPE lexval_t

#define YYTOKENTYPE sp_parser_token_t
typedef sp_parser_token_t yytokentype;


#line 103 "parser.c" /* yacc.c:316  */



/* Copy the first part of user declarations.  */

#line 109 "parser.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SP_TKN_ID = 258,
    SP_TKN_VAL = 259
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (sp_parser_hndl_t *p_hndl);
/* "%code provides" blocks.  */
#line 55 "parser.y" /* yacc.c:355  */

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

#define __CALL_CB_SCOPE(typ, nm, bdy, bdyenc, def) { \
    long pos = ftell(p_hndl->in); \
    sp_errc_t res = \
        p_hndl->cb.scope(p_hndl, (typ), (nm), (bdy), (bdyenc), (def)); \
    if (res==SPEC_SUCCESS && \
        (pos==-1L || fseek(p_hndl->in, pos, SEEK_SET))) res=SPEC_ACCS_ERR; \
    if ((int)res>0) { p_hndl->err.code=res; YYABORT; } \
    else if ((int)res<0) { YYACCEPT; } \
}

#define __IS_EMPTY(loc) ((loc).beg>(loc).end)
#define __PREP_LOC_PTR(loc) (__IS_EMPTY(loc) ? (sp_loc_t*)NULL : &(loc))


#line 212 "parser.c" /* yacc.c:355  */



/* Copy the second part of user declarations.  */

#line 218 "parser.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   14

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  9
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  4
/* YYNRULES -- Number of rules.  */
#define YYNRULES  11
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  19

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   259

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     6,
       2,     5,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     7,     2,     8,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,   107,   107,   113,   117,   118,   130,   158,   173,   187,
     212,   241
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SP_TKN_ID", "SP_TKN_VAL", "'='", "';'",
  "'{'", "'}'", "$accept", "input", "scoped_props", "prop_scope", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,    61,    59,   123,   125
};
# endif

#define YYPACT_NINF -8

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-8)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       5,    -3,     9,     5,    -8,     0,     6,    -8,     5,    -8,
      -8,    -8,     5,     7,     3,     4,    -8,    -8,    -8
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     3,     4,     0,     0,     8,     2,     1,
       5,    11,     2,     6,     0,     0,     7,     9,    10
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
      -8,    -7,    -8,    11
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,     4
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
       5,    14,     6,     7,     8,    15,    11,    12,     1,     9,
      13,    17,    18,    16,    10
};

static const yytype_uint8 yycheck[] =
{
       3,     8,     5,     6,     7,    12,     6,     7,     3,     0,
       4,     8,     8,     6,     3
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,    10,    11,    12,     3,     5,     6,     7,     0,
      12,     6,     7,     4,    10,    10,     6,     8,     8
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,     9,    10,    10,    11,    11,    12,    12,    12,    12,
      12,    12
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     2,     3,     4,     2,     4,
       5,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (&yylloc, p_hndl, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, p_hndl); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, sp_parser_hndl_t *p_hndl)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (p_hndl);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, sp_parser_hndl_t *p_hndl)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, p_hndl);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, sp_parser_hndl_t *p_hndl)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , p_hndl);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, p_hndl); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, sp_parser_hndl_t *p_hndl)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (p_hndl);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (sp_parser_hndl_t *p_hndl)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, p_hndl);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 107 "parser.y" /* yacc.c:1646  */
    {
        /* set to empty scope */
        (yyval).end = 0;
        (yyval).beg = (yyval).end+1;
        (yyval).scope_lev = 0;
    }
#line 1399 "parser.c" /* yacc.c:1646  */
    break;

  case 5:
#line 119 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-1]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-1]).scope_lev;
    }
#line 1409 "parser.c" /* yacc.c:1646  */
    break;

  case 6:
#line 131 "parser.y" /* yacc.c:1646  */
    {
        sp_loc_t lval;
        set_loc(&lval, &(yyvsp[0]), &(yylsp[0]));

        (yyval).beg = (yyvsp[-2]).beg;
        (yyval).scope_lev = (yyvsp[-2]).scope_lev;
        if (__IS_EMPTY(lval)) {
            (yyval).end = (yyvsp[-1]).end;
            (yyloc).last_line = (yylsp[-1]).last_line;
            (yyloc).last_column = (yylsp[-1]).last_column;
        } else {
            (yyval).end = (yyvsp[0]).end;
            (yyloc).last_line = (yylsp[0]).last_line;
            (yyloc).last_column = (yylsp[0]).last_column;
        }

        if (p_hndl->cb.prop && !(yyval).scope_lev) {
            sp_loc_t lname, ldef;
            set_loc(&lname, &(yyvsp[-2]), &(yylsp[-2]));
            set_loc(&ldef, &(yyval), &(yyloc));
            __CALL_CB_PROP(&lname, __PREP_LOC_PTR(lval), &ldef);
        }
    }
#line 1437 "parser.c" /* yacc.c:1646  */
    break;

  case 7:
#line 159 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-3]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-3]).scope_lev;

        if (p_hndl->cb.prop && !(yyval).scope_lev) {
            sp_loc_t lname, lval, ldef;
            set_loc(&lname, &(yyvsp[-3]), &(yylsp[-3]));
            set_loc(&lval, &(yyvsp[-1]), &(yylsp[-1]));
            set_loc(&ldef, &(yyval), &(yyloc));
            __CALL_CB_PROP(&lname, __PREP_LOC_PTR(lval), &ldef);
        }
    }
#line 1455 "parser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 174 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-1]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-1]).scope_lev;

        if (p_hndl->cb.prop && !(yyval).scope_lev) {
            sp_loc_t lname, ldef;
            set_loc(&lname, &(yyvsp[-1]), &(yylsp[-1]));
            set_loc(&ldef, &(yyval), &(yyloc));
            __CALL_CB_PROP(&lname, (sp_loc_t*)NULL, &ldef);
        }
    }
#line 1472 "parser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 188 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-3]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-3]).scope_lev;

        if (p_hndl->cb.scope && !(yyval).scope_lev)
        {
            sp_loc_t lname, lbody, lbdyenc, ldef;

            set_loc(&lname, &(yyvsp[-3]), &(yylsp[-3]));
            set_loc(&lbody, &(yyvsp[-1]), &(yylsp[-1]));
            lbdyenc.beg = (yyvsp[-2]).beg;
            lbdyenc.end = (yyvsp[0]).end;
            lbdyenc.first_line = (yylsp[-2]).first_line;
            lbdyenc.first_column = (yylsp[-2]).first_column;
            lbdyenc.last_line = (yylsp[0]).last_line;
            lbdyenc.last_column = (yylsp[0]).last_column;
            set_loc(&ldef, &(yyval), &(yyloc));

            __CALL_CB_SCOPE(
                (sp_loc_t*)NULL, &lname, __PREP_LOC_PTR(lbody), &lbdyenc, &ldef);
        }
    }
#line 1500 "parser.c" /* yacc.c:1646  */
    break;

  case 10:
#line 213 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-4]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-4]).scope_lev;

        if (p_hndl->cb.scope && !(yyval).scope_lev)
        {
            sp_loc_t ltype, lname, lbody, lbdyenc, ldef;

            set_loc(&ltype, &(yyvsp[-4]), &(yylsp[-4]));
            set_loc(&lname, &(yyvsp[-3]), &(yylsp[-3]));
            set_loc(&lbody, &(yyvsp[-1]), &(yylsp[-1]));
            lbdyenc.beg = (yyvsp[-2]).beg;
            lbdyenc.end = (yyvsp[0]).end;
            lbdyenc.first_line = (yylsp[-2]).first_line;
            lbdyenc.first_column = (yylsp[-2]).first_column;
            lbdyenc.last_line = (yylsp[0]).last_line;
            lbdyenc.last_column = (yylsp[0]).last_column;
            set_loc(&ldef, &(yyval), &(yyloc));

            __CALL_CB_SCOPE(
                &ltype, &lname, __PREP_LOC_PTR(lbody), &lbdyenc, &ldef);
        }
    }
#line 1529 "parser.c" /* yacc.c:1646  */
    break;

  case 11:
#line 242 "parser.y" /* yacc.c:1646  */
    {
        (yyval).beg = (yyvsp[-2]).beg;
        (yyval).end = (yyvsp[0]).end;
        (yyval).scope_lev = (yyvsp[-2]).scope_lev;

        if (p_hndl->cb.scope && !(yyval).scope_lev)
        {
            sp_loc_t ltype, lname, lbdyenc, ldef;

            set_loc(&ltype, &(yyvsp[-2]), &(yylsp[-2]));
            set_loc(&lname, &(yyvsp[-1]), &(yylsp[-1]));
            set_loc(&lbdyenc, &(yyvsp[0]), &(yylsp[0]));
            set_loc(&ldef, &(yyval), &(yyloc));

            __CALL_CB_SCOPE(
                &ltype, &lname, (sp_loc_t*)NULL, &lbdyenc, &ldef);
        }
    }
#line 1552 "parser.c" /* yacc.c:1646  */
    break;


#line 1556 "parser.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, p_hndl, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, p_hndl, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, p_hndl);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, p_hndl);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, p_hndl, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, p_hndl);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, p_hndl);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 262 "parser.y" /* yacc.c:1906  */


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
            if (c=='\r') {
                if ((c=unc_getc(&p_hndl->lex.unc, fgetc(p_hndl->in)))=='\n') {
                    p_hndl->lex.off++;
                } else {
                    unc_ungetc(&p_hndl->lex.unc, c);
                }
            }
            c=EOL;
        }
    } else
        c=EOF;
    return c;
}

/* Lexical scanner (lexer)
 */
static int yylex(YYSTYPE *p_lval, YYLTYPE *p_lloc, sp_parser_hndl_t *p_hndl)
{
    /* lexer SM states */
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
           (if CONFIG_NO_SEMICOL_ENDS_VAL is not defined); line continuation
           allowed */
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

#define __USE_ESC() \
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
            if (is_nq_idc(c))
            {
                __USE_ESC();
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
          {
            __USE_ESC();
            if (!is_nq_idc(c) && !esc) {
                __MCHAR_TOKEN_END();
                endloop=1;
                unc_ungetc(&p_hndl->lex.unc, c);
                continue;
            } else {
                if (c==EOL) {
                    /* error: line continuation is not possible for SP_TKN_ID */
                    __MCHAR_TOKEN_END();
                    endloop=1;
                    token = YYERRCODE;
                } else {
                    __MCHAR_UPDATE_TAIL();
                }
            }
            break;
          }

        case LXST_ID_QUOTED:
          {
            __USE_ESC();
            if (c==EOL) {
                /* error: quoted id need to be finished by the quotation mark
                   NOTE: line continuation is not possible for SP_TKN_ID */
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
            __USE_ESC();
#ifdef CONFIG_NO_SEMICOL_ENDS_VAL
            if (c==EOL && !esc)
#else
            if ((c==EOL || c==';') && !esc)
#endif
            {
                /* mark token as empty */
                __CHAR_TOKEN(SP_TKN_VAL);
                p_lval->beg++;
                endloop=1;
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
                if (c==';') {
                    unc_ungetc(&p_hndl->lex.unc, c);
                    continue;
                }
#endif
            } else
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
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
            __USE_ESC();
#ifdef CONFIG_NO_SEMICOL_ENDS_VAL
            if (c==EOL && !esc)
#else
            if ((c==EOL || c==';') && !esc)
#endif
            {
                __MCHAR_TOKEN_END();
                endloop=1;
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
                if (c==';') {
                    unc_ungetc(&p_hndl->lex.unc, c);
                    continue;
                }
#endif
            } else
#ifdef CONFIG_TRIM_VAL_TRAILING_SPACES
            if (!isspace(c))
#endif
            {
                __MCHAR_UPDATE_TAIL();
            }
            break;
          }
        }   /* switch (state) */

        /* track location of the next char to read */
        if (c==EOL) {
            p_hndl->lex.line++;
            p_hndl->lex.col=1;
        } else {
            p_hndl->lex.col++;
        }
        p_hndl->lex.off++;
    }   /* read loop */

    if (c==EOF)
    {
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
    if (token=='{') {
        p_hndl->lex.scope_lev++;
#ifdef CONFIG_MAX_SCOPE_LEVEL_DEPTH
        if (p_hndl->lex.scope_lev > CONFIG_MAX_SCOPE_LEVEL_DEPTH) {
            token = YYERRCODE;
        }
#endif
    } else
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

#undef __USE_ESC
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
    sp_errc_t ret=SPEC_SUCCESS;
    sp_loc_t globsc = {0, -1L, 1, 1, -1, -1};
    if (!p_parsc) p_parsc=&globsc;

    if (!p_hndl || !in) { ret=SPEC_INV_ARG; goto finish; }

    if (fseek(in, p_parsc->beg, SEEK_SET)) { ret=SPEC_ACCS_ERR; goto finish; }

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
            /* stream; is_str==0 */
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
    int quot_chr;           /* quotation char (SP_TKN_ID token) */

    /* set by esc_getc() after each call */
    size_t n_rdc;       /* number of read chars so far */
    int escaped;        /* the returned char is escaped */
} hndl_eschr_t;

/* Initialize esc_getc() handler; stream input */
static void init_hndl_eschr_stream(hndl_eschr_t *p_hndl,
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn)
{
    p_hndl->input.is_str = 0;
    p_hndl->input.f = p_phndl->in;
    unc_clean(&p_hndl->input.unc);

    p_hndl->tkn = tkn;
    p_hndl->quot_chr = -1;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;

    if (tkn==SP_TKN_ID) {
        int c=fgetc(p_phndl->in);
        if (c=='"' || c=='\'') {
            p_hndl->quot_chr = c;
            p_hndl->n_rdc++;
        } else {
            unc_ungetc(&p_hndl->input.unc, c);
        }
    }
}

/* Initialize esc_getc() handler; string input */
static void init_hndl_eschr_string(hndl_eschr_t *p_hndl, const char *str,
    size_t max_num, sp_parser_token_t tkn)
{
    p_hndl->input.is_str = 1;
    p_hndl->input.str.buf = str;
    p_hndl->input.str.max_num = max_num;
    p_hndl->input.str.idx = 0;
    unc_clean(&p_hndl->input.unc);

    p_hndl->tkn = tkn;
    p_hndl->quot_chr = -1;

    p_hndl->n_rdc = 0;
    p_hndl->escaped = 0;
}

/* Get single char from hndl_eschr_t handle */
static int noesc_getc(hndl_eschr_t *p_hndl)
{
    int c;
    if (p_hndl->input.is_str)
    {
        int str_c;
        if (p_hndl->input.str.idx < p_hndl->input.str.max_num)
        {
            str_c = (int)p_hndl->input.str.buf[p_hndl->input.str.idx] & 0xff;
            if (!str_c) str_c=EOF;
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

#define __GETC(c)    ((c)=noesc_getc(p_hndl))
#define __UNGETC(c)  unc_ungetc(&p_hndl->input.unc, (c))

#define __HEXCHR2BT(chr, out) \
    ((((out)=(chr)-'0')>=0 && (out)<=9) ? 1 : \
    ((((out)=(chr)-'A')>=0 && (out)<=5) ? ((out)+=10, 1) : \
    ((((out)=(chr)-'a')>=0 && (out)<=5) ? ((out)+=10, 1) : 0)))

    p_hndl->escaped=0;
    if (__GETC(c)=='\\')
    {
        n_esc++;
        p_hndl->escaped = 1;

        switch (__GETC(c))
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
            int h1, h2, c1, c2;

            __GETC(c1);
            if (__HEXCHR2BT(c1, h1)) {
                __GETC(c2);
                if (__HEXCHR2BT(c2, h2)) {
                    n_esc+=2;
                    c=(h1<<4)|h2;
                } else {
                    __UNGETC(c2);
                    __UNGETC(c1);
                }
            } else {
                __UNGETC(c1);
            }
            break;
          }

        /* line continuation (SP_TKN_VAL only) */
        case '\n':
        case '\r':
            if (p_hndl->tkn==SP_TKN_VAL) {
                int c1=c;

                n_esc++; __GETC(c);
                if (c1=='\r' && c=='\n') { n_esc++; __GETC(c); }
            } else {
                /* take the following char literally */
            }
            break;

        default:
            /* take the following char literally */
            if (c==EOF) { __UNGETC(c); c='\\'; p_hndl->escaped=0; }
            break;
        }
    }

    if (c!=EOF) p_hndl->n_rdc++;
    if (p_hndl->escaped) p_hndl->n_rdc+=n_esc;

    return c;

#undef __HEXCHR2BT
#undef __UNGETC
#undef __GETC
}

/* Get and escape single char from hndl_eschr_t handle. If quotation char occurs
   it's re-read.
 */
static int esc_reqout_getc(hndl_eschr_t *p_hndl)
{
    int c;
reread:
    c = esc_getc(p_hndl);
    if (p_hndl->quot_chr>=0 && c==p_hndl->quot_chr && !p_hndl->escaped) {
        p_hndl->quot_chr = -1;
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
    hndl_eschr_t eh_tkn;
    long llen=sp_loc_len(p_loc);

    if (p_tklen) *p_tklen=0;
    if (!llen || (!buf_len && !p_tklen)) {
        ret=SPEC_SUCCESS;
        goto finish;
    }

    if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;

    init_hndl_eschr_stream(&eh_tkn, p_phndl, tkn);

    while (eh_tkn.n_rdc<(size_t)llen && (buf_len || p_tklen))
    {
        if ((c=esc_getc(&eh_tkn))==EOF || eh_tkn.n_rdc>(size_t)llen)
            break;

        if (eh_tkn.quot_chr>=0 && c==eh_tkn.quot_chr && !eh_tkn.escaped) {
            eh_tkn.quot_chr = -1;
        } else {
            if (p_tklen) (*p_tklen)++;
            if (buf_len) {
                buf[i++] = (char)c;
                buf_len--;
            }
        }
    }
    if (c!=EOF || eh_tkn.n_rdc>=(size_t)llen) ret=SPEC_SUCCESS;

finish:
    if (buf_len) buf[i]=0;
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_parser_tkn_cmp(const sp_parser_hndl_t *p_phndl,
    sp_parser_token_t tkn, const sp_loc_t *p_loc, const char *str,
    size_t max_num, int stresc, int *p_equ)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    int c_tkn, c_str;
    hndl_eschr_t eh_tkn, eh_str;
    long llen=sp_loc_len(p_loc);

#define __CHK_STREAM() \
    if (eh_tkn.n_rdc<=(size_t)llen) { if (c_tkn==EOF) goto finish; } \
    else c_tkn=EOF;
#define __EH_STR_GETC() \
    (stresc ? esc_getc(&eh_str) : noesc_getc(&eh_str))

    if (!max_num && !llen) {
        ret=SPEC_SUCCESS;
        if (p_equ) *p_equ=1;
        goto finish;
    }

    if (llen) {
        if (fseek(p_phndl->in, p_loc->beg, SEEK_SET)) goto finish;
        init_hndl_eschr_stream(&eh_tkn, p_phndl, tkn);
        c_tkn = esc_reqout_getc(&eh_tkn);
        __CHK_STREAM();
    } else
        c_tkn=EOF;

    if (max_num) {
        init_hndl_eschr_string(&eh_str, str, max_num, tkn);
        c_str = __EH_STR_GETC();
    } else
        c_str=EOF;

    do {
        if (c_tkn!=c_str) break;

        c_tkn = esc_reqout_getc(&eh_tkn);
        __CHK_STREAM();
        c_str = __EH_STR_GETC();
    } while (c_tkn!=EOF && c_str!=EOF);

    ret=SPEC_SUCCESS;
    if (p_equ) *p_equ=(c_tkn==c_str ? 1 : 0);

finish:
    return ret;

#undef __EH_STR_GETC
#undef __CHK_STREAM
}

/* exported; see header for details */
sp_errc_t sp_parser_tokenize_str(
    FILE *out, sp_parser_token_t tkn, const char *str, unsigned cv_flags)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    int quot_chr=-1, c;
    const char *in;

    char bf[16];
    size_t bfs=0, i, cvi;
    unsigned cvlen = SPAR_F_GET_CVLEN(cv_flags);
    sp_eol_t cveol = SPAR_F_GET_CVEOL(cv_flags);

#define __CHK_FERR(c) if ((c)==EOF) goto finish;
#define __CHK_FPRF(c) if ((c)<0) goto finish;

    if (!out || !str || (tkn==SP_TKN_VAL && cvlen>0 && cvlen<SPAR_MIN_CV_LEN))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    if (!(strlen(str))) {
        ret=SPEC_SUCCESS;
        goto finish;
    }

    if (tkn==SP_TKN_VAL)
    {
        bf[bfs++] = '\\';

        switch (cveol)
        {
        case EOL_LF:
            bf[bfs++] = '\n';
            break;

        case EOL_CRLF:
            bf[bfs++] = '\r';
            bf[bfs++] = '\n';
            break;

        case EOL_CR:
            bf[bfs++] = '\r';
            break;

        /* compilation platform specific */
        default:
#if defined(_WIN32) || defined(_WIN64)
            bf[bfs++] = '\r';
#endif
            bf[bfs++] = '\n';
            break;
        }
    }

    in = str;
    if (tkn==SP_TKN_ID)
    {
        /* if a tokenized string contains chars not allowed in SP_TKN_ID then
           use quotation, therefore avoiding over-escaping of the token
           reserved chars */
        while ((c=(*in++ & 0xff))!=0) {
            if (!is_nq_idc(c)) {
                quot_chr='"';
                if (strchr(str, '"') && !strchr(str, '\'')) quot_chr='\'';
                break;
            }
        }

        in = str;
        if (quot_chr>=0) {
            __CHK_FERR(fputc(quot_chr, out));
        } else {
            /* if the first char is a quotation mark - escape it */
            if (*str=='"' || *str=='\'') {
                __CHK_FERR(fputc('\\', out));
                __CHK_FERR(fputc(*in++, out));
            }
        }
    }

    for (i=bfs, cvi=0; (c=(*in++ & 0xff))!=0;)
    {
        if (!isprint(c) || c=='\\' || c==quot_chr
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
            || (tkn==SP_TKN_VAL && c==';')
#endif
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
            /* space char need to be escaped if it's the first
               char in SP_TKN_VAL token to avoid cut leading spaces */
            || (tkn==SP_TKN_VAL && isspace(c) && (in-1)==str)
#endif
            /* space char need to be escaped if it's the last char in
               SP_TKN_VAL token to avoid unreadability (line continuation)
               and possible trimming (CONFIG_TRIM_VAL_TRAILING_SPACES)
             */
            || (tkn==SP_TKN_VAL && isspace(c) && !*in)
           )
        {
            /* char need to escaped */
            bf[i++] = '\\';

            switch (c)
            {
            case '\a':
                bf[i++] = 'a';
                break;
            case '\b':
                bf[i++] = 'b';
                break;
            case '\f':
                bf[i++] = 'f';
                break;
            case '\n':
                bf[i++] = 'n';
                break;
            case '\r':
                bf[i++] = 'r';
                break;
            case '\t':
                bf[i++] = 't';
                break;
            case '\v':
                bf[i++] = 'v';
                break;
            case '\\':
                bf[i++] = '\\';
                break;
            default:
                if (!isprint(c) || (isspace(c) && !*in)) {
                    sprintf(&bf[i], "x%02x", c);
                    i+=3;
                } else {
                    bf[i++] = c;
                }
                break;
            }
        } else {
            bf[i++] = c;
        }

        bf[i] = 0;

        /* check if a SP_TKN_VAL token cut is needed */
        if (tkn==SP_TKN_VAL && cvlen>0 && (cvi+i-bfs)>cvlen) {
            __CHK_FERR(fputs(&bf[0], out));
            cvi = 0;
        } else {
            __CHK_FERR(fputs(&bf[bfs], out));
        }

        cvi += i-bfs;
        i = bfs;
    }

    if (quot_chr>=0) {
        __CHK_FERR(fputc(quot_chr, out));
    }
    ret=SPEC_SUCCESS;

finish:
    return ret;

#undef __CHK_FPRF
#undef __CHK_FERR
}
