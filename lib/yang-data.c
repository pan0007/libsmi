/*
 * yang-data.c --
 *
 *      Operations on the main YANG data structures.
 *
 * Copyright (c) 1999-2002 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Authors: Kaloyan Kanev, Siarhei Kuryla
 * @(#) $Id: data.c 12198 2009-01-05 08:37:07Z schoenw $
 */

#include <config.h>

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WIN_H
#include "win.h"
#endif

#include "common.h"
#include "error.h"
#include "data.h"
#include "util.h"
#include "yang-data.h"
#include "yang.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/*
 * Current parser defined in parser-yang. Workaround - can't include data.h
 */
extern Parser *currentParser;

/*
 * YangNode fields setters
 */
void setConfig(_YangNode *nodePtr, YangConfig config)
{
    if (config == YANG_CONFIG_TRUE && nodePtr->export.config == YANG_CONFIG_FALSE) {
        smiPrintError(currentParser, ERR_INVALID_CONFIG_VALUE);
    } else {
        nodePtr->export.config = config;
    }
}

void setStatus(_YangNode *nodePtr, YangStatus status)
{
	nodePtr->export.status = status;
}

void setDescription(_YangNode *nodePtr, char *description)
{
    if (nodePtr->export.description)
        smiFree(nodePtr->export.description);
    nodePtr->export.description = smiStrdup(description);
}

void setReference(_YangNode *nodePtr, char *reference)
{
    if (nodePtr->export.reference)
        smiFree(nodePtr->export.reference);
    nodePtr->export.reference = smiStrdup(reference);
}


/*
 *----------------------------------------------------------------------
 *
 * findYangModuleByName --
 *
 *      Lookup a Yang module by a given name.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *	NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

_YangNode *yangFindModuleByName(const char *modulename)
{
    _YangNode	*modulePtr;

    for (modulePtr = smiHandle->firstYangModulePtr; modulePtr; modulePtr = modulePtr->nextSiblingPtr) {
        if ((modulePtr->export.value) && !strcmp(modulePtr->export.value, modulename)) {
            return (modulePtr);
        }
    }
    return (NULL);
}


 /*----------------------------------------------------------------------
 *
 * 
 * createModuleInfo --
 *
 *      Attach info to the module.
 *
 * Results:
 *      A pointer to the new _YangModuleInfo structure or
 *	NULL if terminated due to an error.
 *
 *----------------------------------------------------------------------
 */
_YangModuleInfo *createModuleInfo(_YangNode *modulePtr)
{
    if (!modulePtr) return NULL;
    
    _YangModuleInfo *infoPtr = smiMalloc(sizeof(_YangModuleInfo));
    modulePtr->info = infoPtr;
    
    infoPtr->namespace  = NULL;
    infoPtr->prefix        = NULL;
    infoPtr->version       = NULL;
    infoPtr->organization  = NULL;
    infoPtr->contact       = NULL;
    
    return (infoPtr);
}

_YangNode *addYangNode(char *value, YangDecl nodeKind, _YangNode *parentPtr)
{
	_YangNode *node = (_YangNode*) smiMalloc(sizeof(_YangNode));
	
	node->export.value          = smiStrdup(value);
	node->export.nodeKind       = nodeKind;
    node->export.description	= NULL;
    node->export.reference		= NULL;
    node->export.config         = YANG_CONFIG_TRUE;
    node->export.status         = YANG_STATUS_CURRENT;
    
    node->info                  = NULL;    
    
    node->nextSiblingPtr        = NULL;
    node->firstChildPtr         = NULL;
    node->lastChildPtr          = NULL;
    node->parentPtr             = parentPtr;

	if(parentPtr)
	{
        node->modulePtr         = parentPtr->modulePtr;
		//inherit Config value. This is changed later, if there is config statement, for this node
		node->export.config = parentPtr->export.config;
		
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
    } else {
        // it's a module node
        node->modulePtr         = node;
    }
    return node;
}

/*
 *----------------------------------------------------------------------
 *
 * loadYangModule --
 *
 *      Load a YANG module. If modulename is a plain name, the file is
 *	search along the SMIPATH environment variable. If modulename
 *	contains a `.' or DIR_SEPARATOR it is assumed to be the path.
 *
 *
 *----------------------------------------------------------------------
 */

_YangNode *loadYangModule(const char *modulename, Parser *parserPtr)
{
    Parser	    parser;
    Parser      *parentParserPtr;
    char	    *path = NULL;
    SmiLanguage lang = 0;
    FILE	    *file;

    path = getModulePath(modulename);
    
    if (!path) {
        smiPrintError(parserPtr, ERR_MODULE_NOT_FOUND, modulename);
        return NULL;
    }

    parser.path			= path;

    file = fopen(path, "r");
    if (! file) {
        smiPrintError(parserPtr, ERR_OPENING_INPUTFILE, path, strerror(errno));
        smiFree(path);
        fclose(file);        
        return NULL;
    }

    lang = getLanguage(file);

    if (lang != SMI_LANGUAGE_YANG) {
        smiPrintError(parserPtr, ERR_ILLEGAL_INPUTFILE, path);
        smiFree(path);
        fclose(file);
        return NULL;
    }

#ifdef BACKEND_YANG
	parentParserPtr = smiHandle->parserPtr;
	smiHandle->parserPtr = &parser;
    /* 
     *  Initialization of the parser;
     *  In YANG we don't use most of these fields of the Parser
     */
	parser.path			= path;
	parser.flags			= smiHandle->flags;
	parser.modulePtr		= NULL;
	parser.complianceModulePtr	= NULL;
	parser.capabilitiesModulePtr	= NULL;
	parser.currentDecl              = SMI_DECL_UNKNOWN;
	parser.firstStatementLine       = 0;
	parser.firstNestedStatementLine = 0;
	parser.firstRevisionLine        = 0;
	parser.file			= file;

	/*
	 * Initialize a root Node for pending (forward referenced) nodes.
	 */
	parser.pendingNodePtr = addNode(NULL, 0, NODE_FLAG_ROOT, NULL);
    
	if (yangEnterLexRecursion(parser.file) < 0) {
	    smiPrintError(&parser, ERR_MAX_LEX_DEPTH);
	    fclose(parser.file);
	}
	smiDepth++;
	parser.line			= 1;
	yangparse((void *)&parser);
	smiFree(parser.pendingNodePtr);
	yangLeaveLexRecursion();
	smiDepth--;
	fclose(parser.file);
	smiFree(path);
	smiHandle->parserPtr = parentParserPtr;
	return parser.yangModulePtr;
#else
	smiPrintError(parserPtr, ERR_YANG_NOT_SUPPORTED, path);
	smiFree(path);
    fclose(file);
	return NULL;
#endif

    
    
    smiFree(path);
    fclose(file);
    return NULL;
}