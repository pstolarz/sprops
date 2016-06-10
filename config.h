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

/* Maximum nesting level of scopes. If 0 - only global scope is allowed, if
   undefined - no restriction provided.
 */
#undef CONFIG_MAX_SCOPE_LEVEL_DEPTH

#endif  /* __CONFIG_H__ */
