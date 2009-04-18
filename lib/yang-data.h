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

extern const char* yangBuiltInTypeNames[];

typedef enum YangBuiltInTypes {
    YANG_TYPE_NONE                  = -1,
    YANG_TYPE_BINARY                = 0,
    YANG_TYPE_BITS                  = 1,
    YANG_TYPE_BOOLEAN               = 2,
    YANG_TYPE_EMPTY                 = 3,
    YANG_TYPE_ENUMERATION           = 4,
    YANG_TYPE_FLOAT32               = 5,
    YANG_TYPE_FLOAT64               = 6,
    YANG_TYPE_IDENTITY              = 7,
    YANG_TYPE_INSTANCE_IDENTIFIER   = 8,
    YANG_TYPE_INT8                  = 9,
    YANG_TYPE_INT16                 = 10,
    YANG_TYPE_INT32                 = 11,
    YANG_TYPE_INT64                 = 12,
    YANG_TYPE_LEAFREF               = 13,
    YANG_TYPE_STRING                = 14,
    YANG_TYPE_UINT8                 = 15,
    YANG_TYPE_UINT16                = 16,
    YANG_TYPE_UINT32                = 17,
    YANG_TYPE_UINT64                = 18,
    YANG_TYPE_UNION                 = 19
} YangBuiltInTypes;

typedef struct _YangNode {
    YangNode            export;
    int                 isOriginal;
    void                *info;
    int                 line;
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

typedef struct _YangIdentifierRefInfo {
    char		*prefix;
	char		*identifierName;
    _YangNode   *resolvedNode;
    _YangNode   *met;
} _YangIdentifierRefInfo;

typedef struct _YangGroupingInfo {
    _YangParsingState state;
} _YangGroupingInfo;

_YangNode *findYangModuleByName(const char *modulename);

_YangNode *addYangNode(const char *value, YangDecl nodeKind, _YangNode *parentPtr);

_YangModuleInfo *createModuleInfo(_YangNode *modulePtr);

_YangNode* findChildNodeByType(_YangNode *nodePtr, YangDecl nodeKind);

_YangNode* findChildNodeByTypeAndValue(_YangNode *nodePtr, YangDecl nodeKind, char* value);

_YangNode* resolveNodeByTypeAndValue(_YangNode *nodePtr, YangDecl nodeKind, char* value, int depth);

_YangNode* resolveReference(_YangNode *currentNodePtr, YangDecl nodeKind, char* prefix, char* identifierName);

_YangNode *externalModule(_YangNode *importNode);

YangBuiltInTypes getBuiltInTypeName(char *name);
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

