/*
 * yang-data.h --
 *
 *      Definitions for the main YANG data structures.
 *
 * Copyright (c) 1999 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Authors: Kaloyan Kanev, Siarhei Kuryla
 * @(#) $Id: data.h 7966 2008-03-27 21:25:52Z schoenw $
 */

#ifndef _YANG_DATA_H
#define _YANG_DATA_H


#include <stdio.h>
#include "yang.h"

typedef struct _YangNode {
    YangNode            export;
    void                *info;
    struct _YangNode  	*firstChildPtr;
    struct _YangNode  	*lastChildPtr;    
    struct _YangNode  	*nextSiblingPtr;
    struct _YangNode  	*parentPtr;
    struct _YangNode  	*modulePtr;
} _YangNode;

typedef struct _YangModuleInfo {
    char		*prefix;
	char		*version;
    char		*namespace;
    char        *organization;
    char        *contact;
} _YangModuleInfo;

_YangNode *yangFindModuleByName(const char *modulename);

_YangNode *addYangNode(char *value, YangDecl nodeKind, _YangNode *parentPtr);

_YangModuleInfo *createModuleInfo(_YangNode *modulePtr);

/*
 * YangNode fields setters
 */
void setConfig(_YangNode *nodePtr, YangConfig config);
        
void setStatus(_YangNode *nodePtr, YangStatus status);

void setDescription(_YangNode *nodePtr, char *description);

void setReference(_YangNode *nodePtr, char *reference);

#endif /* _YANG_DATA_H */

