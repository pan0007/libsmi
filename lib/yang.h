/*
 * yang.h --
 *
 *      Interface Definition of YANG (version 2:27:0).
 *
 * Copyright (c) 1999,2000 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Author: Kaloyan Kanev, Siarhei Kuryla
 * @(#) $Id: data.c 12198 2009-01-05 08:37:07Z schoenw $
 */

#ifndef _YANG_H
#define _YANG_H

#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_LIMITS_H
#include "limits.h"
#endif
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif


/* YangDecl -- type or statement that leads to a definition                   */
typedef enum YangDecl {
    YANG_DECL_UNKNOWN                   = 0,  /* should not occur                    */
    YANG_DECL_MODULE                    = 1,
    YANG_DECL_SUBMODULE                 = 2,
    YANG_DECL_REVISION                  = 3,
    YANG_DECL_IMPORT                    = 4,
    YANG_DECL_RANGE                     = 5,
    YANG_DECL_PATTERN                   = 6,
    YANG_DECL_CONTAINER                 = 7,
    YANG_DECL_MUST_STATEMENT            = 8,
    YANG_DECL_LEAF                      = 9,
    YANG_DECL_LEAF_LIST                 = 10,
    YANG_DECL_LIST                      = 11,
    YANG_DECL_CASE                      = 12,
    YANG_DECL_USES                      = 13,
    YANG_DECL_AUGMENT                   = 14,
    YANG_DECL_GROUPING                  = 15,
    YANG_DECL_CHOICE                    = 16,
    YANG_DECL_ARGUMENT                  = 17,
    YANG_DECL_RPC                       = 18,
    YANG_DECL_INPUT                     = 19,
    YANG_DECL_OUTPUT                    = 20,
    YANG_DECL_ANYXML                    = 21,
    YANG_DECL_INCLUDE                   = 22,
    YANG_DECL_ORGANIZATION              = 23,                /* organization */
    YANG_DECL_CONTACT                   = 24,                /* contact */
    YANG_DECL_NAMESPACE                 = 25,                /* namespace */
    YANG_DECL_YANGVERSION               = 26,                /* yang-version */
    YANG_DECL_PREFIX                    = 27,                /* prefix */
    YANG_DECL_TYPEDEF                   = 28,                /* typedef */
    YANG_DECL_PATH                      = 29,                /* path */
    YANG_DECL_LENGTH                    = 30,                /* length */
    YANG_DECL_TYPE                      = 31,                /* type */
    YANG_DECL_ERROR_MESSAGE             = 32,                /* error-message */
    YANG_DECL_ERROR_APP_TAG             = 33,                /* error-app-tag */
    YANG_DECL_MANDATORY                 = 34,                /* mandatory */
    YANG_DECL_NOTIFICATION              = 35,                /* notification */
    YANG_DECL_EXTENSION                 = 36                /* extention */
    
} YangDecl;


extern const char* yandDeclKeyword[];

typedef char    *YangString;

/* the truth value of Yang config statement              */
typedef enum YangConfig {
     YANG_CONFIG_DEFAULT_FALSE          = 0, 
     YANG_CONFIG_DEFAULT_TRUE           = 1,    
     YANG_CONFIG_FALSE                  = 2, 
     YANG_CONFIG_TRUE                   = 3
} YangConfig;

typedef enum YangStatus {
    YANG_STATUS_DEFAULT_CURRENT  = 0,
    YANG_STATUS_CURRENT          = 1,
    YANG_STATUS_DEPRECATED       = 2,
    YANG_STATUS_OBSOLETE         = 5  /* for compatibility with SMI */
} YangStatus;


typedef struct YangNode {
    YangString      value;
    YangDecl		nodeKind;
    YangStatus		status;
    YangConfig		config;
    char		*description;
    char		*reference;       
} YangNode;

int yangIsTrueConf(YangConfig conf);

#ifdef __cplusplus
}
#endif


#endif /* _YANG_H */
