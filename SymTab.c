#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include "Globals.h"
#include "SymTab.h"
#include "Util.h"

#define  MAXTABLESIZE 211

#define  HIGHWATERMARK "13_4_666_invalid"


static HashNodePtr hashtable[MAXTABLESIZE];  /*hash table*/

static HashNodePtr secondList;

extern int TraceAnalyse;

int scopeDepth;  /*current scope's deepth*/



static HashNodePtr allocateSymbolNode(char *name,
				      TreeNode *declaration,
				      int lineDefined);

static int hashFunction(char *key); /*a hash table will be generated*/


static void flagError(char *message);


static char *formatSymbolType(TreeNode *node);

/* the guts of dumpCurrentScope() */
static void dumpCurrentScope2(HashNodePtr cursor);



void initSymbolTable(void)
{
    memset(hashtable, 0, sizeof(HashNodePtr) * MAXTABLESIZE);
    secondList = NULL;
}


void insertSymbol(char *name, TreeNode *symbolDefNode, int lineDefined)
{
    char errorString[80];  /* for error reporting */

    HashNodePtr newHashNode, temp;
    int hashBucket;

    /* If the symbol already exists, flag an error */
    if (symbolAlreadyDeclared(name))
    {
			sprintf(errorString, "duplicate identifier \"%s\"\n", name);
			flagError(errorString);
    }
    else
    {
				hashBucket = hashFunction(name);

				newHashNode = allocateSymbolNode(name, symbolDefNode, lineDefined);
	if (newHashNode != NULL)
	{
		    temp = hashtable[hashBucket];
		    hashtable[hashBucket] = newHashNode;
		    newHashNode->next = temp;
	}

				newHashNode = allocateSymbolNode(name, symbolDefNode, lineDefined);
	if (newHashNode != NULL)
	{
	    temp = secondList;
	    secondList = newHashNode;
	    secondList->next = temp;
	}
    }
}



int symbolAlreadyDeclared(char *name)
{
    int         symbolFound = FALSE;
    HashNodePtr cursor;
    cursor = secondList;

    while ((cursor != NULL) && (!symbolFound)
	   && ((strcmp(cursor->name, HIGHWATERMARK) != 0)))
    {
	if (strcmp(name, cursor->name) == 0)
	    symbolFound = TRUE;
	else
	    cursor = cursor->next;
    }

    return (symbolFound);
}


HashNodePtr lookupSymbol(char *name)
{
    HashNodePtr cursor;
    int         hashBucket;   /* hash bucket */
    int         found = FALSE;

    hashBucket = hashFunction(name);
    cursor = hashtable[hashBucket];

    while (cursor != NULL)
    {
		if (strcmp(name, cursor->name) == 0)
		{
		    found = TRUE;
		    break;
		}

				cursor = cursor->next;
	    }

	    if (found == TRUE)
					return cursor;
	    else
					return NULL;
}



void dumpCurrentScope()
{
    HashNodePtr cursor;

    cursor = secondList;

    /* if the current scope isn't empty,  dump it out */
    if ((cursor != NULL) && (strcmp(HIGHWATERMARK, cursor->name)))
				dumpCurrentScope2(cursor);
}

#define IDENT_LEN 12

static void dumpCurrentScope2(HashNodePtr cursor)
{
    char paddedIdentifier[IDENT_LEN+1];
    char *typeInformation;   /* used to catch result of formatSymbolType */

    if ((cursor->next != NULL) && (strcmp(cursor->next->name, HIGHWATERMARK) != 0))
	dumpCurrentScope2(cursor->next);


    memset(paddedIdentifier, ' ', IDENT_LEN);
    memmove(paddedIdentifier, cursor->name, strlen(cursor->name));
    paddedIdentifier[IDENT_LEN] = '\0';

    /* output symbol table entry */
    typeInformation = formatSymbolType(cursor->declaration);

    fprintf(listing, "%3d   %s   %7d     %c    %s\n",
	    scopeDepth,
	    paddedIdentifier,
	    cursor->lineFirstReferenced,
	    cursor->declaration->isParameter ? 'Y' : 'N',
	    typeInformation);

    /* Prevent a memory leak - (bjf, 11/5/2000) */
    free(typeInformation);
}


void newScope()
{
    HashNodePtr newNode, temp;

    newNode = allocateSymbolNode(HIGHWATERMARK, NULL, 0);
    if (newNode != NULL)
    {
				temp = secondList;
				secondList = newNode;
				secondList->next = temp;
    }
}


void endScope()
{


    HashNodePtr hashPtr;
    HashNodePtr temp;  /* used in freeing HashNodes */
    int         hashBucket;

    while ((secondList != NULL)
        && (strcmp(HIGHWATERMARK, secondList->name)) != 0)
    {
				/* locate this node in the hash table, delete it */
				hashBucket = hashFunction(secondList->name);
				hashPtr = hashtable[hashBucket];


				assert((secondList != NULL) && (hashtable[hashBucket] != NULL));
				assert(strcmp(secondList->name, hashPtr->name) == 0);

				/* delete from hash table */
				temp = hashtable[hashBucket]->next;
				free(hashtable[hashBucket]);
				hashtable[hashBucket] = temp;

				/* ... and from second list */
				temp = secondList->next;
				free(secondList);
				secondList = temp;
    }

    /* delete high water mark */
		    assert(strcmp(secondList->name, HIGHWATERMARK) == 0);
		    temp = secondList->next;
		    free(secondList);
		    secondList = temp;
}


static HashNodePtr allocateSymbolNode(char *name,
				      TreeNode *declaration,
				      int lineDefined)
{
    HashNode *temp;

    temp = (HashNode*)malloc(sizeof(HashNode));
    if (temp == NULL)
    {
	Error = TRUE;
	fprintf(listing,
		"*** Out of memory allocating memory for symbol table\n");
    }
    else
    {
				temp->name = copyString(name);
				temp->declaration = declaration;
				temp->lineFirstReferenced = lineDefined;
				temp->next = NULL;
    }

    return temp;
}

/* Power-of-two multiplier */
#define SHIFT 4

static int hashFunction( char *key)
{
    int temp = 0;
    int i = 0;

    while (key[i] != '\0')
    {
				temp = ((temp << SHIFT) + key[i]) % MAXTABLESIZE;
				++i;
    }

    return temp;
}


static void flagError(char *message)
{
    fprintf(listing, ">>> Semantic error (symbol table): %s", message);
    Error = TRUE;
}


static char *formatSymbolType(TreeNode *node)
{
    char stringBuffer[100];

    if ((node == NULL) || (node->nodekind != DecK))
					strcpy(stringBuffer, "<<ERROR>>");
    else
    {
	switch (node->kind.dec)
	{
	case ScalarDecK:
	    sprintf(stringBuffer, "Scalar of type %s",
		    typeName(node->variableDataType));
	    break;
	case ArrayDecK:
	    sprintf(stringBuffer, "Array of type %s with %d elements",
		    typeName(node->variableDataType), node->val);
	    break;
	case FuncDecK:
	    sprintf(stringBuffer, "Function with return type %s",
		    typeName(node->functionReturnType));
	    break;
	default:
	    strcpy(stringBuffer, "<<UNKNOWN>>");
	    break;
	}
    }

    return copyString(stringBuffer);
}
