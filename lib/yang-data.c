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

#include "smi.h"
#include <string.h>

#include "yang.h"
#include "smi.h"
#include "smi.h"

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
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
#include "parser-yang.tab.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/*
 * Current parser defined in parser-yang. Workaround - can't include data.h
 */
extern Parser *currentParser;

const int builtInTypeCount = 20;

const char* yangBuiltInTypeNames[] = {  "binary",
                                        "bits",
                                        "boolean",
                                        "empty",
                                        "enumeration",
                                        "float32",
                                        "float64",
                                        "identityref",
                                        "instance-identifier",
                                        "int8",
                                        "int16",
                                        "int32",
                                        "int64",
                                        "leafref",
                                        "string",
                                        "uint8",
                                        "uint16",
                                        "uint32",
                                        "uint64",
                                        "union"
};


YangBuiltInTypes getBuiltInTypeName(char *name) {
    int i;
    for (i = 0; i <  builtInTypeCount; i++) {
        if (!strcmp(yangBuiltInTypeNames[i], name)) {
            return i;
        }
    }
    return YANG_TYPE_NONE;
}
/*
 * YangNode fields setters
 */
void setConfig(_YangNode *nodePtr, YangConfig config)
{
    if (config == YANG_CONFIG_TRUE && nodePtr->export.config == YANG_CONFIG_DEFAULT_FALSE) {
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
 * Uniqueness checks
 */
void uniqueConfig(_YangNode *nodePtr)
{
    if (nodePtr->export.config == YANG_CONFIG_DEFAULT_FALSE || nodePtr->export.config == YANG_CONFIG_DEFAULT_TRUE) return;
    smiPrintError(currentParser, ERR_REDEFINED_ELEMENT, "config");    
}

void uniqueStatus(_YangNode *nodePtr)
{
    if (nodePtr->export.status != YANG_STATUS_DEFAULT_CURRENT) {
        smiPrintError(currentParser, ERR_REDEFINED_ELEMENT, "status");
    }
}

void uniqueDescription(_YangNode *nodePtr) 
{
    if(nodePtr->export.description) {
        smiPrintError(currentParser, ERR_REDEFINED_DESCRIPTION, NULL);
    }
}

void uniqueReference(_YangNode *nodePtr) 
{
    if(nodePtr->export.reference) {
        smiPrintError(currentParser, ERR_REDEFINED_REFERENCE, NULL);
    }
}

void uniqueNodeKind(_YangNode *nodePtr, YangDecl nodeKind) 
{
    if (findChildNodeByType(nodePtr, nodeKind)) {
        smiPrintError(currentParser, ERR_REDEFINED_ELEMENT, yandDeclKeyword[nodeKind]);
    }
}


void presenceNodeKind(_YangNode *nodePtr, YangDecl nodeKind) 
{
    if (!findChildNodeByType(nodePtr, nodeKind)) {
        smiPrintError(currentParser, ERR_REQUIRED_ELEMENT, yandDeclKeyword[nodeKind]);
    }
}

int getCardinality(_YangNode *nodePtr, YangDecl nodeKind) 
{
    _YangNode *childPtr = NULL;
    int ret = 0;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        if (childPtr->export.nodeKind == nodeKind) {
            ret++;
        }
    }
    return ret;
}

/* ----------------------------------------------------------------------
 *
 *  Utils
 *
 * ----------------------------------------------------------------------
 */
_YangModuleInfo* getModuleInfo(_YangNode* module) {
    return (_YangModuleInfo*)module->info;
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
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */


_YangNode *findYangModuleByName(const char *modulename)
{
    _YangNode	*modulePtr;
    
    for (modulePtr = smiHandle->firstYangModulePtr; modulePtr; modulePtr = modulePtr->nextSiblingPtr) {
        if ((modulePtr->export.value) && !strcmp(modulePtr->export.value, modulename)) {
            return (modulePtr);
        }
    }
    return (NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * findYangModuleByPrefix --
 *
 *      Lookup a Yang module by a given prefix.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */


_YangNode *findYangModuleByPrefix(_YangNode *module, const char *prefix)
{
    _YangImportList *imports = getModuleInfo(module)->imports;    
    for (; imports; imports = imports->next) {
        if (!strcmp(imports->prefix, prefix)) {
            return imports->modulePtr;
        }
    }
    return (NULL);
}
/*
 *----------------------------------------------------------------------
 *
 * findChildNodeByType --
 *
 *      Lookup a child node by a given type.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
_YangNode* findChildNodeByType(_YangNode *nodePtr, YangDecl nodeKind) {
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        if (childPtr->export.nodeKind == nodeKind) {
            return childPtr;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * findChildNodeByTypeAndValue --
 *
 *      Lookup a child node by a given type and value.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
_YangNode* findChildNodeByTypeAndValue(_YangNode *nodePtr, YangDecl nodeKind, char* value) {
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        if (childPtr->export.nodeKind == nodeKind && !strcmp(childPtr->export.value, value)) {
            return childPtr;
        }
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * resolveNodeByTypeAndValue
 *
 *      Resolve a node by a given type.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
_YangNode* resolveNodeByTypeAndValue(_YangNode *nodePtr, YangDecl nodeKind, char* value, int depth) {
    if (depth < 0) return NULL;
    _YangNode *childPtr = NULL;
    for (childPtr = nodePtr->firstChildPtr; childPtr; childPtr = childPtr->nextSiblingPtr) {
        if (childPtr->export.nodeKind == nodeKind && !strcmp(childPtr->export.value, value)) {
            return childPtr;
        }
    }
    if (nodePtr->parentPtr) {
        _YangNode *ret = resolveNodeByTypeAndValue(nodePtr->parentPtr, nodeKind, value, depth);
        if (ret) return ret;
    } else {
        _YangNodeList *submodules = getModuleInfo(nodePtr)->submodules;
        for (; submodules; submodules = submodules->next) {            
            _YangNode *ret = resolveNodeByTypeAndValue(submodules->nodePtr, nodeKind, value, depth - 1);
            if (ret) return ret;
        }        
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * resolveReference
 *
 *      Resolve a reference by a given type.
 *
 * Results:
 *      A pointer to the _YangNode structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
_YangNode* resolveReference(_YangNode *currentNodePtr, YangDecl nodeKind, char* prefix, char* identifierName) {
    if (prefix && strcmp(getModuleInfo(currentNodePtr->modulePtr)->prefix, prefix)) {
        _YangNode *modulePtr = findYangModuleByPrefix(currentNodePtr->modulePtr, prefix);
        if (!modulePtr) return NULL;
        return resolveNodeByTypeAndValue(modulePtr, nodeKind, identifierName, 1);
    } else {
        return resolveNodeByTypeAndValue(currentNodePtr, nodeKind, identifierName, 1);
    }
    return NULL;
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
    
    infoPtr->namespace     = NULL;
    infoPtr->prefix        = NULL;
    infoPtr->version       = NULL;
    infoPtr->organization  = NULL;
    infoPtr->contact       = NULL;
    infoPtr->parsingState  = YANG_PARSING_IN_PROGRESS;
    infoPtr->submodules    = NULL;
    // create a corresponding Module wrapper to maintain interface compatibility
    Module *module = addModule(smiStrdup(modulePtr->export.value), smiStrdup(currentParser->path), 0, currentParser);
    currentParser->modulePtr = module;
    return (infoPtr);
}

void createIdentifierRef(_YangNode *node, char* prefix, char* ident) {
    _YangIdentifierRefInfo *infoPtr = smiMalloc(sizeof(_YangIdentifierRefInfo));
    
    if (prefix) {
        infoPtr->prefix = prefix;
    } else {
        infoPtr->prefix = smiStrdup(getModuleInfo(node->modulePtr)->prefix);
    }
    infoPtr->identifierName = ident;
    infoPtr->resolvedNode = NULL;
    infoPtr->met = NULL;
    
    node->info = infoPtr;   
}

_YangTypeInfo createTypeInfo(_YangNode *node) {
    _YangTypeInfo *infoPtr = smiMalloc(sizeof(_YangTypeInfo));
    
    infoPtr->baseTypeNodePtr    = NULL;
    node->typeInfo              = infoPtr;   
}

_YangNode *addYangNode(const char *value, YangDecl nodeKind, _YangNode *parentPtr)
{
	_YangNode *node = (_YangNode*) smiMalloc(sizeof(_YangNode));
    node->nodeType              = YANG_NODE_ORIGINAL;
	node->export.value          = smiStrdup(value);
	node->export.nodeKind       = nodeKind;
    node->export.description	= NULL;
    node->export.reference		= NULL;
    node->export.extra  		= NULL;
    node->export.config         = YANG_CONFIG_DEFAULT_TRUE;
    node->export.status         = YANG_STATUS_DEFAULT_CURRENT;
    node->line                  = currentParser->line;
    
    node->info                  = NULL;    
    node->typeInfo              = NULL;
    
    node->nextSiblingPtr        = NULL;
    node->firstChildPtr         = NULL;
    node->lastChildPtr          = NULL;
    node->parentPtr             = parentPtr;

	if(parentPtr)
	{
        node->modulePtr         = parentPtr->modulePtr;
		//inherit Config value. This is changed later, if there is config statement, for this node
        if (yangIsTrueConf(parentPtr->export.config)) {
            node->export.config = YANG_CONFIG_DEFAULT_TRUE;
        } else {
            node->export.config = YANG_CONFIG_DEFAULT_FALSE;
        }
		
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

    /* module can not be located, the last try */
    if (!path && parserPtr && parserPtr->path) {
        /*  searching for the module at the path where the previous module was found
            it's used by the imported modules */
        int slashIndex = -1;
        int i = 0;
        for (i = strlen(parserPtr->path) - 1; i + 1; i--)
            if (parserPtr->path[i] == DIR_SEPARATOR) {
                slashIndex = i;
                break;
            }
        if (slashIndex == -1) {
            smiAsprintf(&path, "%s%s", modulename, ".yang");
        } else {
            char *dir = (char*) smiMalloc(slashIndex + 2);
            dir[slashIndex + 1] = 0;
            strncpy(dir, parserPtr->path, slashIndex + 1);

            smiAsprintf(&path, "%s%s%s", dir, modulename, ".yang");
            
            // TODO: implement path extraction and construction 
            smiFree(dir);
        }
    }
    
    if (!path) {
        smiPrintError(parserPtr, ERR_MODULE_NOT_FOUND, modulename);
        return NULL;
    }

    parser.path			= path;
    file = fopen(path, "r");
    if (! file) {
        smiPrintError(parserPtr, ERR_OPENING_INPUTFILE, path, strerror(errno));
        smiFree(path);
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
    if (parser.yangModulePtr) {
        ((_YangModuleInfo*)(parser.yangModulePtr->info))->conformance = parser.modulePtr->export.conformance;
    }
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

/*
 *----------------------------------------------------------------------
 *
 * addSubmodule --
 *
 *      Add an included submodule to the module or submodule
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void addSubmodule(_YangNode *module, _YangNode *submodule) {
    _YangNodeList* cur = ((_YangModuleInfo*)module->info)->submodules;
    while (cur) {
        if (cur->nodePtr == submodule) return;
        cur = cur->next;
    }
    _YangNodeList *nodeListPtr = smiMalloc(sizeof(_YangNodeList));
    nodeListPtr->nodePtr = submodule;
    nodeListPtr->next = ((_YangModuleInfo*)module->info)->submodules;
    ((_YangModuleInfo*)module->info)->submodules = nodeListPtr;
    
    /* go through all child submodules included by the current submodule and add them to the module as well */
    cur = ((_YangModuleInfo*)submodule->info)->submodules;
    while (cur) {
        addSubmodule(module, cur->nodePtr);
        cur = cur->next;
    }    
}

/*
 *----------------------------------------------------------------------
 *
 * addImportedModule --
 *
 *      Add an imported module to the module or submodule
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void addImportedModule(_YangNode *importNode, _YangNode *importedModule) {
    char* prefix = getModuleInfo(importNode->modulePtr)->prefix;
    char* importPrefix = findChildNodeByType(importNode, YANG_DECL_PREFIX)->export.value;

    if (!strcmp(prefix, importPrefix)) {
        smiPrintError(currentParser, ERR_DUPLICATED_PREFIX, importPrefix);
    }
    
    _YangImportList* cur = ((_YangModuleInfo*)importNode->modulePtr->info)->imports;
    while (cur) {
        if (!strcmp(cur->prefix, importPrefix)) {
            smiPrintError(currentParser, ERR_DUPLICATED_PREFIX, importPrefix);
        }
        cur = cur->next;
    }

    _YangImportList *importPtr = smiMalloc(sizeof(_YangImportList));
    importPtr->prefix = importPrefix;
    importPtr->modulePtr = importedModule;
    importPtr->next = ((_YangModuleInfo*)importNode->modulePtr->info)->imports;
    ((_YangModuleInfo*)importNode->modulePtr->info)->imports = importPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * externalModule --
 *
 *      Lookup a YANG Module or Submodule by a given name.
 *
 * Results:
 *      A pointer to the _YangModule structure or
 *      NULL if it is not found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
_YangNode *externalModule(_YangNode *importNode) {
    _YangNode *importedModule = findYangModuleByName(importNode->export.value);

    if (importedModule && ((_YangModuleInfo*)importedModule->info)->parsingState == YANG_PARSING_IN_PROGRESS) {
        smiPrintError(currentParser, ERR_CYCLIC_IMPORTS, importNode->modulePtr->export.value, importedModule->export.value);
    }
    
    if(!importedModule) {
        Parser* tempParser = currentParser;
        importedModule = loadYangModule(importNode->export.value, currentParser);
        currentParser = tempParser;
    }
  
    if(importedModule && !strcmp(importNode->export.value, importedModule->export.value)) {		
        if (importNode->export.nodeKind == YANG_DECL_INCLUDE) {
            addSubmodule(importNode->modulePtr, importedModule);
        } else if (importNode->export.nodeKind == YANG_DECL_IMPORT) {
            addImportedModule(importNode, importedModule);
        }
        return importedModule;
    } else {
        smiPrintError(currentParser, ERR_IMPORT_NOT_FOUND, importNode->export.value);
    }    
    return NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * freeYangNode --
 *
 *      Free a YANG module tree.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void freeYangNode(_YangNode *nodePtr) {
    if (!nodePtr) return;

    /* free only original node's memory, because references hold only pointers to other node's fields */
    if (nodePtr->nodeType == YANG_NODE_ORIGINAL) {
        smiFree(nodePtr->export.value);
        nodePtr->export.value = NULL;
        smiFree(nodePtr->export.extra);
        nodePtr->export.extra = NULL;    
        nodePtr->export.description = NULL;
        nodePtr->export.reference = NULL;
        YangDecl nodeKind = nodePtr->export.nodeKind;

        if (nodeKind == YANG_DECL_MODULE || nodeKind == YANG_DECL_SUBMODULE) {
            _YangNodeList *submodules = getModuleInfo(nodePtr)->submodules;
            while (submodules) {
                _YangNodeList *next = submodules->next;
                smiFree(submodules);
                submodules = next;
            }
            _YangImportList *imports = getModuleInfo(nodePtr)->imports;
            while (imports) {
                _YangImportList *next = imports->next;
                smiFree(imports);
                imports = next;
            }        
        }

        if (nodeKind == YANG_DECL_UNKNOWN_STATEMENT ||
            nodeKind == YANG_DECL_IF_FEATURE ||
            nodeKind == YANG_DECL_TYPE ||
            nodeKind == YANG_DECL_USES || 
            nodeKind == YANG_DECL_BASE) {
                _YangIdentifierRefInfo *info = (_YangIdentifierRefInfo*)nodePtr->info;
                if (info) {
                    smiFree(info->identifierName);
                    smiFree(info->prefix);
                }
        }
        
        if (nodeKind == YANG_DECL_TYPE) {
            smiFree(nodePtr->typeInfo);
        }
        
        smiFree(nodePtr->info);
        nodePtr->info = NULL;
    }

    _YangNode *currentNode= nodePtr->firstChildPtr, *nextNode;
    while (currentNode) {
        nextNode = currentNode->nextSiblingPtr;
        freeYangNode(currentNode);
        currentNode = nextNode;
    }

    smiFree(nodePtr);
    nodePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * yangFreeData --
 *
 *      Free YANG all data structures.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void yangFreeData() {
    _YangNode	*modulePtr;
    for (modulePtr = smiHandle->firstYangModulePtr; modulePtr; modulePtr = modulePtr->nextSiblingPtr) {
        freeYangNode(modulePtr);
    }    
}

int isDataDefNode(_YangNode* nodePtr) {
    YangDecl kind = nodePtr->export.nodeKind;
    return kind == YANG_DECL_CONTAINER ||
           kind == YANG_DECL_LEAF ||
           kind == YANG_DECL_LEAF_LIST ||
           kind == YANG_DECL_LIST ||
           kind == YANG_DECL_CHOICE ||
           kind == YANG_DECL_ANYXML ||
           kind == YANG_DECL_USES;
    
}

 /*
  * Schema Node Identifiers 
  */

int isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

int isDigit(char ch) {
    return (ch >= '0' && ch <= '9');
}

int buildIdentifier(char *s) {
    if (!s || strlen(s) == 0) return 0;
    if (!isAlpha(s[0]) && s[0] != '_') return 0;
    int ret = 1;
    while (ret < strlen(s) && (isAlpha(s[ret]) || isDigit(s[ret]) || s[ret] == '_' || s[ret] == '-' || s[ret] == '.')) {
        ret++;
    }
    return ret;
}

int nodeIdentifier(char* s) {
    if (!s || strlen(s) == 0) return 0;
    int ret = buildIdentifier(s);
    if (!ret || ret == strlen(s)) return ret;
    if (s[ret] == ':') {
        int ret1 = buildIdentifier(s + ret + 1);
        if (ret1) return ret + ret1 + 1;
    }
    return ret;
}

int absoluteSchemaNodeid(char *s) {
    if (!s || strlen(s) == 0) return 0;
    int ret = 0;    
    while (ret < strlen(s)) {
        if (s[ret] != '/') return ret;        
        int cur = nodeIdentifier(s + ret + 1);
        if (cur > 0) {
            ret += cur + 1;
        } else {
            return ret;
        }       
    }
    return ret;
}

int descendantSchemaNodeid(char *s) {
    if (!s || strlen(s) == 0) return 0;
    int ret = nodeIdentifier(s);
    if (!ret) return 0;
    return ret + absoluteSchemaNodeid(s + ret);
}

int isAbsoluteSchemaNodeid(char *s) {
    if (!s || strlen(s) == 0) return 0;
    return (absoluteSchemaNodeid(s) == strlen(s));
}

int isDescendantSchemaNodeid(char *s) {
    if (!s || strlen(s) == 0) return 0;
    return (descendantSchemaNodeid(s) == strlen(s));
}

_YangXPathList *getXPathNode(char* s) {
    int i = 0;
    _YangXPathList *ret = NULL, *prev = NULL;
    if (s[0] == '/') i = 1;
    
    while (i < strlen(s)) {
        int i1 = buildIdentifier(s + i);
        int i2 = 0;
        _YangXPathList *cur = smiMalloc(sizeof(_YangXPathList));
        cur->next = NULL;
        cur->prefix = NULL;
        if (s[i + i1] == ':') {
            i2 = buildIdentifier(s + i + i1 + 1);
            cur->prefix = smiStrndup(s + i, i1);
            cur->ident  = smiStrndup(s + i + i1 + 1, i2);
            i2 += 1;
        } else {
            cur->ident  = smiStrndup(s + i, i1);
        }
        i += i1 + i2 + 1;
        
        if (ret == NULL) {
            ret = cur;
            prev = cur;
        } else {
            prev->next = cur;
            prev = cur;
        }
/*        if (cur->prefix) {
            printf("%s %s\n", cur->ident, cur->prefix);
        } else {
            printf("%s\n", cur->ident);
        }*/
    }
    return ret;    
}