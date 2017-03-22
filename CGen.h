
#ifndef CGEN_H
#define CGEN_H


#define STACKMARKSIZE  8


#include "Globals.h"


#define WORDSIZE 4


extern int TraceCode;

void codeGen(TreeNode *syntaxTree, char *fileName, char *moduleName);
/* Generates code from the program's abstract syntax tree*/
#endif
