/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * optimiser/common.c - Optimiser Common
 * 
 * Performs optimisations on the AST
 */
#define DEBUG	0
#include <global.h>
#include <ast.h>
#include <symbol.h>
#include <optimiser.h>

// === IMPORTS ===
extern tAST_Node	*Opt1_Optimise(tAST_Node *Node);
extern tAST_Node	*Opt2_Optimise(tAST_Node *Node);
extern tFunction	*gpFunctions;

// === PROTOTYPES ===
void	Optimiser_ProcessTree(void);
void	Optimiser_DoPass(tOptimiseCallback *Callback);
tAST_Node	*Optimiser_ProcessNode(tOptimiseCallback *Callback, tAST_Node *Node);
void	Optimiser_Expand(tAST_Node *Node, tOptimiseCallback *Callback);

// === CODE ===
void Optimiser_ProcessTree(void)
{
	Optimiser_DoPass( Opt1_Optimise );
	Optimiser_DoPass( Opt2_Optimise );
}

/**
 * \brief Statically optimises a single node (used by the variable
 *        definition code)
 */
tAST_Node *Optimiser_StaticOpt(tAST_Node *Node)
{
	return Optimiser_ProcessNode(Opt1_Optimise, Node);
}

void Optimiser_DoPass(tOptimiseCallback *Callback)
{
	for(tFunction *fcn = gpFunctions; fcn; fcn = fcn->Next)
	{
		if( fcn->Sym.Value == NULL )
			continue;
		DEBUG1("Optimising %s()\n", fcn->Name);
		fcn->Sym.Value = Optimiser_ProcessNode(Callback, fcn->Sym.Value);
	}
}

//! \note Undef'd at end of function
#define REPLACE(v)	do{\
	(v) = Optimiser_ProcessNode(Callback, (v));\
	}while(0)

void Optimiser_ProcessNodeList(tOptimiseCallback *Callback, tAST_Node **FirstPtr)
{
	for(tAST_Node *tmp = *FirstPtr, *prev = NULL; tmp; prev = tmp)
	{
		tAST_Node *new = Optimiser_ProcessNode(Callback, tmp);
		if(new != tmp)
		{
			if(prev)
				prev->NextSibling = new;
			else
				*FirstPtr = new;
		}
		tmp = new->NextSibling;
	}
}

tAST_Node *Optimiser_ProcessNode(tOptimiseCallback *Callback, tAST_Node *Node)
{
	switch(Node->Type)
	{
	case NODETYPE_NULL:
	case NODETYPE_NOOP:
		break;
	// Leaves are ignored
	case NODETYPE_FLOAT:
	case NODETYPE_INTEGER:
	case NODETYPE_LOCALVAR:
	case NODETYPE_SYMBOL:
	case NODETYPE_STRING:
		break;
	
	// List nodes
	case NODETYPE_BLOCK:
		Optimiser_ProcessNodeList(Callback, &Node->CodeBlock.FirstStatement);
		break;
	case NODETYPE_FUNCTIONCALL:
		REPLACE( Node->FunctionCall.Function );
		
		Optimiser_ProcessNodeList(Callback, &Node->FunctionCall.FirstArgument);
		break;
	
	// Statements
	case NODETYPE_IF:
		REPLACE( Node->If.Test );
		REPLACE( Node->If.True );
		REPLACE( Node->If.False );
		break;
	case NODETYPE_WHILE:
		REPLACE( Node->While.Test );
		REPLACE( Node->While.Action );
		break;
	case NODETYPE_FOR:
		REPLACE(Node->For.Init);
		REPLACE(Node->For.Test);
		REPLACE(Node->For.Next);
		REPLACE(Node->For.Action);
		break;
	case NODETYPE_RETURN:	// UniOp
		REPLACE(Node->UniOp.Value);
		break;
	
	// Unary Ops
	case NODETYPE_NEGATE ... NODETYPE_PREDEC:
		REPLACE( Node->UniOp.Value );
		break;
	
	
	// Binary Operations
	case NODETYPE_ASSIGNOP ... NODETYPE_BOOLAND:
		REPLACE( Node->BinOp.Left );
		REPLACE( Node->BinOp.Right );
		break;
	
	default:
		fprintf(stderr, "Optimiser_ProcessNode - TODO: Handle node type 0x%x\n", Node->Type);
		break;
	}
	DEBUG3("Node %p type 0x%x - Processing using %p\n", Node, Node->Type, Callback);
	tAST_Node *ret = Callback(Node);
	if(ret != Node) {
		free(Node);
	}
	return ret;
}
#undef REPLACE

void Optimiser_Expand(tAST_Node *Node, tOptimiseCallback *Callback)
{
	tAST_Node	*tmp;
	switch(Node->Type & 0xF00)
	{
	case NODETYPE_NEGATE ... NODETYPE_PREDEC:
		tmp = Callback(Node->UniOp.Value);
		if(tmp != Node->UniOp.Value) {
			free(Node->UniOp.Value);
			Node->UniOp.Value = tmp;
		}
		break;
	
	case NODETYPE_ASSIGNOP ... NODETYPE_BOOLAND:
		tmp = Callback(Node->BinOp.Left);
		if(tmp != Node->BinOp.Left) {
			free(Node->BinOp.Left);
			Node->BinOp.Left = tmp;
		}
		
		tmp = Callback(Node->BinOp.Right);
		if(tmp != Node->BinOp.Right) {
			free(Node->BinOp.Right);
			Node->BinOp.Right = tmp;
		}
		break;
	
	default:
		fprintf(stderr, "Optimiser_Expand - TODO: Handle node %i\n", Node->Type);
		break;
	}
}
