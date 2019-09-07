/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

/* Content of this header is not a part of the library API interface.
   It rather defines internal use (private) I/O interface for stream handling.
   Public part of this interface is defined in the public props.h header.
 */

#ifndef __SP_IO_H__

#include "sprops/props.h"

/* fgetc(3) analogous */
int sp_fgetc(SP_FILE *f);

/* fputc(3) analogous */
int sp_fputc(int c, SP_FILE *f);

/* fputs(3) analogous */
int sp_fputs(const char *str, SP_FILE *f);

/* fseek(3) analogous */
int sp_fseek(SP_FILE *f, long int offset, int origin);

/* ftell(3) analogous */
long int sp_ftell(SP_FILE *f);

#endif  /* __SP_IO_H__ */
