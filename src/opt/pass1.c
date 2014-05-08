/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * optimiser/pass1.c - Pass 1 Static Optimiser
 * 
 * Optimises out Arithmatic and Bitwise operations on constants
 */
#include <global.h>
#include <ast.h>
#include <optimiser.h>

// === CODE ===
tAST_Node *Opt1_Optimise(tAST_Node *Node)
{
	uint64_t	val;
	
	switch(Node->Type)
	{
	// -- Unary Operations
	case NODETYPE_NEGATE:
		if( Node->UniOp.Value->Type != NODETYPE_INTEGER )	return Node;
		val = 0 - Node->UniOp.Value->Integer.Value;
		goto unaryop_common;
	case NODETYPE_BWNOT:
		if( Node->UniOp.Value->Type != NODETYPE_INTEGER )	return Node;
		val = ~Node->UniOp.Value->Integer.Value;
		goto unaryop_common;
		
	unaryop_common:
		AST_DeleteNode(Node->UniOp.Value);
		Node->Type = NODETYPE_INTEGER;
		Node->Integer.Value = val;
		break;
	
	// -- Binary Operations
	#define OPT_BINOP(op) \
		if( Node->BinOp.Left->Type != NODETYPE_INTEGER )	return Node; \
		if( Node->BinOp.Right->Type != NODETYPE_INTEGER )	return Node; \
		val = Node->BinOp.Left->Integer.Value op Node->BinOp.Right->Integer.Value; \
		goto binop_common;
	case NODETYPE_ADD:	OPT_BINOP(+)
	case NODETYPE_SUBTRACT:	OPT_BINOP(-)
	case NODETYPE_MULTIPLY:	OPT_BINOP(*)
	case NODETYPE_DIVIDE:	OPT_BINOP(/)	// TODO: Avoid #div0
	case NODETYPE_MODULO:	OPT_BINOP(%)
	
	case NODETYPE_BWOR:	OPT_BINOP(|)
	case NODETYPE_BWAND:	OPT_BINOP(&)
	case NODETYPE_BWXOR:	OPT_BINOP(^)
	case NODETYPE_BITSHIFTLEFT:	OPT_BINOP(<<);
	case NODETYPE_BITSHIFTRIGHT:	OPT_BINOP(>>);
	case NODETYPE_EQUALS:   	OPT_BINOP(==);
	case NODETYPE_NOTEQUALS:	OPT_BINOP(!=);
	
	// TODO: Signed-ness
	case NODETYPE_LESSTHAN: 	OPT_BINOP(<)
	case NODETYPE_LESSTHANEQU:	OPT_BINOP(<=)
	case NODETYPE_GREATERTHAN:	OPT_BINOP(>)
	case NODETYPE_GREATERTHANEQU:	OPT_BINOP(>=)
	case NODETYPE_BOOLOR:	OPT_BINOP(||)
	case NODETYPE_BOOLAND:	OPT_BINOP(&&)
	
	binop_common:
		AST_DeleteNode(Node->BinOp.Left);
		AST_DeleteNode(Node->BinOp.Right);
		Node->Type = NODETYPE_INTEGER;
		Node->Integer.Value = val;
		break;
	default:
		return Node;
	}
	return Node;
}
