/*
 * yang-check.c --
 *
 *      This module contains YANG (semantics) checks 
 *
 * Copyright (c) 2000 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Authors: Kaloyan Kanev, Siarhei Kuryla
 * @(#) $Id: check.c 10751 2008-11-06 22:05:48Z schoenw $
 */

#include <config.h>

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WIN_H
#include "win.h"
#endif

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "yang-data.h"
#include "yang-check.h"
#include "parser-smi.tab.h"


#define SMI_EPOCH	631152000	/* 01 Jan 1990 00:00:00 */ 

/*
 * Current parser defined in parser-yang. Workaround - can't include data.h
 */
extern Parser *currentParser;

int isWSP(char ch) {
    return  (ch == ' ' || ch == '\t');
}

time_t checkDate(Parser *parserPtr, char *date)
{
    struct tm	tm;
    time_t	anytime;
    int		i, len;
    char	*p;

    memset(&tm, 0, sizeof(tm));
    anytime = 0;

    len = strlen(date);
    if (len == 10 || len == 16) {
        for (i = 0; i < len; i++) {
            if (((i < 4 || i == 5 || i == 6 || i == 8 || i == 9 || i == 11
              || i == 12 || i == 14 || i == 15) && ! isdigit((int)date[i]))
            || ((i == 4 || i == 7) && date[i] != '-')
            || (i == 10 && date[i] != ' ')
            || (i == 13 && date[i] != ':')) {
            smiPrintError(parserPtr, ERR_DATE_CHARACTER, date);
            anytime = (time_t) -1;
            break;
            }
        }
    } else {
        smiPrintError(parserPtr, ERR_DATE_LENGTH, date);
        anytime = (time_t) -1;
    }
    
    if (anytime == 0) {
	for (i = 0, p = date, tm.tm_year = 0; i < 4; i++, p++) {
	    tm.tm_year = tm.tm_year * 10 + (*p - '0');
	}
	p++;
	tm.tm_mon = (p[0]-'0') * 10 + (p[1]-'0');
	p += 3;
	tm.tm_mday = (p[0]-'0') * 10 + (p[1]-'0');
	p += 2;
	if (len == 16) {
	    p++;
	    tm.tm_hour = (p[0]-'0') * 10 + (p[1]-'0');
	    p += 3;
	    tm.tm_min = (p[0]-'0') * 10 + (p[1]-'0');
	}
	
	if (tm.tm_mon < 1 || tm.tm_mon > 12) {
	    smiPrintError(parserPtr, ERR_DATE_MONTH, date);
	}
	if (tm.tm_mday < 1 || tm.tm_mday > 31) {
	    smiPrintError(parserPtr, ERR_DATE_DAY, date);
	}
	if (tm.tm_hour < 0 || tm.tm_hour > 23) {
	    smiPrintError(parserPtr, ERR_DATE_HOUR, date);
	}
	if (tm.tm_min < 0 || tm.tm_min > 59) {
	    smiPrintError(parserPtr, ERR_DATE_MINUTES, date);
	}
	
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	tm.tm_isdst = 0;

	anytime = timegm(&tm);

	if (anytime == (time_t) -1) {
	    smiPrintError(parserPtr, ERR_DATE_VALUE, date);
	} else {
	    if (anytime < SMI_EPOCH) {
            smiPrintError(parserPtr, ERR_DATE_IN_PAST, date);
	    }
	    if (anytime > time(NULL)) {
            smiPrintError(parserPtr, ERR_DATE_IN_FUTURE, date);
	    }
	}
}
    
    return (anytime == (time_t) -1) ? 0 : anytime;
}

void validateInclude(_YangNode *module, _YangNode *extModule) {
    if (!extModule) return;
    _YangNode* node = findChildNodeByType(extModule, YANG_DECL_BELONGS_TO);
    if (node) {
        if (module->export.nodeKind == YANG_DECL_MODULE) {
            if (strcmp(node->export.value, module->export.value)) {
                smiPrintError(currentParser, ERR_SUBMODULE_BELONGS_TO_ANOTHER_MODULE, extModule->export.value, module->export.value);
            }
        } else if (module->export.nodeKind = YANG_DECL_SUBMODULE) {
            _YangNode* node1 = findChildNodeByType(module, YANG_DECL_BELONGS_TO);
            if (strcmp(node->export.value, node1->export.value)) {
                smiPrintError(currentParser, ERR_SUBMODULE_BELONGS_TO_ANOTHER_MODULE, extModule->export.value, node1->export.value);
            }            
        }
    } else {
        smiPrintError(currentParser, ERR_SUBMODULE_BELONGS_TO_ANOTHER_MODULE, extModule->export.value, module->export.value);
    }
}

typedef enum YangIdentifierGroup {
     YANG_IDGR_NONE = 0,
     YANG_IDGR_EXTENSION,
     YANG_IDGR_FEATURE,
     YANG_IDGR_IDENTITY,
     YANG_IDGR_TYPEDEF,
     YANG_IDGR_GROUPING,
     YANG_IDGR_NODE,     
} YangIdentifierGroup;

int getIdentifierGroup(YangDecl kind) {
    if (kind == YANG_DECL_EXTENSION) {
        return YANG_IDGR_EXTENSION;
    } else if (kind == YANG_DECL_FEATURE) {
        return YANG_IDGR_FEATURE;
    } else if (kind == YANG_DECL_IDENTITY) {
        return YANG_IDGR_IDENTITY;
    } else if (kind == YANG_DECL_TYPEDEF) {
        return YANG_IDGR_TYPEDEF;
    } else if (kind == YANG_DECL_GROUPING) {
        return YANG_IDGR_GROUPING;
    } else if (kind == YANG_DECL_LEAF || 
               kind == YANG_DECL_LEAF_LIST || 
               kind == YANG_DECL_LIST || 
               kind == YANG_DECL_CONTAINER ||
               kind == YANG_DECL_CHOICE ||
               kind == YANG_DECL_RPC ||
               kind == YANG_DECL_NOTIFICATION ||
               kind == YANG_DECL_ANYXML) {
        return YANG_IDGR_NODE;
    }
    return YANG_IDGR_NONE;
}

int countChildNodesByTypeAndValue(_YangNode *nodePtr, _YangNode *curNode, YangIdentifierGroup group, char* value) {
    _YangNode *childPtr = NULL;
    int ret = 0;
    for (childPtr = nodePtr->firstChildPtr; childPtr && childPtr != curNode; childPtr = childPtr->nextSiblingPtr) {       
        if (getIdentifierGroup(childPtr->export.nodeKind) == group && !strcmp(childPtr->export.value, value)) {
            ret++;
        }
    }
    return ret;
}

int countChoiceChildNodesByTypeAndValue(_YangNode *nodePtr, _YangNode *curNode, YangIdentifierGroup group, char* value) {
    _YangNode *childPtr = NULL;
    int ret = 0;
    for (childPtr = nodePtr->firstChildPtr; childPtr && childPtr != curNode; childPtr = childPtr->nextSiblingPtr) {
        if (childPtr->export.nodeKind == YANG_DECL_CASE) {
            _YangNode *childPtr2 = childPtr->firstChildPtr;
            for (; childPtr2 && childPtr2 != curNode; childPtr2 = childPtr2->nextSiblingPtr) {
                if (getIdentifierGroup(childPtr2->export.nodeKind) == group && !strcmp(childPtr2->export.value, value)) {
                    ret++;
                }                
            }
            if (childPtr2 == curNode) break;
        } else {
            if (getIdentifierGroup(childPtr->export.nodeKind) == group && !strcmp(childPtr->export.value, value)) {
                ret++;
            }
        }
    }
    return ret;
}

int validateNodeUniqueness(_YangNode *nodePtr) {
    YangIdentifierGroup ig = getIdentifierGroup(nodePtr->export.nodeKind);
    _YangNode *cur = nodePtr->parentPtr;
    while (cur) {
        if (cur->export.nodeKind == YANG_DECL_CASE) {
            cur = cur->parentPtr;
            if (countChoiceChildNodesByTypeAndValue(cur, nodePtr, ig, nodePtr->export.value)) {
                return 0;
            }
        } else {
            if (countChildNodesByTypeAndValue(cur, nodePtr, ig, nodePtr->export.value)) {
                return 0;
            }            
        }
        cur = cur->parentPtr;
        if (ig == YANG_IDGR_NODE) {
            break;
        }
    }
    /* check with all submodules if it's a top-level definition or not a data defition statement */
    if (ig != YANG_IDGR_NODE || !nodePtr->parentPtr->parentPtr) {
        _YangNodeList* submodules = ((_YangModuleInfo*)nodePtr->modulePtr->info)->submodules;
        while (submodules) {
            if (countChildNodesByTypeAndValue(submodules->nodePtr, nodePtr, ig, nodePtr->export.value)) {
                return 0;
            }                    
            submodules = submodules->next;
        }    
    }
    return 1;
}

_YangNode *createReferenceNode(_YangNode *parentPtr, _YangNode *reference)
{
	_YangNode *node = (_YangNode*) smiMalloc(sizeof(_YangNode));
    node->isOriginal            = 0;
	node->export.value          = reference->export.value;
	node->export.nodeKind       = reference->export.nodeKind;
    node->export.description	= NULL;
    node->export.reference		= NULL;
    node->export.extra  		= reference->export.extra;
    node->info                  = NULL;    
    
    node->nextSiblingPtr        = NULL;
    node->firstChildPtr         = NULL;
    node->lastChildPtr          = NULL;
    node->parentPtr             = parentPtr;
    node->modulePtr             = parentPtr->modulePtr;
    
    if(parentPtr->lastChildPtr)
    {
        (parentPtr->lastChildPtr)->nextSiblingPtr = node;
        parentPtr->lastChildPtr = node;
    }
    else //first child
    {
        parentPtr->firstChildPtr = node;
        parentPtr->lastChildPtr = node;
    }    
    return node;
}

int isDataDefinitionNode(_YangNode *node) {
    if (!node) return;
    YangDecl kind = node->export.nodeKind;
    return (kind == YANG_DECL_CONTAINER ||
            kind == YANG_DECL_LEAF ||
            kind == YANG_DECL_LEAF_LIST ||
            kind == YANG_DECL_LIST || 
            kind == YANG_DECL_CHOICE || 
            kind == YANG_DECL_ANYXML ||
            kind == YANG_DECL_USES);
}

void copySubtree(_YangNode *destPtr, _YangNode *subtreePtr) {
    _YangNode *reference = createReferenceNode(destPtr, subtreePtr);
    _YangNode* childPtr = subtreePtr->firstChildPtr;
    while (childPtr) {
        copySubtree(reference, childPtr);
        childPtr = childPtr->nextSiblingPtr;
    }
}

int expandGroupings(_YangNode *node) {
    if (!node || !node->isOriginal) return;
    YangDecl nodeKind = node->export.nodeKind;
    if (nodeKind == YANG_DECL_GROUPING) {
        if (node->info) {
            _YangGroupingInfo *info = (_YangGroupingInfo*)node->info;
            if (info->state == YANG_PARSING_IN_PROGRESS) {
                smiPrintErrorAtLine(currentParser, ERR_CYCLIC_REFERENCE_CHAIN, node->line, node->export.value);
                return 0;
            }
            return 1;
        }
        _YangGroupingInfo *info = smiMalloc(sizeof(_YangGroupingInfo));
        info->state = YANG_PARSING_IN_PROGRESS;
        node->info = info;
    }
    if (nodeKind == YANG_DECL_USES) {
        _YangIdentifierRefInfo* info = (_YangIdentifierRefInfo*)node->info;
        if (info->resolvedNode) {
            if (expandGroupings(info->resolvedNode)) {            
                _YangNode *refChild = info->resolvedNode->firstChildPtr;
                while (refChild) {
                    if (isDataDefinitionNode(refChild)) {
                        copySubtree(node->parentPtr, refChild);
                    }
                    refChild = refChild->nextSiblingPtr;
                }
            }
        }
    }
    
    _YangNode *child = node->firstChildPtr;
    while (child) {
        if (child->isOriginal) {
            expandGroupings(child);
        }
        child = child->nextSiblingPtr;
    }
    if (nodeKind == YANG_DECL_GROUPING) {
        ((_YangGroupingInfo*)node->info)->state = YANG_PARSING_DONE;
    }
    return 1;
}

/*
 * Verifies that all identifiers are unique within all namespaces
 */
void uniqueNames(_YangNode* nodePtr) { 
    /* go over all child nodes*/
    _YangNode* cur = nodePtr->firstChildPtr;
    while (cur) {
        YangIdentifierGroup yig = getIdentifierGroup(cur->export.nodeKind);
        if (yig > YANG_IDGR_NONE) {            
            if (!validateNodeUniqueness(cur)) {
                if (cur->line == 0) {
                    smiPrintError(currentParser, ERR_DUPLICATED_GROUPING, cur->export.value);
                } else {
                    smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_IDENTIFIER, cur->line, cur->export.value);
                }
            }
        }
        uniqueNames(cur);
        cur = cur->nextSiblingPtr;
    }
}

int map[65];

void initMap() {
    map[YANG_DECL_UNKNOWN_STATEMENT] = YANG_DECL_EXTENSION;
    map[YANG_DECL_IF_FEATURE] = YANG_DECL_FEATURE;
    map[YANG_DECL_TYPE] = YANG_DECL_TYPEDEF;
    map[YANG_DECL_USES] = YANG_DECL_GROUPING;
    map[YANG_DECL_BASE] = YANG_DECL_IDENTITY;
}

void resolveReferences(_YangNode* node) {
    YangDecl nodeKind = node->export.nodeKind;
    if (nodeKind == YANG_DECL_UNKNOWN_STATEMENT ||
        nodeKind == YANG_DECL_IF_FEATURE ||
        (nodeKind == YANG_DECL_TYPE && getBuiltInTypeName(node->export.value) == YANG_TYPE_NONE) ||
        nodeKind == YANG_DECL_USES ||
        nodeKind == YANG_DECL_BASE) {
            _YangIdentifierRefInfo* identifierRef = (_YangIdentifierRefInfo*)node->info;            
            if (!identifierRef->resolvedNode) {

                _YangNode *reference = resolveReference(node->parentPtr, map[nodeKind], identifierRef->prefix, identifierRef->identifierName);
                if (!reference) {
                    smiPrintErrorAtLine(currentParser, ERR_REFERENCE_NOT_RESOLVED, node->line, identifierRef->prefix, identifierRef->identifierName);
                }
                identifierRef->resolvedNode = reference;
                identifierRef->met = node;
                
                if (nodeKind == YANG_DECL_UNKNOWN_STATEMENT) {
                    /* check the argument */
                    _YangNode *argument = findChildNodeByType(reference, YANG_DECL_ARGUMENT);
                    if (argument && !node->export.extra) {
                        smiPrintErrorAtLine(currentParser, ERR_EXPECTED_EXTENSION_ARGUMENT, node->line, node->export.value);
                    } else if (!argument && node->export.extra) {
                        smiPrintErrorAtLine(currentParser, ERR_UNEXPECTED_EXTENSION_ARGUMENT, node->line, node->export.value);
                    }
                } else if (nodeKind == YANG_DECL_IF_FEATURE || nodeKind == YANG_DECL_BASE) {
                    /* check whether there is no cyclic reference */
                    if (node->parentPtr->export.nodeKind == map[nodeKind]) {
                        _YangNode *cur = identifierRef->resolvedNode;
                        while (cur) {
                            _YangNode *childRef = findChildNodeByType(cur, nodeKind);
                            if (childRef) {
                                _YangIdentifierRefInfo* info = ((_YangIdentifierRefInfo*)childRef->info);
                                if (!info->resolvedNode) {
                                        info->resolvedNode = resolveReference(node->parentPtr, map[nodeKind], identifierRef->prefix, identifierRef->identifierName);
                                        if (!info->resolvedNode) {
                                            smiPrintErrorAtLine(currentParser, ERR_REFERENCE_NOT_RESOLVED, childRef->line, info->prefix, info->identifierName);
                                        }
                                        info->met = node;
                                        cur = info->resolvedNode;
                                } else {
                                    if (info->met == node) {
                                        smiPrintErrorAtLine(currentParser, ERR_CYCLIC_REFERENCE_CHAIN, info->resolvedNode->line, info->resolvedNode->export.value);
                                    }
                                    break;
                                }                                     
                            } else {
                                break;
                            }
                        }
                    }
                }
                    
            }
    }

    _YangNode *child = node->firstChildPtr;
    while (child) {
        resolveReferences(child);
        child = child->nextSiblingPtr;
    }
}

void semanticAnalysis(_YangNode *module) {   
    initMap();
    resolveReferences(module);
    expandGroupings(module);
    uniqueNames(module);
}
