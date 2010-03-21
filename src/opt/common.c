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
	tAST_Node	*tmp;
	tmp = Optimiser_ProcessNode(Opt1_Optimise, Node);
	if(tmp != Node) {
		AST_DeleteNode(Node);
	}
	return tmp;
}

void Optimiser_DoPass(tOptimiseCallback *Callback)
{
	tAST_Node	*tmp;
	tFunction	*fcn;
	for(fcn = gpFunctions;
		fcn;
		fcn = fcn->Next)
	{
		DEBUG1("Optimising %s()\n", fcn->Name);
		tmp = Optimiser_ProcessNode(Callback, fcn->Code);
		if(tmp != fcn->Code) {
			AST_DeleteNode(fcn->Code);
			fcn->Code = tmp;
		}
	}
}

//! \note Undef'd at end of function
#define REPLACE(v)	do{\
	tAST_Node	*new = Optimiser_ProcessNode(Callback, (v));\
	DEBUG2("%p[%03x] ", (v),(v)->Type);\
	if(new != (v)) {\
		DEBUG2("changed (now %p[%03x])\n", new,new->Type);\
		free((v)); (v) = new;\
	}\
	DEBUG2("no change\n");\
	}while(0)

tAST_Node *Optimiser_ProcessNode(tOptimiseCallback *Callback, tAST_Node *Node)
{
	tAST_Node	*tmp, *prev, *new;
	
	switch(Node->Type & 0xF00)
	{
	// Leaves are ignored
	case NODETYPE_CAT_LEAF:	break;
	
	case NODETYPE_CAT_LIST:
		switch(Node->Type)
		{
		case NODETYPE_BLOCK:
			for(tmp = Node->CodeBlock.FirstStatement;
				tmp;
				prev = tmp)
			{
				tAST_Node	*new;
				new = Optimiser_ProcessNode(Callback, tmp);
				if(new != tmp) {
					if(prev)
						prev->NextSibling = new;
					else
						Node->CodeBlock.FirstStatement = new;
					new->NextSibling = tmp->NextSibling;
					free(tmp);
					tmp = new->NextSibling;
				}
				else
					tmp = tmp->NextSibling;
			}
			break;
		
		case NODETYPE_FUNCTIONCALL:
			REPLACE( Node->FunctionCall.Function );
			
			for(tmp = Node->FunctionCall.FirstArgument;
				tmp;
				prev = tmp)
			{
				new = Optimiser_ProcessNode(Callback, tmp);
				if(new != tmp) {
					if(prev)
						prev->NextSibling = new;
					else
						Node->FunctionCall.FirstArgument = new;
					new->NextSibling = tmp->NextSibling;
					free(tmp);
					tmp = new->NextSibling;
				}
				else
					tmp = tmp->NextSibling;
			}
			break;
		}
		break;
	
	// Statements
	case NODETYPE_CAT_STATEMENT:
		switch(Node->Type)
		{
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
		}
		break;
	
	// Unary Ops
	case NODETYPE_CAT_UNARYOPS:
		REPLACE( Node->UniOp.Value );
		break;
	
	
	// Binary Operations
	case NODETYPE_CAT_BINARYOPS:
		REPLACE( Node->BinOp.Left );
		REPLACE( Node->BinOp.Right );
		break;
	
	default:
		fprintf(stderr, "Optimiser_ProcessNode - TODO: Handle node type 0x%x\n", Node->Type);
		break;
	}
	DEBUG3("Node %p type 0x%x - Processing using %p\n", Node, Node->Type, Callback);
	return Callback(Node);
}
#undef REPLACE

void Optimiser_Expand(tAST_Node *Node, tOptimiseCallback *Callback)
{
	tAST_Node	*tmp;
	switch(Node->Type & 0xF00)
	{
	case NODETYPE_CAT_UNARYOPS:
		tmp = Callback(Node->UniOp.Value);
		if(tmp != Node->UniOp.Value) {
			free(Node->UniOp.Value);
			Node->UniOp.Value = tmp;
		}
		break;
	
	case NODETYPE_CAT_BINARYOPS:
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
		fprintf(stderr, "Optimiser_Expand - TODO: Handle node class %i\n", Node->Type >> 8);
		break;
	}
}
