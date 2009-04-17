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
    int                 line;
    void                *info;
    struct _YangNode  	*firstChildPtr;
    struct _YangNode  	*lastChildPtr;    
    struct _YangNode  	*nextSiblingPtr;
    struct _YangNode  	*parentPtr;
    struct _YangNode  	*modulePtr;    
} _YangNode;

typedef struct _YangNodeList {
    struct _YangNode  	*nodePtr;
    struct _YangNodeList *next;
} _YangNodeList;

typedef struct _YangImportList {
    char                *prefix;
    struct _YangNode  	*modulePtr;
    struct _YangImportList *next;
} _YangImportList;

/* _YangParseState -- reflects the current state of the module processing state      */
typedef enum _YangParsingState {
    YANG_PARSING_IN_PROGRESS       = 0,  /* should not occur            */
    YANG_PARSING_DONE              = 1
} _YangParsingState;

typedef struct _YangModuleInfo {
    char		*prefix;
	char		*version;
    char		*namespace;
    char        *organization;
    char        *contact;
    _YangParsingState parsingState;
    int         conformance;
    struct _YangNodeList *submodules;
    struct _YangImportList *imports;
} _YangModuleInfo;

_YangNode *findYangModuleByName(const char *modulename);

_YangNode *addYangNode(const char *value, YangDecl nodeKind, _YangNode *parentPtr);

_YangModuleInfo *createModuleInfo(_YangNode *modulePtr);

_YangNode* findChildNodeByType(_YangNode *nodePtr, YangDecl nodeKind);

_YangNode* findChildNodeByTypeAndValue(_YangNode *nodePtr, YangDecl nodeKind, char* value);

_YangNode *externalModule(_YangNode *importNode);
/*
 * YangNode fields setters
 */
void setConfig(_YangNode *nodePtr, YangConfig config);
        
void setStatus(_YangNode *nodePtr, YangStatus status);

void setDescription(_YangNode *nodePtr, char *description);

void setReference(_YangNode *nodePtr, char *reference);

/*
 * Uniqueness checks
 */

void uniqueConfig(_YangNode *nodePtr);

void uniqueStatus(_YangNode *nodePtr);

void uniqueDescription(_YangNode *nodePtr);

void uniqueReference(_YangNode *nodePtr);

void uniqueNodeKind(_YangNode *nodePtr, YangDecl nodeKind);

void presenceNodeKind(_YangNode *nodePtr, YangDecl nodeKind);

int getCardinality(_YangNode *nodePtr, YangDecl nodeKind);
/*
 *  Free YANG datastructures
 */
void yangFreeData();

#endif /* _YANG_DATA_H */

