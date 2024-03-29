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

#include "yang.h"

#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "yang-data.h"
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
#include "yang-data.h"


#define SMI_EPOCH	631152000	/* 01 Jan 1990 00:00:00 */ 

/*
 * Current parser defined in parser-yang. Workaround - can't include data.h
 */
extern Parser *currentParser;


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
     YANG_IDGR_CASE
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
    } else if (kind == YANG_DECL_CASE) {
        return YANG_IDGR_CASE;
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
        YangList* submodules = ((_YangModuleInfo*)nodePtr->modulePtr->info)->submodules;
        while (submodules) {
            if (countChildNodesByTypeAndValue(listNode(submodules), nodePtr, ig, nodePtr->export.value)) {
                return 0;
            }                    
            submodules = submodules->next;
        }    
    }
    return 1;
}

_YangNode *createReferenceNode(_YangNode *parentPtr, _YangNode *reference, YangNodeType nodeType)
{
	_YangNode *node = (_YangNode*) smiMalloc(sizeof(_YangNode));
    node->nodeType              = nodeType;
	node->export.value          = reference->export.value;
	node->export.nodeKind       = reference->export.nodeKind;
    node->export.config 		= reference->export.config;
    node->export.status 		= reference->export.status;
    node->line                  = reference->line;
    node->export.description	= NULL;
    node->export.reference		= NULL;
    node->export.extra  		= reference->export.extra;
    node->info                  = NULL;    
    node->typeInfo              = NULL;
    
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

int isMandatory(_YangNode *nodePtr) {
    _YangNode *mandatory = findChildNodeByType(nodePtr, YANG_DECL_MANDATORY);
    if (mandatory && !strcmp(mandatory->export.value, "true")) {
        return 1;
    }
    return 0;
}

void copySubtree(_YangNode *destPtr, _YangNode *subtreePtr, YangNodeType nodeType, int skipMandatory) {
    if (skipMandatory && isMandatory(subtreePtr)) {
        smiPrintErrorAtLine(currentParser, ERR_AUGMENTATION_BY_MANDATORY_NODE, subtreePtr->line);
    }
    _YangNode *reference = createReferenceNode(destPtr, subtreePtr, nodeType);
    _YangNode* childPtr = subtreePtr->firstChildPtr;
    while (childPtr) {
        copySubtree(reference, childPtr, nodeType, skipMandatory);
        childPtr = childPtr->nextSiblingPtr;
    }
}

_YangNode* findTargetNode(_YangNode *nodePtr, char* value) {
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        if (isSchemaNode(childPtr->export.nodeKind) && !strcmp(childPtr->export.value, value)) {
            return childPtr;
        }
    }
    return NULL;
}

/*
 *  Resolves a node in the current or imported module by an XPath expression.
 */
_YangNode *resolveXPath(_YangNode *nodePtr) {
    YangList *listPtr = getXPathNode(nodePtr->export.value), *tmp;
    if (!listPtr) return NULL;
    
    _YangNode *cur = NULL, *tmpNode;
    /* 'uses' substatement */
    if (nodePtr->parentPtr->export.nodeKind == YANG_DECL_USES) {
        /* let's start from the node which is the parent of the 'uses'*/
        if (!validatePrefixes(listPtr, getModuleInfo(nodePtr->modulePtr)->prefix, 0)) {
            freeIdentiferList(listPtr);
            return NULL;            
        }
        cur = nodePtr->parentPtr->parentPtr;
    } else {
        /* 'module' substatement */
        _YangModuleInfo *info = getModuleInfo(nodePtr->modulePtr);
        if (listIdentifierRef(listPtr)->prefix && strcmp(listIdentifierRef(listPtr)->prefix, info->prefix)) {
            cur = findYangModuleByPrefix(nodePtr->modulePtr, listIdentifierRef(listPtr)->prefix);
            if (!cur) {
                freeIdentiferList(listPtr);
                return NULL;
            }
            if (!validatePrefixes(listPtr, listIdentifierRef(listPtr)->prefix, 1)) {
                freeIdentiferList(listPtr);
                return NULL;
            }            
        } else {            
            cur = nodePtr->modulePtr;
            if (!validatePrefixes(listPtr, getModuleInfo(cur)->prefix, 0)) {
                freeIdentiferList(listPtr);
                return NULL;
            }
        }
    }
    tmp = listPtr;
    YangList* submodules = NULL;
    while (listPtr) {
        tmpNode = cur;
        cur = findTargetNode(cur, listIdentifierRef(listPtr)->ident);
        if (!cur) {
            if (submodules) {                
                cur = listNode(submodules);
                submodules = submodules->next;
                continue;
            }
            if (tmpNode->export.nodeKind == YANG_DECL_MODULE || tmpNode->export.nodeKind == YANG_DECL_SUBMODULE) {
                submodules = getModuleInfo(tmpNode)->submodules;
                if (submodules) {
                    cur = listNode(submodules);
                    submodules = submodules->next;
                    continue;                
                }
            }
            
            freeIdentiferList(tmp);
            return NULL;
        }
        listPtr = listPtr->next;
    }
    freeIdentiferList(tmp);
    return cur;
}

int isAllowedStatement(int stmt, int *allowedStmts, int len) {
    int i = 0;
    for (; i < len; i++)
        if (allowedStmts[i] == stmt) {
            return 1;
        }
    return 0;
}

void applyRefine(_YangNode* target, _YangNode* refinement, int* allowedStmts, int len) {
    _YangNode *child = refinement->firstChildPtr;
    
    while (child) {
        if (!isAllowedStatement(child->export.nodeKind, allowedStmts, len)) {
            smiPrintErrorAtLine(currentParser, ERR_INVALID_REFINE, child->line, yandDeclKeyword[target->export.nodeKind], target->export.value, yandDeclKeyword[child->export.nodeKind]);
        } else {
            if (child->export.nodeKind == YANG_DECL_MUST_STATEMENT) {
                copySubtree(target, child, YANG_NODE_REFINED, 0);
            } else if (child->export.nodeKind == YANG_DECL_DESCRIPTION || 
                child->export.nodeKind == YANG_DECL_REFERENCE) {
                /* just skip, because they are not relevant for the future checks */
            } else if (child->export.nodeKind == YANG_DECL_PRESENCE) {
                if (!findChildNodeByType(target, child->export.nodeKind)) {
                    copySubtree(target, child, YANG_NODE_REFINED, 0);
                }
            } else if (child->export.nodeKind == YANG_DECL_CONFIG ||
                    child->export.nodeKind == YANG_DECL_DEFAULT || 
                    child->export.nodeKind == YANG_DECL_MANDATORY || 
                    child->export.nodeKind == YANG_DECL_MIN_ELEMENTS ||
                    child->export.nodeKind == YANG_DECL_MAX_ELEMENTS) {
                _YangNode *oldOne = findChildNodeByType(target, child->export.nodeKind);
                if (oldOne) {
                    smiFree(oldOne->export.value);
                    oldOne->export.value = smiStrdup(child->export.value);
                } else {
                    copySubtree(target, child, YANG_NODE_REFINED, 0);
                    oldOne = child;
                }
                if (oldOne->export.nodeKind == YANG_DECL_CONFIG) {
                    if (!strcmp(oldOne->export.value, "true")) {
                        setConfig(target, YANG_CONFIG_TRUE);
                    } else {  
                        setConfig(target, YANG_CONFIG_FALSE);
                    }
                }
            }
        }
        child = child->nextSiblingPtr;
    }
}

/*
 * From the specification:
 *  1. The effect of a "uses" reference to a grouping is that the nodes defined by the grouping 
 *      are copied into the current schema tree and then updated according to the refinement statements. 
 *
 *  2. Once a grouping is defined, it can be referenced in a "uses"  statement (see Section 7.12).  
 *      A grouping MUST NOT reference itself,   neither directly nor indirectly through a chain of other groupings. 

 */
int expandGroupings(_YangNode *node) {
    if (!node || node->nodeType != YANG_NODE_ORIGINAL) return;
    YangDecl nodeKind = node->export.nodeKind;
    if (nodeKind == YANG_DECL_GROUPING) {
        if (node->info) {
            _YangGroupingInfo *info = (_YangGroupingInfo*)node->info;
            if (info->state == YANG_PARSING_IN_PROGRESS) {
                smiPrintErrorAtLine(currentParser, ERR_CYCLIC_REFERENCE_CHAIN, node->line, yandDeclKeyword[node->export.nodeKind], node->export.value);
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
                        copySubtree(node->parentPtr, refChild, YANG_NODE_EXPANDED_USES, 0);
                    }
                    refChild = refChild->nextSiblingPtr;
                }
                
                /* Apply refinements if there are any  */
                _YangNode *child = node->firstChildPtr;
                while (child) {
                    if (child->export.nodeKind == YANG_DECL_REFINE) {
                        _YangNode* refinement = child;
                        if (refinement) {
                            _YangNode *targetNodePtr = resolveXPath(refinement);

                            if (!targetNodePtr) {
                                smiPrintErrorAtLine(currentParser, ERR_XPATH_NOT_RESOLVED, refinement->line, refinement->export.value);
                            } else {
                                if (targetNodePtr->export.nodeKind == YANG_DECL_CONTAINER) {
                                    int types[] = {YANG_DECL_MUST_STATEMENT, YANG_DECL_PRESENCE, YANG_DECL_CONFIG, YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 4);
                                } else if (targetNodePtr->export.nodeKind == YANG_DECL_LEAF) {
                                    int types[] = {YANG_DECL_MUST_STATEMENT, YANG_DECL_DEFAULT, YANG_DECL_CONFIG, YANG_DECL_MANDATORY, YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 6);
                                } else if (targetNodePtr->export.nodeKind == YANG_DECL_LEAF_LIST || targetNodePtr->export.nodeKind == YANG_DECL_LIST) {
                                    int types[] = {YANG_DECL_MUST_STATEMENT, YANG_DECL_CONFIG, YANG_DECL_MIN_ELEMENTS, YANG_DECL_MAX_ELEMENTS, YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 6);
                                } else if (targetNodePtr->export.nodeKind == YANG_DECL_CHOICE) {
                                    int types[] = {YANG_DECL_DEFAULT, YANG_DECL_CONFIG, YANG_DECL_MANDATORY, YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 5);
                                } else if (targetNodePtr->export.nodeKind == YANG_DECL_CASE) {
                                    int types[] = {YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 2);
                                } else if (targetNodePtr->export.nodeKind == YANG_DECL_ANYXML) {
                                    int types[] = {YANG_DECL_CONFIG, YANG_DECL_MANDATORY, YANG_DECL_DESCRIPTION, YANG_DECL_REFERENCE};
                                    applyRefine(targetNodePtr, refinement, types, 4);
                                }                            
                            }
                        }
                    }                    
                    child = child->nextSiblingPtr;
                }               
            }
        }
    }
    
    _YangNode *child = node->firstChildPtr;
    while (child) {
        expandGroupings(child);
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
                if (cur->nodeType == YANG_NODE_EXPANDED_USES) {
                    smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_NODE_WHILE_GROUPING_INSTANTIATION, cur->line, cur->export.value);
                } else if (cur->nodeType == YANG_NODE_EXPANDED_AUGMENT) {
                    smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_NODE_WHILE_AUGMENT_INSTANTIATION, cur->line, cur->export.value);
                } else {
                    smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_IDENTIFIER, cur->line, cur->export.value);
                }
            }
        }
        uniqueNames(cur);
        cur = cur->nextSiblingPtr;
    }
}

/*
 *  Should be reimplemented in more efficient way by using some appropriate datastructures, like HashTable
 */
void uniqueSubmoduleDefinitions(_YangNode* modulePtr) {
    /* validate name uniqueness of the top level definitions in all submodules */
    YangList* submodulePtr = ((_YangModuleInfo*)modulePtr->info)->submodules, *firstSubmodulePtr;
    firstSubmodulePtr = submodulePtr;
    while (submodulePtr) {
        _YangNode* curNodePtr = listNode(submodulePtr)->firstChildPtr;
        while (curNodePtr) {
            YangIdentifierGroup ig = getIdentifierGroup(curNodePtr->export.nodeKind);
            if (ig > YANG_IDGR_NONE) {
               YangList* sPtr = firstSubmodulePtr;
                while (sPtr != submodulePtr) {
                    if (countChildNodesByTypeAndValue(listNode(sPtr), curNodePtr, ig, curNodePtr->export.value)) {
                        smiPrintErrorAtLine(((Parser*)getModuleInfo(listNode(submodulePtr))->parser), ERR_IDENTIFIER_DEFINED_IN_OTHER_SUBMODLE, curNodePtr->line, curNodePtr->export.value, listNode(sPtr)->export.value);
                    }
                    sPtr = sPtr->next;
                }
            }
            curNodePtr = curNodePtr->nextSiblingPtr;
        }        
        submodulePtr= submodulePtr->next;
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

 /* 
  *  Resolves references to extensions, features, defined types, groupings and identities.
  *  Validates whether there are no circular dependencies between these nodes.
  */  
void resolveReferences(_YangNode* node) {
    YangDecl nodeKind = node->export.nodeKind;
    if (nodeKind == YANG_DECL_UNKNOWN_STATEMENT ||
        nodeKind == YANG_DECL_IF_FEATURE ||
        (nodeKind == YANG_DECL_TYPE && getBuiltInType(node->export.value) == YANG_TYPE_NONE) ||
        nodeKind == YANG_DECL_USES ||
        nodeKind == YANG_DECL_BASE) {
            _YangIdentifierRefInfo* identifierRef = (_YangIdentifierRefInfo*)node->info;
            if (!identifierRef->resolvedNode) {

                _YangNode *reference = resolveReference(node->parentPtr, map[nodeKind], identifierRef->prefix, identifierRef->identifierName);
                if (!reference) {
                    smiPrintErrorAtLine(currentParser, ERR_REFERENCE_NOT_RESOLVED, node->line, identifierRef->prefix, identifierRef->identifierName);
                }
                identifierRef->resolvedNode = reference;
                identifierRef->marker = node;
                /* store a base type */
                if (nodeKind == YANG_DECL_TYPE) {
                    node->typeInfo->baseTypeNodePtr = reference;
                }
                
                if (nodeKind == YANG_DECL_UNKNOWN_STATEMENT) {
                    /* check the argument */
                    _YangNode *argument = findChildNodeByType(reference, YANG_DECL_ARGUMENT);
                    if (argument && !node->export.extra) {
                        smiPrintErrorAtLine(currentParser, ERR_EXPECTED_EXTENSION_ARGUMENT, node->line, node->export.value);
                    } else if (!argument && node->export.extra) {
                        smiPrintErrorAtLine(currentParser, ERR_UNEXPECTED_EXTENSION_ARGUMENT, node->line, node->export.value);
                    }
                } else if (nodeKind == YANG_DECL_IF_FEATURE || nodeKind == YANG_DECL_BASE || nodeKind == YANG_DECL_TYPE) {
                    /* check whether there is no cyclic reference */
                    if (node->parentPtr->export.nodeKind == map[nodeKind]) {
                        _YangNode *cur = identifierRef->resolvedNode;
                        while (cur) {
                            _YangNode *childRef = findChildNodeByType(cur, nodeKind);
                            /* Skip basic types, they don't have info */
                            if (childRef && childRef->info) {
                                _YangIdentifierRefInfo* info = ((_YangIdentifierRefInfo*)childRef->info);
                                if (!info->resolvedNode) {
                                        info->resolvedNode = resolveReference(node->parentPtr, map[nodeKind], identifierRef->prefix, identifierRef->identifierName);
                                        if (!info->resolvedNode) {
                                            smiPrintErrorAtLine(currentParser, ERR_REFERENCE_NOT_RESOLVED, childRef->line, info->prefix, info->identifierName);
                                        }
                                        /* store a base type */
                                        if (nodeKind == YANG_DECL_TYPE) {
                                            node->typeInfo->baseTypeNodePtr = info->resolvedNode;
                                        }
                                        
                                        info->marker = node;
                                        cur = info->resolvedNode;
                                } else {
                                    if (info->marker == node) {
                                        smiPrintErrorAtLine(currentParser, ERR_CYCLIC_REFERENCE_CHAIN, info->resolvedNode->line, yandDeclKeyword[map[nodeKind]], info->resolvedNode->export.value);
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

int validatePrefixes(YangList *listPtr, char* modulePrefix, int prefixRequired) {
    YangList *c = listPtr;
    while (c) {
        if (listIdentifierRef(c)->prefix && strcmp(listIdentifierRef(c)->prefix, modulePrefix)) {
            return 0;
        }
        if (!listIdentifierRef(c)->prefix && prefixRequired) {
            return 0;
        }
        c = c->next;
    }
    return 1;
}

int isPossibleAugmentTargetNode(YangDecl kind) {
    return kind == YANG_DECL_CONTAINER ||
           kind == YANG_DECL_LIST ||
           kind == YANG_DECL_CHOICE ||
           kind == YANG_DECL_CASE ||
           kind == YANG_DECL_INPUT ||
           kind == YANG_DECL_OUTPUT ||
           kind == YANG_DECL_NOTIFICATION;
}

int isSchemaNode(YangDecl kind) {
    return kind == YANG_DECL_CONTAINER ||
           kind == YANG_DECL_LEAF ||
           kind == YANG_DECL_LEAF_LIST ||
           kind == YANG_DECL_LIST ||
           kind == YANG_DECL_CHOICE ||
           kind == YANG_DECL_CASE ||
           kind == YANG_DECL_RPC ||
           kind == YANG_DECL_INPUT ||
           kind == YANG_DECL_OUTPUT ||
           kind == YANG_DECL_ANYXML ||
           kind == YANG_DECL_NOTIFICATION;
}

/*
 * Expands all augment statements.
 */
void expangAugments(_YangNode* node) {
    _YangNode *child = node->firstChildPtr;
    while (child) {
        expangAugments(child);
        child = child->nextSiblingPtr;
    }    
    YangDecl nodeKind = node->export.nodeKind;    
    if (nodeKind == YANG_DECL_AUGMENT) {
        /* resolve XPath to get the target node */
       _YangNode *targetNodePtr = resolveXPath(node);
       
       if (!targetNodePtr) {
            smiPrintErrorAtLine(currentParser, ERR_XPATH_NOT_RESOLVED, node->line, node->export.value);
       } else {
            /* check whether the target node is of required type */
            if (!isPossibleAugmentTargetNode(targetNodePtr->export.nodeKind)) {
                smiPrintErrorAtLine(currentParser, ERR_WRONG_AUGMENT_TARGET_NODE, node->line, node->export.value);
                return;
            }
            /* expand augment */
            _YangNode *child = node->firstChildPtr;
            
            /*  
             *  From the specification:
             *  If the target node of the “augment” is in another module, 
             *  then nodes added by the augmentation MUST NOT be mandatory nodes. 
             */
            int isAnotherModule = 1;
            if (!strcmp(getModuleInfo(targetNodePtr->modulePtr)->prefix, getModuleInfo(node->modulePtr)->prefix)) {
                isAnotherModule = 0;
            }
            while (child) {                
                int isAllowed = 1;
                if (isAnotherModule) {
                    YangIdentifierGroup ig = getIdentifierGroup(child->export.nodeKind);
                    if (ig != YANG_IDGR_NONE && countChildNodesByTypeAndValue(targetNodePtr, NULL, ig, child->export.value)) {
                        smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_NODE_WHILE_AUGMENT_INSTANTIATION, child->line, child->export.value);
                        child = child->nextSiblingPtr;
                        continue;
                    }
                }
                
                /*
                 * If the target node is a container, list, case, input, output, or notification node, 
                 * the "container", "leaf", "list", "leaf-list", "uses", and "choice" statements can be used within the "augment" statement. 
                 * If the target node is a choice node, the "case" statement can be used within the "augment" statement. 
                 */
                if (child->export.nodeKind == YANG_DECL_ANYXML) {
                    smiPrintErrorAtLine(currentParser, ERR_NODE_KIND_NOT_ALLOWED, child->line, yandDeclKeyword[YANG_DECL_ANYXML], child->export.value, yandDeclKeyword[targetNodePtr->export.nodeKind], targetNodePtr->export.value);
                } else if (isDataDefNode(child)) {
                    if (targetNodePtr->export.nodeKind == YANG_DECL_CHOICE) {
                        smiPrintErrorAtLine(currentParser, ERR_NODE_KIND_NOT_ALLOWED, child->line, yandDeclKeyword[child->export.nodeKind], child->export.value, yandDeclKeyword[targetNodePtr->export.nodeKind], targetNodePtr->export.value);
                    }
                    /*
                     *  If the target node is in the external module we should check whether adding this node does not break uniqueness
                     *  (because all imported modules have been already validated)
                     */
                    copySubtree(targetNodePtr, child, YANG_NODE_EXPANDED_AUGMENT, isAnotherModule);
                } else if (child->export.nodeKind == YANG_DECL_CASE) {
                    if (targetNodePtr->export.nodeKind != YANG_DECL_CHOICE) {
                        smiPrintErrorAtLine(currentParser, ERR_NODE_KIND_NOT_ALLOWED, child->line, yandDeclKeyword[child->export.nodeKind], child->export.value, yandDeclKeyword[targetNodePtr->export.nodeKind], targetNodePtr->export.value);
                    }
                    copySubtree(targetNodePtr, child, YANG_NODE_EXPANDED_AUGMENT, isAnotherModule);
                }
                child = child->nextSiblingPtr;
            }
           
       }
    }    
}

/*
 *  From the specification:
 *  If a node has "config" "false", no node underneath it can have "config" set to "true".
 *  If a "config" statement is present for any node in the input, output or notification tree, it is ignored.
 */
void validateConfigProperties(_YangNode *nodePtr, int isConfigTrue, int ignore) {
    if (ignore) {
        if (nodePtr->export.nodeKind == YANG_DECL_CONFIG) {
            smiPrintErrorAtLine(currentParser, ERR_IGNORED_CONFIG, nodePtr->line);
        }
    } else if (!isConfigTrue) {
        if (nodePtr->export.config == YANG_CONFIG_TRUE) {
            smiPrintErrorAtLine(currentParser, ERR_INVALID_CONFIG, nodePtr->line, nodePtr->export.value);
            nodePtr->export.config = YANG_CONFIG_DEFAULT_FALSE;
        }
        if (nodePtr->export.config == YANG_CONFIG_DEFAULT_TRUE) {
            nodePtr->export.config = YANG_CONFIG_DEFAULT_FALSE;
        }
    }
    int ignoreFlag = ignore;
    if (nodePtr->export.nodeKind == YANG_DECL_INPUT ||
        nodePtr->export.nodeKind == YANG_DECL_OUTPUT ||
        nodePtr->export.nodeKind == YANG_DECL_NOTIFICATION) {
        ignoreFlag = 1;
    }
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        validateConfigProperties(childPtr, yangIsTrueConf(nodePtr->export.config), ignoreFlag);
    }
}

void validateLists(_YangNode *nodePtr) {
    if (nodePtr->export.nodeKind == YANG_DECL_LIST) {
        /*
         *  From the specification:
         *  The "key" statement, which MUST be present if the list represents configuration, and MAY be present otherwise, 
         *  takes as an argument a string which specifies a space separated list of leaf identifiers of this list.  
         *  Each such leaf identifier MUST refer to a child leaf of the list.  A leaf that is part of the key can be of any built-in or derived type, except it MUST NOT be the built-in type "empty". 
         *  All key leafs in a list MUST have the same value for their "config"  as the list itself. 
         */
        _YangNode *key = findChildNodeByType(nodePtr, YANG_DECL_KEY);
        if (yangIsTrueConf(nodePtr->export.config)) {
            if (!key) {
                smiPrintErrorAtLine(currentParser, ERR_KEY_REQUIRED, nodePtr->line, nodePtr->export.value);
            }
        }        
        if (key) {
            YangList *keys = (YangList*)key->info;
            while (keys) {
                _YangNode *leafPtr = findChildNodeByTypeAndValue(nodePtr, YANG_DECL_LEAF, listIdentifierRef(keys)->ident);
                if (!leafPtr) {
                    smiPrintErrorAtLine(currentParser, ERR_INVALID_KEY_REFERENCE, key->line, listIdentifierRef(keys)->ident);
                } else {
                    _YangNode *type = findChildNodeByType(leafPtr, YANG_DECL_TYPE);
                    if (!strcmp(type->export.value, "empty")) {
                        smiPrintErrorAtLine(currentParser, ERR_EMPTY_KEY, key->line, leafPtr->export.value);
                    }
                    if (yangIsTrueConf(nodePtr->export.config) != yangIsTrueConf(leafPtr->export.config)) {
                        smiPrintErrorAtLine(currentParser, ERR_INVALID_KEY_LEAF_CONFIG_VALUE, key->line, leafPtr->export.value, nodePtr->export.value);
                    }
                }
                keys = keys->next;
            }
        }

        /*
         *  From the specification:  
         *  The "unique" statement takes as an argument a string which contains a space separated list of schema node identifiers, 
         *  which MUST be given in the descendant form. Each such schema node identifier MUST refer to a leaf. 
         *  If one of the referenced leafs represents configuration data, then all of the referenced leafs MUST represent configuration data. 
         */
        _YangNode *childPtr = nodePtr->firstChildPtr;
        while (childPtr) {            
            if (childPtr->export.nodeKind == YANG_DECL_UNIQUE) {
                _YangList* l = (_YangList*)childPtr->info;
                int configNodeCount = 0, stateNodeCount = 0;
                while (l) {                    
                    YangList* il = (YangList*)l->data;
                    _YangNode* cur = nodePtr;
                    while (il) {
                        cur = cur->firstChildPtr;                        
                        while (cur) {
                            if (isDataDefNode(cur) && !strcmp(cur->export.value, listIdentifierRef(il)->ident)) break;
                            cur = cur->nextSiblingPtr;
                        }
                        if (!cur) {
                            smiPrintErrorAtLine(currentParser, ERR_INVALID_UNIQUE_REFERENCE, childPtr->line, l->additionalInfo);
                            break;
                        }
                        il = il->next;
                    }
                    if (cur && cur->export.nodeKind != YANG_DECL_LEAF) {
                        smiPrintErrorAtLine(currentParser, ERR_INVALID_UNIQUE_REFERENCE, childPtr->line, l->additionalInfo);
                        break;
                    } else if (cur) {
                        if (yangIsTrueConf(cur->export.config)) {
                            configNodeCount++;
                        } else {
                            stateNodeCount++;
                        }
                    }                    
                    l = l->next;
                }
                /*
                 *  If one of the referenced leafs represents configuration data, 
                 *  then all of the referenced leafs MUST represent configuration data.
                 */                
                if (configNodeCount && stateNodeCount) {
                    smiPrintErrorAtLine(currentParser, ERR_MUST_BE_CONFIG, childPtr->line);
                }
            }
            childPtr = childPtr->nextSiblingPtr;
        }
    }
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        validateLists(childPtr);
    }    
}

void validateDefaultStatements(_YangNode *nodePtr) {
    YangDecl nodeKind = nodePtr->export.nodeKind;
    if (nodeKind == YANG_DECL_DEFAULT) {
        YangDecl parentKind = nodePtr->parentPtr->export.nodeKind;

        /* An empty type cannot have a default value. */
        if (parentKind == YANG_DECL_LEAF || parentKind == YANG_DECL_TYPEDEF) {
            _YangNode* typePtr = findChildNodeByType(nodePtr->parentPtr, YANG_DECL_TYPE);
            if (getBuiltInType(typePtr->export.value) == YANG_TYPE_EMPTY) {
                smiPrintErrorAtLine(currentParser, ERR_DEFAULT_NOT_ALLOWED, nodePtr->line);
            }
        }

        /* The "default" statement of the leaf and choice MUST NOT be present where "mandatory" is true. */
        if (parentKind == YANG_DECL_CHOICE || parentKind == YANG_DECL_LEAF) {
            _YangNode* mandatory = findChildNodeByType(nodePtr->parentPtr, YANG_DECL_MANDATORY);
            if (mandatory && !strcmp(mandatory->export.value, "true")) {
                smiPrintErrorAtLine(currentParser, ERR_IVALIDE_DEFAULT, nodePtr->line);
            }
        }
        
        /*
         * The argument of the choice is the identifier of the "case" statement.
         * There MUST NOT be any mandatory nodes directly under the default case. 
         */
        if (parentKind == YANG_DECL_CHOICE) {
            _YangNode *defaultCase = findChildNodeByTypeAndValue(nodePtr->parentPtr, YANG_DECL_CASE, nodePtr->export.value);
            if (!defaultCase) {
                smiPrintErrorAtLine(currentParser, ERR_IVALIDE_DEFAULT_CASE, nodePtr->line, nodePtr->export.value);
            } else {
               /* Mandatory nodes:
                * 1. A leaf or choice node with a "mandatory" statement with the value "true".
                * 2. A list or leaf-list node with a "min-elements" statement with a value greater than zero.
                * 3. A container node without a "presence" statement.
                */                
                _YangNode *childPtr = NULL;
                _YangNode *mandatory = NULL;
                for (childPtr = defaultCase->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
                    mandatory = findChildNodeByTypeAndValue(childPtr, YANG_DECL_MANDATORY, "true");
                    if (!mandatory) {
                        mandatory = findChildNodeByType(childPtr, YANG_DECL_PRESENCE);
                    }
                    if (!mandatory) {
                        mandatory = findChildNodeByType(childPtr, YANG_DECL_MIN_ELEMENTS);
                        if (mandatory->export.value[0] == '0') {
                            mandatory = NULL;
                        }
                    }
                    if (mandatory) break;                   
                }        
                smiPrintErrorAtLine(currentParser, ERR_MANDATORY_NODE_UNDER_DEFAULT_CASE, nodePtr->line, defaultCase->export.value, nodePtr->parentPtr->export.value);
            }
        }
    }
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        validateDefaultStatements(childPtr);
    }        
}

void typeHandler(_YangNode* nodePtr) {
    if (nodePtr->nodeType != YANG_NODE_ORIGINAL) return;
    /* resolve built-in type */
    _YangNode* curNode = nodePtr;
    while (curNode->typeInfo->baseTypeNodePtr != NULL) {
        curNode = findChildNodeByType(curNode->typeInfo->baseTypeNodePtr, YANG_DECL_TYPE);
    }
    nodePtr->typeInfo->builtinType = curNode->typeInfo->builtinType;

    /* Validate union subtypes.
       A member type can be of any built-in or derived type, except it MUST NOT be one of the built-in types "empty" or "leafref". */
    if (nodePtr->parentPtr->export.nodeKind == YANG_DECL_TYPE) {        
        if (nodePtr->typeInfo->builtinType == YANG_TYPE_EMPTY) {
            smiPrintErrorAtLine(currentParser, ERR_INVALID_UNION_TYPE, nodePtr->line, "empty");
        } else if (nodePtr->typeInfo->builtinType == YANG_TYPE_LEAFREF) {
            smiPrintErrorAtLine(currentParser, ERR_INVALID_UNION_TYPE, nodePtr->line, "leafref");
        }
    }

    if (!nodePtr->typeInfo->baseTypeNodePtr) {
        switch (nodePtr->typeInfo->builtinType) {
            case YANG_TYPE_ENUMERATION:
                if (!findChildNodeByType(nodePtr, YANG_DECL_ENUM)) {
                    smiPrintErrorAtLine(currentParser, ERR_CHILD_REQUIRED, nodePtr->line, "enumeration", "enum");
                }
                break;
            case YANG_TYPE_BITS:
                if (!findChildNodeByType(nodePtr, YANG_DECL_BIT)) {
                    smiPrintErrorAtLine(currentParser, ERR_CHILD_REQUIRED, nodePtr->line, "bits", "bit");
                }
                break;
            case YANG_TYPE_LEAFREF:
                if (!findChildNodeByType(nodePtr, YANG_DECL_PATH)) {
                    smiPrintErrorAtLine(currentParser, ERR_CHILD_REQUIRED, nodePtr->line, "leafref", "path");
                }
                break;
            case YANG_TYPE_IDENTITY:
                if (!findChildNodeByType(nodePtr, YANG_DECL_BASE)) {
                    smiPrintErrorAtLine(currentParser, ERR_CHILD_REQUIRED, nodePtr->line, "identityref", "base");
                }
                break;
            case YANG_TYPE_UNION:
                if (!findChildNodeByType(nodePtr, YANG_DECL_TYPE)) {
                    smiPrintErrorAtLine(currentParser, ERR_CHILD_REQUIRED, nodePtr->line, "union", "type");
                }
                break;
        }
    }

    curNode = nodePtr->firstChildPtr;
    while (curNode) {
        switch (curNode->export.nodeKind) {
            case YANG_DECL_RANGE:
                if (!isNumericalType(nodePtr->typeInfo->builtinType)) {
                   smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "range");
                }
                break;
            case YANG_DECL_LENGTH:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_STRING &&
                    nodePtr->typeInfo->builtinType != YANG_TYPE_BINARY) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "length");
                }
                break;
            case YANG_DECL_PATTERN:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_STRING) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "pattern");
                }
                break;
            case YANG_DECL_ENUM:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_ENUMERATION) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "enum");
                }
                break;
            case YANG_DECL_BIT:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_BITS) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "bit");
                }
                break;
            case YANG_DECL_PATH:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_LEAFREF) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "path");
                }
                break;
            case YANG_DECL_BASE:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_IDENTITY) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "base");
                }
                break;
            case YANG_DECL_TYPE:
                if (nodePtr->typeInfo->builtinType != YANG_TYPE_UNION) {
                    smiPrintErrorAtLine(currentParser, ERR_RESTRICTION_NOT_ALLOWED, curNode->line, "union");
                }
                break;
            default:
                break;
        }
        curNode = curNode->nextSiblingPtr;
    }
}


int isInList(int value, int* list) {
    int i;
    for (i = 1; i <= list[0]; i++) {
        if (value == list[i]) {
            return 1;
        }
    }
    return 0;
}

void _iterate(_YangNode *nodePtr, void* handler, int* nodeKindList) {
    if (isInList(nodePtr->export.nodeKind, nodeKindList)) {
        void (*handlerPtr)(_YangNode*);
        handlerPtr = handler;
        handlerPtr(nodePtr);
    }
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        _iterate(childPtr, handler, nodeKindList);
    }
}
/*
 * The last argument should be the YANG_DECL_UNKNOWN value
 */
void iterate(_YangNode *nodePtr, void* handler, ...) {
    va_list ap;
    va_start(ap, handler);

    int cnt = 0, value;
    while ((value = va_arg(ap, int)) != YANG_DECL_UNKNOWN) {
        cnt++;
    }
    va_start(ap, handler);
    int *nodeKindList = smiMalloc((cnt + 1) * sizeof(int));
    nodeKindList[0] = cnt;
    cnt = 0;
    while ((value = va_arg(ap, int)) != YANG_DECL_UNKNOWN) {
        cnt++;
        nodeKindList[cnt] = value;
    }
    va_end(ap);
    _iterate(nodePtr, handler, nodeKindList);
}

void semanticAnalysis(_YangNode *module) {
    getModuleInfo(module)->originalModule = copyModule(module);
    initMap();
    resolveReferences(module);
    expandGroupings(module);
    
    expangAugments(module);

    /*
     *  module - a pointer to the root node of the module;
     *  1      - current config value;
     *  0      - don't ignore config value (it may be used for 'input', 'output' or 'notification' nodes, for which the 'config' value should be ignored;
     */
    validateConfigProperties(module, 1, 0);
    
    validateDefaultStatements(module);

    validateLists(module);
    
    uniqueNames(module);

    uniqueSubmoduleDefinitions(module);

    /* validate type restrictions */
    iterate(module, typeHandler, YANG_DECL_TYPE, YANG_DECL_UNKNOWN);
}
