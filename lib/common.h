/*
 * common.h --
 *
 *      Common functionality.
 *
 * Copyright (c) 1999 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: data.h 7966 2008-03-27 21:25:52Z schoenw $
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "util.h"
#include "error.h"

char* getModulePath(const char *modulename);
SmiLanguage getLanguage(FILE *file);
SmiLanguage guessLanguage(const char *modulename);

#endif /* _COMMON_H */