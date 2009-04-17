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
               kind == YANG_DECL_NOTIFICATION) {
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

void uniqueNames(_YangNode* nodePtr) {    
    /* if the current node is choise, then all cases are the same namespace and we have to handle it differently */
    if (nodePtr->export.nodeKind == YANG_DECL_CHOICE) {
        _YangNode* cur = nodePtr->firstChildPtr;
        while (cur) {
            if (cur->export.nodeKind == YANG_DECL_CASE) {
                _YangNode* caseChild = cur->firstChildPtr;
                for (; caseChild; caseChild = caseChild->nextSiblingPtr) {
                    YangIdentifierGroup yig = getIdentifierGroup(caseChild->export.nodeKind);
                    if (yig != YANG_IDGR_NONE) {
                        /* check whether this identifier has been defined before */
                        if (countChoiceChildNodesByTypeAndValue(nodePtr, caseChild, getIdentifierGroup(caseChild->export.nodeKind), caseChild->export.value)) {
                            smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_IDENTIFIER, caseChild->line, caseChild->export.value);
                        }
                    }
                    uniqueNames(caseChild);
                }
                cur = cur->nextSiblingPtr;
            } else {
                YangIdentifierGroup yig = getIdentifierGroup(cur->export.nodeKind);
                if (yig != YANG_IDGR_NONE) {
                    /* check whether this identifier has been defined before */
                    if (countChoiceChildNodesByTypeAndValue(nodePtr, cur, getIdentifierGroup(cur->export.nodeKind), cur->export.value)) {
                        smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_IDENTIFIER, cur->line, cur->export.value);
                    }
                }
                uniqueNames(cur);
                cur = cur->nextSiblingPtr;
            }
        }
    } else {        
    /* otherwise apply a regular processing  */
        /* go over all child nodes*/
        _YangNode* cur = nodePtr->firstChildPtr;
        while (cur) {            
            YangIdentifierGroup yig = getIdentifierGroup(cur->export.nodeKind);
            if (yig != YANG_IDGR_NONE) {
                /* check whether this identifier is contained in one of the submodules*/
                int isDuplicated = 0;
                if (nodePtr->export.nodeKind == YANG_DECL_MODULE || nodePtr->export.nodeKind == YANG_DECL_SUBMODULE) {
                    _YangNodeList* submodules = ((_YangModuleInfo*)nodePtr->info)->submodules;
                    while (submodules) {
                        if (countChildNodesByTypeAndValue(submodules->nodePtr, cur, getIdentifierGroup(cur->export.nodeKind), cur->export.value)) {
                            isDuplicated = 1;
                            smiPrintErrorAtLine(currentParser, ERR_SUBMODULE_DUPLICATED_IDENTIFIER, cur->line, cur->export.value, submodules->nodePtr->export.value);
                            break;
                        }                    
                        submodules = submodules->next;
                    }
                }                
                /* check whether this identifier has been defined before */
                if (!isDuplicated && countChildNodesByTypeAndValue(nodePtr, cur, getIdentifierGroup(cur->export.nodeKind), cur->export.value)) {
                    smiPrintErrorAtLine(currentParser, ERR_DUPLICATED_IDENTIFIER, cur->line, cur->export.value);
                }
            }
            uniqueNames(cur);
            cur = cur->nextSiblingPtr;
        }
    }
}


void semanticAnalysis(_YangNode *module) {
    uniqueNames(module);
}
