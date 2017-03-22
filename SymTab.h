
#ifndef SYMTAB_H
#define SYMTAB_H

#include "Globals.h"


typedef struct hashNode {
    char          *name;        /*identifier's name*/
    TreeNode      *declaration;  /* pointer to the symbol's declaration node */
    int           lineFirstReferenced;   /* self-explanatory */
    struct hashNode *next;      /* next node */
} HashNode, *HashNodePtr;


extern int scopeDepth;

void initSymbolTable();


void insertSymbol(char *name, TreeNode *symbolDefNode, int lineDefined);


int symbolAlreadyDeclared(char *name);
/*check ifthe given symbol is already defined*/

HashNodePtr lookupSymbol(char *name);
/*Retrieves a HashNode* pointer for a supplied identifier that
*           points to the declaration node for the identifier.*/

void dumpCurrentScope();
/*dumps oup the current scope*/

void newScope();


void endScope();


#endif
