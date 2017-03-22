#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "CGen.h"
#include "Globals.h"
#include "Util.h"


static FILE *output;

void calcAsmAttribute(TreeNode *syntaxTree);


void calcSizeAttribute(TreeNode *syntaxTree);


void calcStackOffsets(TreeNode *syntaxTree);


void genComment(char *comment);/* emit a DCode comment to the output file. */

void genCommentSeparator(void); /*  emit a cosmetic "seperator" to the output file */

void genProgram(TreeNode *tree, char *fileName, char *moduleName);

void genTopLevelDecl(TreeNode *tree);


/*
 * generates the appropriate DCode.
 */
void genFunction(TreeNode *tree);


/*
 * genFunctionLocals(): given a function declaration AST node, generates the
 *  appropriate DCode
 */
void genFunctionLocals(TreeNode *tree);

void genFunctionLocals2(TreeNode *tree);
/*
 *  genStatement(): given a statement AST node, generates
 * appropriate DCode. Delegates as necessary.
 */
void genStatement();

void genExpression(TreeNode *tree, int addressNeeded);

/*
 * Generate and return a new unique label.
 */
char *genNewLabel(void);


void genInstruction(char *instruction);
/*
 * genIfStmtt(): generates DCode to evaluate an IF statement.
 */
void genIfStmt(TreeNode *tree);
/*
 * genWhileStmt(): generate DCode to evaluate a WHILE statement.
 */
void genWhileStmt(TreeNode *tree);
/*
 * genReturnStatement(): generate DCode to evaluate a RETURN statement.
 */
void genReturnStmt(TreeNode *tree);

/*
 * genCallStmt(): generates DCode to evaluate CALL statements.
 */
void genCallStmt(TreeNode *tree);
/*
 * varSize(): given a scalar/array declaration, computes the size of the
 *  variable, or returns 0 otherwise.
 */
int varSize(TreeNode *tree);

void codeGen(TreeNode *syntaxTree, char *fileName, char *moduleName)
{
    output = fopen(fileName, "w");
    if (output == NULL)
    {
	       Error = TRUE;
	      fprintf(listing, ">>> Unable to open output file for writing.\n");
    }
    else
    {
	      calcAsmAttribute(syntaxTree);
	      calcSizeAttribute(syntaxTree);
	      calcStackOffsets(syntaxTree);

	      genProgram(syntaxTree, fileName, moduleName);
    }
}

void calcAsmAttribute(TreeNode *tree)
{
    int i;               /* child traversal loop counter */
    static int asmArea;  /* the "maximum" the assembly area needs to get */

    int asmInThisFunc;

    TreeNode *parmPtr;


    while (tree != NULL)
    {

	     if ((tree->nodekind == DecK) && (tree->kind.dec == FuncDecK))
	         asmArea = 0;

	/* visit children */
	for (i=0; i<MAXCHILDREN; ++i)
	    calcAsmAttribute(tree->child[i]);


	if ((tree->nodekind == StmtK)
	    && (tree->kind.stmt == CallK))
	     {
	        asmInThisFunc = 0;

	    /* examine function declaration's parameters */
	        parmPtr = tree->declaration->child[0];

	    while (parmPtr != NULL)
	    {
		      asmInThisFunc += WORDSIZE;
		      parmPtr = parmPtr->sibling;
	    }

	    if (asmInThisFunc > asmArea)
		      asmArea = asmInThisFunc;
	}


	if ((tree->nodekind == DecK)
	    && (tree->kind.dec == FuncDecK))
	{

	    tree->assemblyAreaSize = asmArea;
	}

	tree = tree->sibling;
    }
}


void calcSizeAttribute(TreeNode *syntaxTree)
{
    int i;
    static int size;   /* amount of space locals */

    while (syntaxTree != NULL)
    {

	if ((syntaxTree->nodekind == DecK)
	 && (syntaxTree->kind.dec == FuncDecK))
	    size = 0;

	/* visit children nodes */
	for (i=0; i < MAXCHILDREN; ++i)
	    calcSizeAttribute(syntaxTree->child[i]);

	if ((syntaxTree->nodekind == DecK)
	&& ((syntaxTree->kind.dec == ScalarDecK)
	 || (syntaxTree->kind.dec == ArrayDecK))
	    && (syntaxTree->isParameter == FALSE))
	{
	    if (syntaxTree->kind.dec == ScalarDecK)
		      size += WORDSIZE;
	    else if (syntaxTree->kind.dec == ArrayDecK)
		      size += (WORDSIZE * syntaxTree->val);

	    /* record the attribute */
	    syntaxTree->localSize = size;
	}

        /* leaving a function? record attribute in function-dec node */
	if ((syntaxTree->nodekind == DecK)
	    && (syntaxTree->kind.dec == FuncDecK))
	{
	    syntaxTree->localSize = size;
	}

	syntaxTree = syntaxTree->sibling;
    }
}


void calcStackOffsets(TreeNode *syntaxTree)
{
    int i;      /* used for iterating over tree-node children */


    static int AP;
    static int LP;


    while (syntaxTree != NULL)
    {
	/*  reset offset counters to 0, if we're entering a function,*/
	if ((syntaxTree->nodekind==DecK) && (syntaxTree->kind.dec==FuncDecK))
	{
	    AP = 0; LP = 0;
	}

	for (i=0; i < MAXCHILDREN; ++i)
	    calcStackOffsets(syntaxTree->child[i]);


	if ((syntaxTree->nodekind == DecK) &&
	    ((syntaxTree->kind.dec==ArrayDecK)
	     ||(syntaxTree->kind.dec==ScalarDecK)))
	{
	    if (syntaxTree->isParameter)
	    {
		        syntaxTree->offset = AP;
		        AP += varSize(syntaxTree);
	    } else {

		LP -= varSize(syntaxTree);
		syntaxTree->offset = LP;

	    }
	}

	   syntaxTree = syntaxTree->sibling;
    }
}



/* emit a DCode comment to the output file. */
void genComment(char *comment)
{
    if (TraceCode)
	fprintf(output, ";; %s\n", comment);
}


void genCommentSeparator(void)
{
    if (TraceCode)
	fprintf(output,
	    ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n");
}


void genTopLevelDecl(TreeNode *tree)
{

    TreeNode *current;
    char commentBuffer[80];  /* used for formatting comments */

    current = tree;

    while (current != NULL)
    {
	if ((current->nodekind == DecK) && (current->kind.dec == ScalarDecK))
	{
	    /* scalar */
	    if (TraceCode)
	    {
		        genCommentSeparator();
		        sprintf(commentBuffer,"Variable \"%s\" is a scalar of type %s\n",
			         current->name, typeName(current->variableDataType));
		        genComment(commentBuffer);
	    }

	    fprintf(output, ".VAR\n");
	    fprintf(output, "    _%s: .WORD  1\n\n", current->name);
	} else if ((current->nodekind==DecK)&&(current->kind.dec==ArrayDecK))
	{
	    /* array */
	    if (TraceCode)
	    {
		        genCommentSeparator();
		        sprintf(commentBuffer,"Variable \"%s\" is an array of type %s and size %d\n",current->name, typeName(current->variableDataType),	current->val);
		        genComment(commentBuffer);
	    }

	    fprintf(output, ".VAR\n");
	    fprintf(output, "    _%s: .WORD %d\n\n", current->name,
		    current->val);
	}
	else if ((current->nodekind==DecK)&&(current->kind.dec==FuncDecK))
	{
	    /* function */
	    genFunction(current);
	}

	     current = current->sibling;
    }
}


/*
 * genFunction(): given a function declaration AST node, generates the
 *  appropriate DCode.
 */
void genFunction(TreeNode *tree)
{
    char commentBuffer[80];  /* used for formatting comments */

    /* node must be a function declaration */
    assert((tree->nodekind == DecK) && (tree->kind.dec == FuncDecK));

    if (TraceCode)
    {
	        genCommentSeparator();
	        sprintf(commentBuffer,"Function declaration for \"%s()\"", tree->name);
	        genComment(commentBuffer);
        	fprintf(output, "\n");
    }

    /* emit function header */
    fprintf(output,".PROC _%s(.NOCHECK,.SIZE=%d,.NODISPLAY,.ASSEMBLY=%d)\n",
	    tree->name, tree->localSize, tree->assemblyAreaSize);

    genFunctionLocals(tree);
    fprintf(output, ".ENTRY\n");

    genStatement(tree->child[1]);

    genInstruction("exit");
    genInstruction("endP");
    fprintf(output, ".ENDP\n\n");
}



void genFunctionLocals(TreeNode *tree)
{
    int i;

    for (i=0; i<MAXCHILDREN; ++i)
	if (tree->child[i] != NULL)
	    genFunctionLocals2(tree->child[i]);
}

void genFunctionLocals2(TreeNode *tree)
{

    int i;
    int offset; /* "offset" for .LOCAL declaration */
    char commentBuffer[80];

    while (tree != NULL)
    {
        /* preorder operations go here */

        for (i=0; i < MAXCHILDREN; ++i)
            genFunctionLocals2(tree->child[i]);

	/* postorder operations go here */
	if (tree->nodekind==DecK)
	{
	    /* if TraceCode enabled generate some meaningful comments */
	    if (TraceCode)
	    {
		if (!tree->isParameter)
		{
		    sprintf(commentBuffer,"Local variable \"%s\"",tree->name);
		    genComment(commentBuffer);
		} else {
		    /* it's a parameter */
		    sprintf(commentBuffer, "Parameter \"%s\"", tree->name);
		    genComment(commentBuffer);
		}
	    }


	    if (tree->isParameter)
		      offset = STACKMARKSIZE;
	    else
		      offset = 0;

	    offset += tree->offset;

	    fprintf(output, "  .LOCAL _%s %d,%d (0,0,0)\n",	tree->name, offset, varSize(tree));

	    /* insert a \n to make output more readable */
	    if (TraceCode) fprintf(output, "\n");

	}

        tree = tree->sibling;
    }
}


void genStatement(TreeNode *tree)
{
    TreeNode *current;

    current = tree;

    while (current != NULL)
    {
	if ((current->nodekind==ExpK) && (current->kind.exp==AssignK))
	{
	    genComment("** assignment statement*");

	    genComment("evaluate rvalue as value");

	    genExpression(current->child[1], FALSE);

	    /* generate code to evaluate value */
	    genComment("evaluate lvalue as address");
	    genExpression(current->child[0], TRUE);

	    /* emit code to perform assignment */
	    genInstruction("assignW");

	} else if (current->nodekind==StmtK) {


	    switch(current->kind.stmt)
	    {
      	    case IfK:

      		genIfStmt(current);
      		break;

      	    case WhileK:

      		genWhileStmt(current);
      		break;

      	    case ReturnK:

      		genReturnStmt(current);
      		break;

      	    case CallK:

      		genCallStmt(current);
      		break;

      	    case CompoundK:

      		genStatement(current->child[1]);
      		break;
	    }
	}

	current = current->sibling;
    }
}



void genExpression(TreeNode *tree, int addressNeeded)
{
    char scratch[80];   /* used for assembling arguments to instructions */


    /* if it's an expression, eval as expression... */
    if (tree->nodekind == ExpK) {

	switch (tree->kind.exp) {
	case IdK:
	    if (tree->declaration->kind.dec == ArrayDecK) {


		if (tree->child[0]==NULL)
		{
		    genComment("leave address of array on stack");
		    if (tree->declaration->isGlobal) /* GLOBAL VARIABLE */
		    {
			        genComment("push address of global variable");
			        sprintf(scratch, "pshAdr  _%s", tree->declaration->name);
			        genInstruction(scratch);
		    } else if (tree->declaration->isParameter) {
			        sprintf(scratch, "pshAP   %d", tree->declaration->offset);
			        genInstruction(scratch);
			        genInstruction("derefW");
		    } else {  /* it's a local variable to the calling procedure */
			        sprintf(scratch, "pshLP   %d", tree->declaration->offset);
			        genInstruction(scratch);
		    }


		} else {
		    /* calculate array offset */
		    genComment("calculate array offset");
		    genExpression(tree->child[0], FALSE);
		    genInstruction("pshLit  4");
		    genInstruction("mul     noTrap");

		    /* get address of array variable onto the stack */
		    genComment("get address of array onto stack");
		    if (tree->declaration->isGlobal) /* GLOBAL VARIABLE */
		    {
			       genComment("push address of global variable");
			       sprintf(scratch, "pshAdr  _%s", tree->declaration->name);
			       genInstruction(scratch);
		    } else if (tree->declaration->isParameter) {
			       sprintf(scratch, "pshAP   %d", tree->declaration->offset);
			       genInstruction(scratch);
			       genInstruction("derefW");
		    } else {
			       sprintf(scratch, "pshLP   %d", tree->declaration->offset);
			       genInstruction(scratch);
		    }

		    /* index into array */
		    genComment("index into array");
		    genInstruction("add     noTrap");

		    /* if needed, dereference */
		    if (!addressNeeded) {
			       genComment("dereference resulting address");
			       genInstruction("derefW");
		    }
		}

	    } else if (tree->declaration->kind.dec == ScalarDecK) {

		/* get effective address */
		genComment("calculate effective address of variable");
		if (tree->declaration->isGlobal) { /* GLOBAL VARIABLE */
		    genComment("push address of global variable");
		    sprintf(scratch, "pshAdr  _%s", tree->declaration->name);
		} else if (tree->declaration->isParameter) {
		    genComment("push parm address");
		    sprintf(scratch, "pshAP   %d", tree->declaration->offset);
		} else {
		    genComment("push address of local");
		    sprintf(scratch, "pshLP   %d", tree->declaration->offset);
		}

		genInstruction(scratch);

		if (!addressNeeded)
		    genInstruction("derefW");
	    }

	    break;

	case OpK:

	    /* compute operands */
	    genExpression(tree->child[0], FALSE);
	    genExpression(tree->child[1], FALSE);

	    switch (tree->op) {

	    case PLUS:

		genInstruction("add     noTrap");
		break;

	    case MINUS:

		genInstruction("sub     noTrap");
		break;

	    case TIMES:

		genInstruction("mul     noTrap");
		break;

	    case DIVIDE:

		genInstruction("div     noTrap");
		break;

	    case LT:

		genInstruction("intLS");
		break;

	    case GT:

		genInstruction("intGT");
		break;

	    case NE:

		genInstruction("relNE");
		break;

	    case LTE:

		genInstruction("intLE");
		break;

	    case GTE:

		genInstruction("intGE");
		break;

	    case EQ:

		genInstruction("relEQ");
		break;

	    default:
		abort();
	    }

	    break;

	case ConstK:

	    sprintf(scratch, "pshLit  %d", tree->val);
	    genInstruction(scratch);
	    break;

	case AssignK:

	    /* generate code to find value */
	    genComment("calculate the rvalue of the assignment");
	    genExpression(tree->child[1], FALSE);
	    genInstruction("dup1");
	    genComment("calculate the lvalue of the assignment");
	    genExpression(tree->child[0], TRUE);
	    genComment("perform assignment");
	    genInstruction("assignW");

	    break;

	}
    } else if (tree->nodekind == StmtK)
    {
	if (tree->kind.stmt == CallK)
	{
	    genCallStmt(tree);
	    if (tree->declaration->functionReturnType != Void)
		genInstruction("pshRetW");
	}
    }
}



char *genNewLabel(void)
{
    static int nextLabel = 0;
    char labelBuffer[40];


    sprintf(labelBuffer, "label%d", nextLabel++);
    return copyString(labelBuffer);
}



void genInstruction(char *instruction)
{
    fprintf(output, "        %s\n", instruction);
}


/*generates DCode to evaluate an IF statement.*/
void genIfStmt(TreeNode *tree)
{
    char *elseLabel;
    char *endLabel;
    char scratch[80];

    elseLabel = genNewLabel();
    endLabel = genNewLabel();

    /* test expression */
    genComment("IF statement");
    genComment("if false, jump to else-part");
    genExpression(tree->child[0], FALSE);

    sprintf(scratch, "brFalse %s", elseLabel);
    genInstruction(scratch);

    genStatement(tree->child[1]);
    sprintf(scratch, "branch %s", endLabel);
    genInstruction(scratch);

    fprintf(output, "%s:\n", elseLabel);

    genStatement(tree->child[2]);

    fprintf(output, "%s:\n", endLabel);
}



void genWhileStmt(TreeNode *tree)
{
    char *startLabel;
    char *endLabel;
    char scratch[80];

    startLabel = genNewLabel();
    endLabel = genNewLabel();

    genComment("WHILE statement");
    genComment("if expression evaluates to FALSE, exit loop");

    fprintf(output, "%s:\n", startLabel);

    genExpression(tree->child[0], FALSE);

    sprintf(scratch, "brFalse  %s", endLabel);
    genInstruction(scratch);

    genStatement(tree->child[1]);

    /* emit branch to start */
    sprintf(scratch, "branch  %s", startLabel);
    genInstruction(scratch);

    /* generate end label */
    fprintf(output, "%s:\n", endLabel);
}


/* generate DCode to evaluate a RETURN statement. */
void genReturnStmt(TreeNode *tree)
{
    if (tree->declaration->functionReturnType != Void) {

	if (tree->child[0] != NULL)
	    genExpression(tree->child[0], FALSE);
	else
	    genInstruction("pshLit  0");

	genInstruction("popRetW");
    }

    genInstruction("exit");
}


/* generates DCode to evaluate CALL statements. */
void genCallStmt(TreeNode *tree)
{
    int numPars = 0;
    TreeNode *argPtr;   /* iterate over function arguments */
    char scratch[80];   /* scratch place to format instructions */


    argPtr = tree->child[0];

    while (argPtr != NULL)
    {
	genExpression(argPtr, FALSE);
	sprintf(scratch, "mkPar   4,%d", numPars * 4);
	genInstruction(scratch);

	++numPars;
	argPtr = argPtr->sibling;
    }

    sprintf(scratch, "call    _%s,%d", tree->name, numPars);
    genInstruction(scratch);
}


/* genProgram(): given a abstract syntax tree, generates the appropriate DCode.*/
void genProgram(TreeNode *tree, char *fileName, char *moduleName)
{

    genCommentSeparator();
    genComment("Generated by Ben Fowler\'s C- compiler");
    genComment("ITB464 Modern Compiler Construction.");
    genCommentSeparator();
    fprintf(output, "\n");

    fprintf(output, ".TITLE %s\n", moduleName);
    fprintf(output, ".FILE \"%s\"\n\n", fileName);
    fprintf(output, ".EXPORT _main\n\n");
    fprintf(output, ".IMPORT _input\n");
    fprintf(output, ".IMPORT _output\n\n");

    genTopLevelDecl(tree);
}


int varSize(TreeNode *tree)
{
    int size;

    if (tree->nodekind == DecK)
    {
	if (tree->kind.dec == ScalarDecK)
	    size = WORDSIZE;
	else if (tree->kind.dec == ArrayDecK)
	{
	    /* array parameters are passed by reference! */
	    if (tree->isParameter)
		size = 4;
	    else
		size = WORDSIZE * (tree->val);
	} else
	    size = 0;
    } else
	size = 0;

    return size;
}
