/*
 * parser-yang.y --
 *
 *      Syntax rules for parsing the YANG MIB module language.
 *
 * Copyright (c) 1999 Frank Strauss, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  Authors: Kaloyan Kanev, Siarhei Kuryla
 */

%{

#include <config.h>
    
#ifdef BACKEND_YANG

#define _ISOC99_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <float.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#include "yang.h"
#include "yang-data.h"
#include "parser-yang.h"
#include "scanner-yang.h"
#include "util.h"
#include "error.h"
#include "errormacros.h"
    
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


/*
 * These arguments are passed to yyparse() and yylex().
 */
#define YYPARSE_PARAM parserPtr
#define YYLEX_PARAM   parserPtr
 
    
#define thisParserPtr      ((Parser *)parserPtr)
#define thisModulePtr     (((Parser *)parserPtr)->yangModulePtr)


#define DEBUG

#ifdef DEBUG 
#define debug(args...) fprintf (stderr, args);
#else
#define debug
#endif

    
/*
 * NOTE: The argument lvalp ist not really a void pointer. Unfortunately,
 * we don't know it better at this point. bison generated C code declares
 * YYSTYPE just a few lines below based on the `%union' declaration.
 */
extern int yylex(void *lvalp, Parser *parserPtr);

/* The declaration stack is used to determine what is the parrent
 * statement of the current one. It helps to write generic
 * substatements like the description statement which calls the
 * function setDescription, which in turn checks the current
 * statement type(decl) and uses the a global pointer to the 
 * current parent statement to set the description. 
 */
typedef struct declStack {
	YangDecl decl;
    _YangNode* node;
	struct declStack *down;
} declStack;

static declStack *dStack = NULL;

static void pushDecl(YangDecl decl)
{
	declStack *top = (declStack*)smiMalloc(sizeof(declStack));
	top->down = NULL;
	top->decl = decl;
	
	if(dStack == NULL) dStack = top;
	else
	{
		top->down = dStack;
		dStack = top;
	}
}

static void pushNode(_YangNode* node)
{
	declStack *top = (declStack*)smiMalloc(sizeof(declStack));
	top->down = NULL;
	top->node = node;
    if (node) {
        top->decl = node->export.nodeKind;
    }
	
	if(dStack == NULL) dStack = top;
	else
	{
		top->down = dStack;
		dStack = top;
	}
}

static void pop()
{
	declStack *top;	
	if(dStack != NULL)
	{
		top = dStack;
		dStack = dStack->down;
		free(top);
	}
}

static YangDecl topDecl()
{
	if (dStack == NULL)
	{
		return YANG_DECL_UNKNOWN;
	}
	else
	{
		return dStack->decl;
	}
}

static _YangNode* topNode()
{
	if (dStack == NULL)
	{
		return NULL;
	}
	else
	{
		return dStack->node;
	}
}

Parser *currentParser = NULL;
static _YangNode *currentModule = NULL;
static _YangModuleInfo *currentModuleInfo = NULL;

static _YangNode *node = NULL;


/*static Type *findType(char* spec)
{
	Type *typePtr = NULL;
	Module *modulePtr;
	char *prefix, *type;
	YangNode* n = NULL;
	
	if(strstr(spec,":") != NULL)
	{
		prefix = strtok(spec, ":");
		type = strtok(NULL , ":");
		modulePtr = findImportedModuleByPrefix(currentModule, prefix);
		if(modulePtr == NULL)
		{
			//TODO print unknow prefix error
			return NULL;
		}
		n = findYangNodeByScopeAndName(modulePtr, type, NULL);
	}
	else
	{
		n = findYangNodeByScopeAndName(currentModule, spec, currentNode);
	}
	
	if(n == NULL) return NULL;
	if(n->export.nodeKind == YANG_DECL_TYPEDEF)
	{
		typePtr = n->type;
	}
        return typePtr;
}*/

/*static void
checkTypes(Parser *parserPtr, Module *modulePtr)
{
    Type *typePtr;
    
    for(typePtr = modulePtr->firstTypePtr;
	typePtr; typePtr = typePtr->nextPtr) {
*/
	/*
	 * Complain about empty description clauses.
	 */

/*	if (! parserPtr->flags & SMI_FLAG_NODESCR
	    && (! typePtr->export.description
		|| ! typePtr->export.description[0])) {
	    smiPrintErrorAtLine(parserPtr, ERR_EMPTY_DESCRIPTION,
				typePtr->line, typePtr->export.name);
	}
	
	smiCheckNamedNumberRedefinition(parserPtr, typePtr);
	smiCheckNamedNumberSubtyping(parserPtr, typePtr);
    }
}*/

/*
void checkUniqueAndKey()
{
	YangNode *node;
	YangNodeList *list;
	for(list = currentNode->uniqueList; list; list = list->next)
	{
		for(node = currentNode->firstChildPtr; node; node = node->nextSiblingPtr)
		{
			if(!strcmp(list->name,node->export.name))
			{
				list->yangNode = node;
				smiFree(list->name);
				break;
			}
		}
		if(!list->yangNode)
			smiPrintError(currentParser,ERR_NO_SUCH_UNIQUE_LEAF,
					      			list->name,currentNode->export.name);		
	}
	
	
	
	for(list = currentNode->keyList; list; list = list->next)
	{
		for(node = currentNode->firstChildPtr; node; node = node->nextSiblingPtr)
		{
			if(!strcmp(list->name,node->export.name))
			{
				list->yangNode = node;
				smiFree(list->name);
				break;
			}
		}
		if(!list->yangNode)
			smiPrintError(currentParser,ERR_NO_SUCH_KEY_LEAF,
					      			list->name,currentNode->export.name);		
	}
	return;
}*/

/*
void setType(SmiBasetype basetype, Type *parent, char *parentName)
{
	switch( topDecl() )
	{
		case YANG_DECL_TYPEDEF:
			if(basetype == SMI_BASETYPE_UNKNOWN) //type derived from non-base type
			{
				if(parent) //known parent - put reference
				{
					//for typedefs type name is the same as node name
					char *typeName = smiStrdup(currentNode->export.name);
					currentType = createType(typeName); //add type with this name
					setTypeDecl(currentType, YANG_DECL_TYPEDEF);
					setTypeParent(currentType, parent);
					setTypeBasetype(currentType, parent->export.basetype);
				}
				else //forward reference - put empty type with parent's name
				{
					char *typeName = smiStrdup(parentName);
					currentType = createType(typeName);
                                        setTypeDecl(currentType, YANG_DECL_TYPEDEF);
				}
			}
			else //New type derived from base type
			{
				
				//for typedefs type name is the same as node name
			        currentType = createType(currentNode->export.name);

				setTypeBasetype(currentType, basetype);
				setTypeParent(currentType, parent);
			}
		        	
			break;
		case YANG_DECL_LEAF_LIST:
		case YANG_DECL_LEAF:
			if(basetype == SMI_BASETYPE_UNKNOWN) //type derived from non-base type
			{
				if(parent) //known parent - put reference
				{
					char *typeName = smiStrdup(parentName);
					currentType = createType(typeName); //add type with this name
					setTypeDecl(currentType, YANG_DECL_TYPEDEF);
					setTypeParent(currentType,parent);
					setTypeBasetype(currentType,parent->export.basetype);
				}
				else //forward reference - put empty type with parent's name
				{
					char *typeName = smiStrdup(parentName);
					currentType = createType(typeName); //add type with this name
				//IMPORTANT: After parsing is finished the dummy types are changed with
				// references to their parent. Dummies are distinguished by the fact that
				// they have no parent (parentPtr = NULL)
				}
			}
			else //New type derived from base type
			{
		
				currentType = createType(parentName);		

				setTypeBasetype(currentType, basetype);
				setTypeParent(currentType, parent);
			}
				
			break;
		default:
			//TODO print error
			printf("DEBUGG: OOPS wrong err decl %d, at line %d\n",topDecl(),currentParser->line);
			break;
	}
}*/

%}

/*
 * The grammars start symbol.
 */
%start yangFile

/*
 * We call the parser from within the parser when IMPORTing modules,
 * hence we need reentrant parser code. This is a bison feature.
 */
%pure_parser

/*
 * The attributes.
 */
%union {
    char            *id;				/* identifier name           */
    int             rc;                 /* >=0: ok, <0: error    */
    char            *text;
}

/*
 * Tokens and their attributes.
 */
%token <rc>augmentKeyword
%token <rc>belongs_toKeyword
%token <rc>choiceKeyword
%token <rc>configKeyword
%token <rc>contactKeyword
%token <rc>containerKeyword
%token <rc>defaultKeyword
%token <rc>descriptionKeyword
%token <rc>enumKeyword
%token <rc>error_app_tagKeyword
%token <rc>error_messageKeyword
%token <rc>extensionKeyword
%token <rc>groupingKeyword
%token <rc>importKeyword
%token <rc>includeKeyword
%token <rc>keyKeyword
%token <rc>leafKeyword
%token <rc>leaf_listKeyword
%token <rc>lengthKeyword
%token <rc>listKeyword
%token <rc>mandatoryKeyword
%token <rc>max_elementsKeyword
%token <rc>min_elementsKeyword
%token <rc>moduleKeyword
%token <rc>submoduleKeyword
%token <rc>mustKeyword
%token <rc>namespaceKeyword
%token <rc>ordered_byKeyword
%token <rc>organizationKeyword
%token <rc>prefixKeyword
%token <rc>rangeKeyword
%token <rc>referenceKeyword
%token <rc>patternKeyword
%token <rc>revisionKeyword
%token <rc>statusKeyword
%token <rc>typeKeyword
%token <rc>typedefKeyword
%token <rc>uniqueKeyword
%token <rc>unitsKeyword
%token <rc>usesKeyword
%token <rc>valueKeyword
%token <rc>whenKeyword
%token <rc>bitKeyword
%token <rc>pathKeyword
%token <rc>int8Keyword
%token <rc>int16Keyword
%token <rc>int32Keyword
%token <rc>int64Keyword
%token <rc>uint8Keyword
%token <rc>uint16Keyword
%token <rc>uint32Keyword
%token <rc>uint64Keyword
%token <rc>float32Keyword
%token <rc>float64Keyword
%token <rc>stringKeyword
%token <rc>booleanKeyword
%token <rc>enumerationKeyword
%token <rc>bitsKeyword
%token <rc>binaryKeyword
%token <rc>leafrefKeyword
%token <rc>emptyKeyword
%token <rc>anyXMLKeyword
%token <rc>deprecatedKeyword
%token <rc>currentKeyword
%token <rc>obsoleteKeyword
%token <rc>trueKeyword
%token <rc>falseKeyword
%token <rc>caseKeyword
%token <rc>inputKeyword
%token <rc>outputKeyword
%token <rc>rpcKeyword
%token <rc>notificationKeyword
%token <rc>argumentKeyword
%token <rc>yangversionKeyword
%token <rc>baseKeyword
%token <rc>deviationKeyword
%token <rc>deviateKeyword
%token <rc>featureKeyword
%token <rc>identityKeyword
%token <rc>ifFeatureKeyword
%token <rc>positionKeyword
%token <rc>presenceKeyword
%token <rc>refineKeyword
%token <rc>requireInstanceKeyword
%token <rc>yinElementKeyword


%token <text>identifier
%token <text>identifierRefArgStr
%token <text>dateString
%token <text>yangVersion
%token <text>qString
%token <text>uqString
%token <text>decimalNumber
%token <text>floatNumber
%token <text>hexNumber

/*
 * Types of non-terminal symbols.
 */

%type <rc>yangFile
%type <rc>moduleStatement
%type <rc>submoduleStatement
%type <rc>commonStatement
%type <rc>dataDefStatement
%type <rc>extensionStatement
%type <rc>extensionStatementBody
%type <rc>extensionSubstatement
%type <rc>extensionSubstatement_0n
%type <rc>argumentStatement
%type <rc>argumentStatementBody
%type <rc>linkageStatement
%type <rc>linkageStatement_0n
%type <rc>moduleMetaStatement
%type <rc>moduleMetaStatement_0n
%type <rc>moduleHeaderStatement
%type <rc>moduleHeaderStatement_0n
%type <rc>bodyStatement
%type <rc>bodyStatement_0n
%type <rc>containerStatement
%type <rc>containerSubstatement
%type <rc>containerSubstatement_0n
%type <rc>mustStatement
%type <rc>mustSubstatement
%type <rc>mustSubstatement_0n
%type <rc>organizationStatement
%type <rc>contactStatement
%type <rc>referenceStatement
%type <rc>descriptionStatement
%type <rc>revisionStatement
%type <rc>revisionStatement_0n
%type <rc>revisionDescriptionStatement
%type <rc>importStatement
%type <rc>includeStatement
%type <rc>includeStatementBody
%type <rc>prefixStatement
%type <rc>typedefStatement
%type <rc>typedefSubstatement
%type <rc>typedefSubstatement_0n
%type <rc>typeStatement
%type <rc>type
%type <rc>typeBody
%type <rc>refinedType
%type <rc>refinedBasetype
%type <text>numRestriction
%type <rc>stringRestriction
%type <rc>restriction
%type <rc>range
%type <rc>length
%type <text>date
%type <text>string
%type <text>prefix
%type <text>identifierRef
%type <rc>enumSpec
%type <rc>enum
%type <rc>enum_0n
%type <rc>stmtEnd
%type <rc>unknownStatement0_n
%type <rc>unknownStatement
%type <rc>error_app_tag
%type <rc>error_message
%type <rc>err
%type <rc>optError
%type <rc>path
%type <rc>pattern
%type <rc>status
%type <rc>statusStatement
%type <rc>configStatement
%type <rc>defaultStatement
%type <rc>leafStatement
%type <rc>leafSubstatement
%type <rc>leafSubstatement_0n
%type <rc>leaf_listStatement
%type <rc>leaf_listSubstatement
%type <rc>leaf_listSubstatement_0n
%type <rc>listStatement
%type <rc>listSubstatement
%type <rc>listSubstatement_0n
%type <rc>min_elementsStatement
%type <rc>max_elementsStatement
%type <rc>ordered_byStatement
%type <rc>keyStatement
%type <rc>uniqueStatement
%type <rc>choiceStatement
%type <rc>choiceSubstatement
%type <rc>choiceSubstatement_0n
%type <rc>caseStatement
%type <rc>caseSubstatement
%type <rc>caseSubstatement_0n
%type <rc>groupingStatement
%type <rc>groupingSubstatement
%type <rc>groupingSubstatement_0n
%type <rc>usesStatement
%type <rc>usesSubstatement
%type <rc>usesSubstatement_0n
%type <rc>augmentStatement
%type <rc>augmentSubstatement
%type <rc>augmentSubstatement_0n
%type <rc>whenStatement
%type <rc>rpcStatement
%type <rc>rpcSubstatement
%type <rc>rpcSubstatement_0n
%type <rc>inputStatement
%type <rc>inputSubstatement
%type <rc>inputSubstatement_0n
%type <rc>outputStatement
%type <rc>outputSubstatement
%type <rc>outputSubstatement_0n
%type <rc>notificationStatement
%type <rc>notificationSubstatement
%type <rc>notificationSubstatement_0n
%type <rc>anyXMLStatement
%type <rc>anyXMLSubstatement
%type <rc>anyXMLSubstatement_0n


%%

/*
 * Yacc rules.
 *
 */


/*
 * One mibFile may contain multiple MIB modules.
 * It's also possible that there's no module in a file.
 */
yangFile:		moduleStatement
			{
			    /*
			     * Return the number of successfully
			     * parsed modules.
			     */
			    $$ = $1;
			}
	|
			submoduleStatement
			{
			    /*
			     * Return the number of successfully
			     * parsed modules.
			     */
			    $$ = $1;
			}
	;

moduleStatement:	moduleKeyword identifier
			{
                thisParserPtr->yangModulePtr = findYangModuleByName($2);
			    if (!thisParserPtr->yangModulePtr) {
                    thisParserPtr->yangModulePtr =  addYangNode($2, YANG_DECL_MODULE, NULL);
                    
                    if (smiHandle->firstYangModulePtr) {
                        smiHandle->firstYangModulePtr->nextSiblingPtr = thisModulePtr;
                    } else {
                        smiHandle->firstYangModulePtr = thisModulePtr;
                    }
			    } else {
			        smiPrintError(thisParserPtr, ERR_MODULE_ALREADY_LOADED, $2);
                    free($2);
                    /*
                     * this aborts parsing the whole file,
                     * not only the current module.
                     */
                    YYABORT;
			    }
                $$ = 1;
                currentParser = thisParserPtr;
                currentModule = thisModulePtr;
                currentModuleInfo = createModuleInfo(currentModule);
                pushNode(currentModule);
			}
			'{'
                stmtSep
				moduleHeaderStatement_0n
				linkageStatement_0n
				moduleMetaStatement_0n
				revisionStatement_0n
				bodyStatement_0n			
			'}'
			{
                pop();
			}
	;

submoduleStatement:	submoduleKeyword identifier
			{
			   pushDecl(YANG_DECL_SUBMODULE);
			}
			'{'
				moduleHeaderStatement_0n
				linkageStatement_0n
				moduleMetaStatement_0n
				revisionStatement_0n
				bodyStatement_0n			
			'}'
			{
			   pop();
			}
	;

moduleHeaderStatement_0n:	moduleHeaderStatement0_n
         	{
                if (!currentModuleInfo->namespace) {
                    smiPrintError(parserPtr, ERR_NAMESPACE_MISSING, NULL);
                }
                if (!currentModuleInfo->prefix) {
                    smiPrintError(parserPtr, ERR_PREFIX_MISSING, NULL);
                }
                $$ = 1;
			}
		;

moduleHeaderStatement0_n:
                |
                    moduleHeaderStatement moduleHeaderStatement0_n
                ;

moduleHeaderStatement:	yangVersionStatement stmtSep
                |
                        namespaceStatement stmtSep
                |
                        prefixStatement stmtSep
		;

moduleMetaStatement_0n:	
		     	{
				$$ = 1;
			}
		|
			moduleMetaStatement_0n moduleMetaStatement stmtSep
			{
				$$ = 1 + $2;
			}
		;

moduleMetaStatement:	organizationStatement
		|
			contactStatement 
		|
			descriptionStatement
		|
			referenceStatement
		;

linkageStatement_0n:	{
				$$ = 1;
			}
		|
			linkageStatement_0n linkageStatement
			{
				$$ = 1 + $2;
			}
		;

linkageStatement:	includeStatement stmtSep
		|
			importStatement stmtSep
		;

revisionStatement_0n:	{
				$$ = 1;
			}
		|
			revisionStatement_0n revisionStatement stmtSep
			{
				$$ = $1 + 1;
			}
		;

bodyStatement_0n:	
		     	{
				$$ = 1;
			}
		|
			bodyStatement_0n bodyStatement stmtSep
			{
				$$ = $1 + 1;
			}
		;

bodyStatement:		extensionStatement
		|
			typedefStatement
		|
			groupingStatement
	    |
	     	dataDefStatement
		|
			rpcStatement
		|
			notificationStatement
		;

dataDefStatement:	containerStatement
		|
			leafStatement
		|
			leaf_listStatement
		|
			listStatement
		|
			choiceStatement
		|
			anyXMLStatement
		|
			usesStatement
		|
			augmentStatement
		;

commonStatement:	descriptionStatement 
       	|
			statusStatement
		|
			referenceStatement
		|
			configStatement
		;	       

organizationStatement:	organizationKeyword string stmtEnd
			{
                if (!currentModuleInfo->organization) {
                    node = addYangNode($2, YANG_DECL_ORGANIZATION, topNode());
                    currentModuleInfo->organization = node->export.value;
                } else {
                    smiPrintError(currentParser, ERR_REDEFINED_ORGANIZATION, NULL);
                }				
                $$ = 1;
			}
	;

contactStatement:	contactKeyword string stmtEnd
			{
                if (!currentModuleInfo->contact) {
                    node = addYangNode($2, YANG_DECL_CONTACT, topNode());
                    currentModuleInfo->contact = node->export.value;
                } else {
                    smiPrintError(currentParser, ERR_REDEFINED_CONTACT, NULL);
                }
                $$ = 1;
			}
	;

descriptionStatement:	descriptionKeyword string stmtEnd
			{
                uniqueDescription(topNode());
                setDescription(topNode(), $2);
                $$ = 1;
			}
	;

referenceStatement:	referenceKeyword string stmtEnd
			{
                uniqueReference(topNode());
                setReference(topNode(), $2);
				$$ = 1;
			}
	;

statusStatement:	statusKeyword status stmtEnd
			{
                uniqueStatus(topNode());
                setStatus(topNode(), $2);
                $$ = 1;
			}
	;

namespaceStatement:	namespaceKeyword string stmtEnd
		  	{
                if (!currentModuleInfo->namespace) {
                    node = addYangNode($2, YANG_DECL_NAMESPACE, topNode());
                    currentModuleInfo->namespace = node->export.value;
                } else {
                    smiPrintError(currentParser, ERR_REDEFINED_NAMESPACE, NULL);
                }
			}
	;

yangVersionStatement:  yangversionKeyword yangVersion stmtEnd
		  	{
                if (!currentModuleInfo->version) {
                    node = addYangNode($2, YANG_DECL_YANGVERSION, topNode());
                    currentModuleInfo->version = node->export.value;
                } else {
                    smiPrintError(currentParser, ERR_REDEFINED_YANGVERSION, NULL);
                }
			}
	;

status:		deprecatedKeyword
		{
			$$ = YANG_STATUS_OBSOLETE;
		}
	|
		currentKeyword
		{
			$$ = YANG_STATUS_OBSOLETE;
		}
	|
		obsoleteKeyword
		{
			$$ = YANG_STATUS_OBSOLETE;
		}
	;

prefixStatement:	prefixKeyword prefix stmtEnd
			{
                node = addYangNode($2, YANG_DECL_PREFIX, topNode());
                switch(topDecl())
                {
                    case YANG_DECL_MODULE:
                        if(!currentModuleInfo->prefix)
                            currentModuleInfo->prefix = node->export.value;
                        else 
                            smiPrintError(currentParser, ERR_REDEFINED_PREFIX, NULL);
                        break;
                    case YANG_DECL_IMPORT:
                    
                        break;
                    default:
                        //TODO print error
                        debug("DEBUGG: OOPS wrong prefix DECL %d, at line %d\n", topDecl(), currentParser->line);
                        break;			
                }
			}
	;

revisionStatement:	revisionKeyword date ';' 
			{
                node = addYangNode($2, YANG_DECL_REVISION, topNode());
            }
        |
                    revisionKeyword date
			{
                node = addYangNode($2, YANG_DECL_REVISION, topNode());
				pushNode(node);
			}
			'{'
                stmtSep
				revisionDescriptionStatement
			'}'
			{
				pop();
			}
    ;


revisionDescriptionStatement: 
                    |
                            descriptionStatement stmtSep 
                    ;

date: 	dateString
	{
        checkDate(currentParser, $1);
		$$ = $1;
	}
	;

importStatement: importKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_IMPORT, topNode());
			pushNode(node);
		}
		'{'
            stmtSep
			prefixStatement stmtSep
            importRevision
		'}'
		{
            importModule(node);
			pop();
		}
        ;

importRevision:
                    |
                      revisionStatement stmtSep 
                    ;

includeStatement: includeKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_INCLUDE, topNode());
			pushNode(node);
		}
                includeStatementBody
		{
			pop();
		}
        ;

includeStatementBody:         ';'
                |
                    '{'
                               stmtSep
                    '}'
                ;

typedefStatement:	typedefKeyword identifier
			{
                node = addYangNode($2, YANG_DECL_TYPEDEF, topNode());
                pushNode(node);
			}
			'{'
                stmtSep
				typedefSubstatement_0n
			'}'
			{                                
				pop();
			}
	;

typedefSubstatement_0n:	typedefSubstatement
			{
				$$ = 1;
			}
		|
		       typedefSubstatement_0n typedefSubstatement stmtSep
			{
				$$ = 1 + $1;
			}
	;				

typedefSubstatement:	typeStatement
		|
			descriptionStatement
		|
			referenceStatement
		|
			defaultStatement
	;

typeStatement: typeKeyword type
		{			
		}
	;

type:		refinedBasetype
	|
		refinedType
	;

refinedBasetype:	float32Keyword numRestriction
			{
//				setType(SMI_BASETYPE_FLOAT32, smiHandle->typeFloat32Ptr, NULL);
			}
		|	
			float64Keyword numRestriction
			{
//				setType(SMI_BASETYPE_FLOAT64, smiHandle->typeFloat64Ptr, NULL);
			}
		|
			int8Keyword numRestriction
			{
//				setType(SMI_BASETYPE_INTEGER8, smiHandle->typeInteger8Ptr, NULL);
			}
		|	
			int16Keyword numRestriction
			{
//				setType(SMI_BASETYPE_INTEGER16, smiHandle->typeInteger16Ptr, NULL);
			}
		|	
			int32Keyword numRestriction
			{
//				setType(SMI_BASETYPE_INTEGER32, smiHandle->typeInteger32Ptr, NULL);
			}
		|
			int64Keyword numRestriction
			{
//				setType(SMI_BASETYPE_INTEGER64, smiHandle->typeInteger64Ptr, NULL);
			}
		|
			uint8Keyword numRestriction
			{
//				setType(SMI_BASETYPE_UNSIGNED8, smiHandle->typeUnsigned8Ptr, NULL);
			}
		|	
			uint16Keyword numRestriction
			{
//				setType(SMI_BASETYPE_UNSIGNED16, smiHandle->typeUnsigned16Ptr, NULL);
			}
		|	
			uint32Keyword numRestriction
			{
//				setType(SMI_BASETYPE_UNSIGNED32, smiHandle->typeUnsigned32Ptr, NULL);
			}
		|	
			uint64Keyword numRestriction
			{
//				setType(SMI_BASETYPE_UNSIGNED64, smiHandle->typeUnsigned64Ptr, NULL);
			}
		|
			stringKeyword stringRestriction
			{
				
//				setType(SMI_BASETYPE_OCTETSTRING, smiHandle->typeOctetStringPtr, NULL);
			}
		|
			float32Keyword ';'
			{
//				setType(SMI_BASETYPE_FLOAT32, smiHandle->typeFloat32Ptr, NULL);
			}
		|	
			float64Keyword ';'
			{
//				setType(SMI_BASETYPE_FLOAT64, smiHandle->typeFloat64Ptr, NULL);
			}
		|
			int8Keyword ';'
			{
//				setType(SMI_BASETYPE_INTEGER8, smiHandle->typeInteger8Ptr, NULL);
			}
		|	
			int16Keyword ';'
			{
//				setType(SMI_BASETYPE_INTEGER16, smiHandle->typeInteger16Ptr, NULL);
			}
		|	
			int32Keyword ';'
			{
//				setType(SMI_BASETYPE_INTEGER32, smiHandle->typeInteger32Ptr, NULL);
			}
		|
			int64Keyword ';'
			{
//				setType(SMI_BASETYPE_INTEGER64, smiHandle->typeInteger64Ptr, NULL);
			}
		|
			uint8Keyword ';'
			{
//				setType(SMI_BASETYPE_UNSIGNED8, smiHandle->typeUnsigned8Ptr, NULL);
			}
		|	
			uint16Keyword ';'
			{
//				setType(SMI_BASETYPE_UNSIGNED16, smiHandle->typeUnsigned16Ptr, NULL);
			}
		|	
			uint32Keyword ';'
			{
//				setType(SMI_BASETYPE_UNSIGNED32, smiHandle->typeUnsigned32Ptr, NULL);
			}
		|	
			uint64Keyword ';'
			{
//				setType(SMI_BASETYPE_UNSIGNED64, smiHandle->typeUnsigned64Ptr, NULL);
			}
		|
			stringKeyword ';'
			{
				
//				setType(SMI_BASETYPE_OCTETSTRING, smiHandle->typeOctetStringPtr, NULL);
			}
		|
			booleanKeyword ';'
			{
//				setType(SMI_BASETYPE_BOOLEAN, smiHandle->typeBooleanPtr, NULL);
			}
		|
			leafrefKeyword path
			{
                // TODO: in version 03 'keyref' has been changed to 'leafref'
//				setType(SMI_BASETYPE_KEYREF, smiHandle->typeKeyrefPtr, NULL);
			}
		|
			binaryKeyword ';'
			{
//				setType(SMI_BASETYPE_BINARY, smiHandle->typeBinaryPtr, NULL);
			}
		|
			enumerationKeyword enumSpec
	;

refinedType:	identifierRef 
		{
            node = addYangNode($1, YANG_DECL_TYPE, topNode());
            pushNode(node);

/*			Type *t = findType($1);
			if(t) //parent type already in types list
			{
				setType(SMI_BASETYPE_UNKNOWN,t,$1);
			}
			else //forward reference
			{
				setType(SMI_BASETYPE_UNKNOWN,NULL,$1);
			}*/
		}
            typeBody
        {                                
            pop();
        }
	;

typeBody:   ';'
        |
            '{'
                stmtSep
                restriction 
			'}'

restriction: 	numRestriction
	|
		stringRestriction
	|
		enumSpec
	|
		path
	|
		';'
	;
	
numRestriction: '{'
			range
		'}'
		{
			$$=NULL;
		}
	;

stringRestriction:	'{'
			length
                    '}'
        |
            '{'
                pattern
            '}'
        |
            '{'
                pattern
                length
            '}'
        |
            '{'
                length
                pattern
            '}'
        ;

enumSpec: 
	'{'
		enum_0n
	'}'


enum_0n:
       |
	enum_0n enum
	;

enum: enumKeyword string ';'
    |
	enumKeyword string
	'{'
		valueKeyword string stmtEnd
	'}'
	;

range:		rangeKeyword string
		{
            node = addYangNode($2, YANG_DECL_RANGE, topNode());
            pushNode(node);
		}
		 optError
		{
			pop();
		}
	;

length:		lengthKeyword string
		{
            node = addYangNode($2, YANG_DECL_LENGTH, topNode());
            pushNode(node);
		}
		 optError
		{
			pop();
		}
	;


path:		pathKeyword string optError
		{
            node = addYangNode($2, YANG_DECL_PATH, topNode());
		}
	;

pattern:	patternKeyword string 
        {
            node = addYangNode($2, YANG_DECL_PATTERN, topNode());
            pushNode(node);
		}
		 optError
		{
			pop();
		}
	;

optError:	';'
    	| 
            '{'
                err
            '}'
        ;

err: 	error_app_tag error_message
	|
		error_message error_app_tag
	|
		error_message
	|
		error_app_tag
	;

error_message: error_messageKeyword string stmtEnd
            {
                node = addYangNode($2, YANG_DECL_ERROR_MESSAGE, topNode());
            }
	;

error_app_tag: error_app_tagKeyword string stmtEnd
            {
                node = addYangNode($2, YANG_DECL_ERROR_APP_TAG, topNode());
            }
	;

stmtEnd:    ';'
         |
            '{'
                    unknownStatement0_n
            '}'
        ;

stmtSep:
	|
        unknownStatement0_n
	;

unknownStatement0_n:
	|
        unknownStatement unknownStatement0_n;
	;

unknownStatement:   identifier stmtEnd
        |                    
                    identifier string stmtEnd
        ;

containerStatement: containerKeyword identifier
			{
                node = addYangNode($2, YANG_DECL_CONTAINER, topNode());				
                pushNode(node);
			}
			'{'
				containerSubstatement_0n
			'}'
			{
				pop();
			}
	;


containerSubstatement_0n:	containerSubstatement
			{
				$$ = 1;
			}
		|
		       containerSubstatement containerSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

containerSubstatement:	commonStatement
		|
			dataDefStatement
		|
			mustStatement
		|
			typedefStatement
		;

mustStatement: mustKeyword string
		{
            node = addYangNode($2, YANG_DECL_MUST_STATEMENT, topNode());
            pushNode(node);
		}
		'{'
			mustSubstatement_0n
		'}'
		{
			pop();
		}
	|
		mustKeyword string ';'
		{
            node = addYangNode($2, YANG_DECL_MUST_STATEMENT, topNode());
            pushNode(node);
		}
	;

mustSubstatement_0n:	mustSubstatement
			{
				$$ = 1;
			}
		|
		       mustSubstatement mustSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

mustSubstatement:	error_message
		|
			error_app_tag
		|
			descriptionStatement
		|
			referenceStatement
		;
			
configStatement: 	configKeyword trueKeyword stmtEnd
			{
                setConfig(topNode(), YANG_CONFIG_TRUE);
			}
	|
			configKeyword falseKeyword stmtEnd
			{
				setConfig(topNode(), YANG_CONFIG_FALSE);
			}
		;

mandatoryStatement: mandatoryKeyword trueKeyword stmtEnd
			{
                node = addYangNode("true", YANG_DECL_MANDATORY, topNode());
			}
		|
		    	mandatoryKeyword falseKeyword stmtEnd	
			{
				node = addYangNode("false", YANG_DECL_MANDATORY, topNode());
			}
		;
			
leafStatement: leafKeyword identifier
			{
				node = addYangNode($2,YANG_DECL_LEAF, topNode());
                pushNode(node);
			}
			'{'
				leafSubstatement_0n
			'}'
			{
				pop();
			}
		;
			
leafSubstatement_0n:	leafSubstatement
			{
				$$ = 1;
			}
		|
		       leafSubstatement leafSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

leafSubstatement:	mustStatement
		|
			commonStatement
		|
			mandatoryStatement
		|
			typeStatement
		|
			defaultStatement	
		;

leaf_listStatement: leaf_listKeyword identifier
			{
				node = addYangNode($2, YANG_DECL_LEAF_LIST, topNode());
                pushNode(node);
			}
			'{'
				leaf_listSubstatement_0n
			'}'
			{
				pop();
			}
		;
			
leaf_listSubstatement_0n:	leaf_listSubstatement
			{
				$$ = 1;
			}
		|
		       leaf_listSubstatement leaf_listSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

leaf_listSubstatement:	mustStatement
		|
			commonStatement
		|
			mandatoryStatement
		|
			typeStatement
		|	
			max_elementsStatement
		|
			min_elementsStatement
		|
			ordered_byStatement
		;
		
listStatement: listKeyword identifier
			{
				node = addYangNode($2, YANG_DECL_LIST, topNode());
                pushNode(node);
			}
			'{'
				listSubstatement_0n
			'}'
			{
				pop();
                /*  TODO: 
                 * checkUniqueAndKey();
                 */
			}
		;

listSubstatement_0n:	listSubstatement
			{
				$$ = 1;
			}
		|
		       listSubstatement listSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

listSubstatement:	mustStatement
		|
			commonStatement
		|
			max_elementsStatement
		|
			min_elementsStatement
		|
			ordered_byStatement
		|
			keyStatement
		|
			uniqueStatement
		|
			dataDefStatement
		|
			typedefStatement
		|
			groupingStatement
		;

max_elementsStatement: 	max_elementsKeyword string stmtEnd
			{
				if(strcmp($2,"unbounded")) //if true means string is different from "unbounded"
				{
					//TODO print error
				}
				//else do nothing because unbounded is default value anyway
			}
		|	max_elementsKeyword decimalNumber stmtEnd
			{
				//SmiUnsigned32 value = strtoul($2, NULL, 10);
				//setYangNodeMaxElements(currentNode, value);
			}
		;

min_elementsStatement: 	min_elementsKeyword decimalNumber stmtEnd
			{
				//SmiUnsigned32 value = strtoul($2, NULL, 10);
				//setYangNodeMinElements(currentNode, value);				
			}
		;

ordered_byStatement: 	ordered_byKeyword string stmtEnd
			{
/*				if(!strcmp($2,"system"))
				{
					setYangNodeOrder(currentNode, SMI_ORDER_SYSTEM);
				}
				else if(!strcmp($2,"user"))
				{
					setYangNodeOrder(currentNode, SMI_ORDER_USER);
				}
				else
				{
					//TODO print error
				}*/
			}
		;

keyStatement: keyKeyword string stmtEnd
		{
/*			currentNode->keyList = (YangNodeList*)smiMalloc(sizeof(YangNodeList));
			YangNodeList *list  = currentNode->keyList;
			
			char *beginWord = $2; //beginning or current word
			int i = 0;
			for(i; beginWord[i]; i++) //substitude tabs with spaces
			{
				if(beginWord[i]=='\t') beginWord[i]=' ';
			}
			
			char *nextWord = strstr(beginWord," "); //location of next space
 
			while(nextWord)
			{
				nextWord[0] = '\0'; //change space for null so that duplication is to this position
				list->name = smiStrdup(beginWord);
				list->next = (YangNodeList*)smiMalloc(sizeof(YangNodeList));
				list = list->next;
				nextWord++; //next iteration starts from char after space
				beginWord = nextWord;
				nextWord = strstr(beginWord," ");
			}
			list->name = smiStrdup(beginWord);
			list->next = NULL;//terminate list*/
		}
	;
	
uniqueStatement: uniqueKeyword string stmtEnd
		{
/*			currentNode->uniqueList = (YangNodeList*)smiMalloc(sizeof(YangNodeList));
			YangNodeList *list  = currentNode->uniqueList;
			
			char *beginWord = $2; //beginning or current word
			int i = 0;
			for(i; beginWord[i]; i++) //substitute tabs with spaces
			{
				if(beginWord[i]=='\t') beginWord[i]=' ';
			}
			
			char *nextWord = strstr(beginWord," "); //location of next space
 
			while(nextWord)
			{
				nextWord[0] = '\0'; //change space for null so that duplication is to this position
				list->name = smiStrdup(beginWord);
				list->next = (YangNodeList*)smiMalloc(sizeof(YangNodeList));
				list = list->next;
				nextWord++; //next iteration starts from char after space
				beginWord = nextWord;
				nextWord = strstr(beginWord," ");
			}
			list->name = smiStrdup(beginWord);
			list->next = NULL;//terminate list*/
		}
	;

choiceStatement: choiceKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_CHOICE, topNode());
            pushNode(node);
		}
		'{'
			choiceSubstatement_0n			
		'}'
		{
			pop();
		}
		;
 
choiceSubstatement_0n:	choiceSubstatement
			{
				$$ = 1;
			}
		|
		       choiceSubstatement choiceSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

choiceSubstatement:	commonStatement
		  |
			caseStatement
		  |
			defaultStatement
		;

caseStatement: 	caseKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_CASE, topNode());
            pushNode(node);
		}
		'{'
			caseSubstatement_0n
		'}'
		{
			pop();
		}	
	|
		{
            node = addYangNode("", YANG_DECL_CASE, topNode());
            pushNode(node);
    	}
			dataDefStatement
		{
			//TODO fix name of che case node to be the same as caseSubstatement name
			pop();
		}	
	;

caseSubstatement_0n:	caseSubstatement
			{
				$$ = 1;
			}
		|
		       caseSubstatement caseSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

caseSubstatement: 	descriptionStatement
		|
			statusStatement
		|
			referenceStatement
		|
			mandatoryStatement
		|
			dataDefStatement
		;

groupingStatement: groupingKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_GROUPING, topNode());
            pushNode(node);
		}
		'{'
			groupingSubstatement_0n
		'}'
		{
			pop();
		}
		;

groupingSubstatement_0n:	groupingSubstatement
			{
				$$ = 1;
			}
		|
		       groupingSubstatement groupingSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

groupingSubstatement:	statusStatement
		|
			descriptionStatement
		|
			referenceStatement
		|
			dataDefStatement
		|
			groupingStatement
		|
			typedefStatement
		;

usesStatement: usesKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_USES, topNode());
            pushNode(node);
    	}
	     	'{'
			usesSubstatement_0n
	     	'}'
		{
			pop();
		}
	|
		usesKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_USES, topNode());
            pushNode(node);
		}
		';'
		{
			pop();
		}
		;

usesSubstatement_0n:	usesSubstatement
			{
				$$ = 1;
			}
		|
		       usesSubstatement usesSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

usesSubstatement:	descriptionStatement
		|
			referenceStatement
		|
			statusStatement
		|
			containerStatement
		|
			leafStatement
		|
			leaf_listStatement
		|
			listStatement
		|
			choiceStatement
		|
			anyXMLStatement
		|
			usesStatement
		;

augmentStatement: augmentKeyword string 
		{
            node = addYangNode($2, YANG_DECL_AUGMENT, topNode());
            pushNode(node);
		}
		'{'
			augmentSubstatement_0n
    	'}'
		{
			pop();
		}
		;

augmentSubstatement_0n:	augmentSubstatement
			{
				$$ = 1;
			}
		|
		       augmentSubstatement augmentSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

augmentSubstatement:	whenStatement
		|
		   	descriptionStatement
		|
			referenceStatement
		|
			statusStatement
		|
			containerStatement
		|
			leafStatement
		|
			leaf_listStatement
		|
			listStatement
		|
			choiceStatement
		|
			anyXMLStatement
		|
			usesStatement
		;

whenStatement:	whenKeyword string stmtEnd
	    {
			//currentNode->when = smiStrdup($2)
        }
		;

rpcStatement: rpcKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_RPC, topNode());
            pushNode(node);
		}
		'{'
			rpcSubstatement_0n
		'}'
		{
			pop();
		}
		;

rpcSubstatement_0n:	rpcSubstatement
			{
				$$ = 1;
			}
		|
		       rpcSubstatement rpcSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

rpcSubstatement:	descriptionStatement
		|
			referenceStatement
		|
			statusStatement
		|
			inputStatement
		|
			outputStatement
	;

inputStatement: inputKeyword
		{
            node = addYangNode("", YANG_DECL_INPUT, topNode());
            pushNode(node);
		}
		'{'
			inputSubstatement_0n
		'}'
		{
			pop();
		}
		;

inputSubstatement_0n:	inputSubstatement
			{
				$$ = 1;
			}
		|
		       inputSubstatement inputSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

inputSubstatement:	dataDefStatement
	  	|
			groupingStatement
		|
			typedefStatement
		;

outputStatement: outputKeyword
		{
            node = addYangNode("", YANG_DECL_OUTPUT, topNode());
            pushNode(node);
		}
		'{'
			outputSubstatement_0n
		'}'
		{
			pop();
		}
		;

outputSubstatement_0n:	outputSubstatement
			{
				$$ = 1;
			}
		|
		       outputSubstatement outputSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

outputSubstatement:	dataDefStatement
	  	|
			groupingStatement
		|
			typedefStatement
		;

notificationStatement: notificationKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_NOTIFICATION, topNode());
            pushNode(node);
		}
		'{'
			notificationSubstatement_0n
		'}'
		{
			pop();
		}
		;

notificationSubstatement_0n:	notificationSubstatement
			{
				$$ = 1;
			}
		|
		       notificationSubstatement notificationSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

notificationSubstatement:	descriptionStatement
			|
				referenceStatement
			|
				statusStatement
			|	
				dataDefStatement
	  		|
				groupingStatement
			|
				typedefStatement
			;

anyXMLStatement: anyXMLKeyword identifier
		{
            node = addYangNode($2, YANG_DECL_ANYXML, topNode());
            pushNode(node);
		}
		'{'
			anyXMLSubstatement_0n
		'}'
		{
			pop();
		}
		;

anyXMLSubstatement_0n:	anyXMLSubstatement
			{
				$$ = 1;
			}
		|
		       anyXMLSubstatement anyXMLSubstatement_0n
			{
				$$ = 1 + $2;
			}
	;

anyXMLSubstatement:	descriptionStatement
		|
			referenceStatement
		|
			statusStatement
		;

extensionStatement: extensionKeyword identifier
		{
			node = addYangNode($2, YANG_DECL_EXTENSION, topNode());
            pushNode(node);
		}
                extensionStatementBody
		{
			pop();
		}

		
extensionStatementBody:  '{' stmtSep extensionSubstatement_0n '}'
                    |
                         ';'
                    ;                       


extensionSubstatement_0n:	
			{
				$$ = 1;
			}
		|
            extensionSubstatement_0n extensionSubstatement stmtSep
			{
				$$ = 1 + $1;
			}
		;

extensionSubstatement:	argumentStatement
		|
			statusStatement
		|
			descriptionStatement
		|
			referenceStatement
		;

argumentStatement: argumentKeyword identifier
            {
                uniqueNodeKind(topNode(), YANG_DECL_ARGUMENT);
                addYangNode($2, YANG_DECL_ARGUMENT, topNode());
            }
                    argumentStatementBody
		;

argumentStatementBody:  '{' stmtSep '}'
                    |
                         ';'
                    ;                       


defaultStatement: defaultKeyword string stmtEnd
		;

prefix:		identifier
		{
			$$ = $1;
		}
        ;

identifierRef: identifierRefArgStr 
                {
			$$ = $1;
                }
        |
                identifier 
                {
			$$ = $1;
                }
        ;


string:		qString
		{
			$$ = $1;
		}
	|
    		uqString
		{
			$$ = $1;
		}
	|
            identifier
		{
			$$ = $1;
		}
        |
            dateString
		{
			$$ = $1;
		}
        |
            yangVersion
		{
			$$ = $1;
		}
	;
%%

#endif			
