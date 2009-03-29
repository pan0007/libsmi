/*
 * yang.c --
 *
 *      Interface Implementation of YANG.
 *
 * Copyright (c) 1999 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Author: Kaloyan Kanev, Siarhei Kuryla
 * @(#) $Id: data.c 12198 2009-01-05 08:37:07Z schoenw $
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <limits.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "yang.h"
#include "error.h"
#include "util.h"
#include "snprintf.h"
