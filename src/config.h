/*
   Copyright (c) 2016,2021,2022 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SP_CONFIG_H__
#define __SP_CONFIG_H__

/* If the boolean parameter is configured: forbid a semicolon (';') as the
   property ending marker. End-of-line is the only allowed marker. A property
   value may contain a semicolon in its content, which not need to be escaped.
   If not configured: End-of-line and a semicolon mark the property ending.
   If a property value needs to contain a semicolon, it must be escaped.
 */
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
# define CONFIG_NO_SEMICOL_ENDS_VAL 0
#endif

/* If the boolean parameter is configured: cut leading spaces in a property
   value.
 */
#ifndef CONFIG_CUT_VAL_LEADING_SPACES
# define CONFIG_CUT_VAL_LEADING_SPACES 1
#endif

/* If the boolean parameter is configured: trim trailing spaces in a property
   value.
 */
#ifndef CONFIG_TRIM_VAL_TRAILING_SPACES
# define CONFIG_TRIM_VAL_TRAILING_SPACES 1
#endif

/* Maximum nesting level of scopes. If 0 - only the global scope is allowed,
   if < 0 - no restriction provided.
 */
#ifndef CONFIG_MAX_SCOPE_LEVEL_DEPTH
# define CONFIG_MAX_SCOPE_LEVEL_DEPTH (-1)
#endif

/* If the boolean parameter is configured: forbid alternative version of empty
   scope with a type:

   type scope {}    <- is valid
   type scope;      <- is not valid

   If not configured, both versions are valid.

   NOTE: The alternative version may be misleading by its peculiar syntax.
   Additionally it's possible for scopes with a type only, otherwise would lead
   to ambiguity with empty property syntax. For these reasons it's recommended
   to disable the alternative syntax.
 */
#ifndef CONFIG_NO_EMPTY_SCOPE_ALT
# define CONFIG_NO_EMPTY_SCOPE_ALT 1
#endif

/* Due to indentation issues observed (under some circumstances) for transactions
   with not a NULL parsing scopes and consisting of more than one modification,
   this option specifies a type of the parsing scope initial modification to
   avoid the problem. The following PARSC_EXTIND value shall be sufficient for
   most use cases, guaranteeing no additional performance overhead.
   See trans.h header for more values for this configuration parameter.
   If not specified, not modified parsing scope is used.
 */
#ifndef CONFIG_TRANS_PARSC_MOD
# define CONFIG_TRANS_PARSC_MOD PARSC_EXTIND
#endif

/* If a parameter is defined w/o value assigned, it is assumed as configured.
 */
#define __XEXT1(__prm) (1##__prm)
#define __EXT1(__prm) __XEXT1(__prm)

#ifdef CONFIG_NO_SEMICOL_ENDS_VAL
# if (__EXT1(CONFIG_NO_SEMICOL_ENDS_VAL) == 1)
#  undef CONFIG_NO_SEMICOL_ENDS_VAL
#  define CONFIG_NO_SEMICOL_ENDS_VAL 1
# endif
#endif

#ifdef CONFIG_CUT_VAL_LEADING_SPACES
# if (__EXT1(CONFIG_CUT_VAL_LEADING_SPACES) == 1)
#  undef CONFIG_CUT_VAL_LEADING_SPACES
#  define CONFIG_CUT_VAL_LEADING_SPACES 1
# endif
#endif

#ifdef CONFIG_TRIM_VAL_TRAILING_SPACES
# if (__EXT1(CONFIG_TRIM_VAL_TRAILING_SPACES) == 1)
#  undef CONFIG_TRIM_VAL_TRAILING_SPACES
#  define CONFIG_TRIM_VAL_TRAILING_SPACES 1
# endif
#endif

#ifdef CONFIG_NO_EMPTY_SCOPE_ALT
# if (__EXT1(CONFIG_NO_EMPTY_SCOPE_ALT) == 1)
#  undef CONFIG_NO_EMPTY_SCOPE_ALT
#  define CONFIG_NO_EMPTY_SCOPE_ALT 1
# endif
#endif

#undef __EXT1
#undef __XEXT1

#endif  /* __SP_CONFIG_H__ */
