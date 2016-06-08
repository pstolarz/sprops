#ifndef __CONFIG_H__
#define __CONFIG_H__

/* If defined: forbid a semicolon (';') as the property ending marker.
   End-of-line is the only allowed marker. A property value may contain a
   semicolon in its content, which not need to be escaped.
   If not defined: End-of-line and a semicolon mark the property ending. If a
   property value needs to contain a semicolon, it must be escaped.
 */
#undef NO_SEMICOL_ENDS_VAL

/* If defined: cut leading spaces in a property value.
 */
#define CUT_VAL_LEADING_SPACES

/* If defined: trim trailing spaces in a property value.
 */
#define TRIM_VAL_TRAILING_SPACES

#endif  /* __CONFIG_H__ */
