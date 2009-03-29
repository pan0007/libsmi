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

const char* yandDeclKeyword[] = {   "unknown",
                                    "module",
                                    "submodule",
                                    "revision",
                                    "import",
                                    "range",
                                    "pattern",
                                    "container",
                                    "must",
                                    "leaf",
                                    "leaf-list",
                                    "list",
                                    "case",
                                    "uses",
                                    "augument",
                                    "grouping",
                                    "choice",
                                    "argument",
                                    "rpc",
                                    "input",
                                    "output",
                                    "anyxml",
                                    "include",
                                    "organization",
                                    "contact",
                                    "namespace",
                                    "yang-version",
                                    "prefix",
                                    "typedef",
                                    "path",
                                    "length",
                                    "type",
                                    "error-message",
                                    "error-app-tag",
                                    "mandatory",
                                    "notification",
                                    "extension"
                               };

int yangIsTrueConf(YangConfig conf) {
    return conf == YANG_CONFIG_DEFAULT_TRUE || conf == YANG_CONFIG_TRUE;
}