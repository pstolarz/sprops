#ifndef __CONFIG_H__
#define __CONFIG_H__

/* If defined: forbid a semicolon (';') as the property ending marker.
   End-of-line is the only allowed marker. A property value may contain a
   semicolon in its content, which not need to be escaped.
   If not defined: End-of-line and a semicolon mark the property ending. If a
   property value needs to contain a semicolon, it must be escaped.
 */
#undef CONFIG_NO_SEMICOL_ENDS_VAL

/* If defined: cut leading spaces in a property value.
 */
#define CONFIG_CUT_VAL_LEADING_SPACES

/* If defined: trim trailing spaces in a property value.
 */
#define CONFIG_TRIM_VAL_TRAILING_SPACES

/* Maximum nesting level of scopes. If 0 - only the global scope is allowed,
   if undefined - no restriction provided.
 */
#undef CONFIG_MAX_SCOPE_LEVEL_DEPTH

/* CONFIG_TRANS_PARSC_MOD variants */
#define PARSC_EXTIND    1
#define PARSC_AS_INPUT  2

/* Due to indentation issues observed for transactions on non-NULL parsing
   scope and consisting of more than one modification, this option specifies
   type of the parsing scope modification to avoid the problem.
   If not specified, not modified parsing scope is used.
 */
#define CONFIG_TRANS_PARSC_MOD  PARSC_EXTIND

#endif  /* __CONFIG_H__ */
