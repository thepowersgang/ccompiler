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
#define DEBUG_ENABLED
#include <global.h>
#include <ast.h>
#include <optimiser.h>

// === CODE ===
tAST_Node *Opt1_Optimise(tAST_Node *Node)
{
	uint64_t	val;
	
	DEBUG("Node=%p{%i}", Node, Node->Type);

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
	
	case NODETYPE_CAST:
		Node->Cast.Value = Opt1_Optimise(Node->Cast.Value);
		if( Node->Cast.Value->Type != NODETYPE_INTEGER )	return Node;
		val = Node->Cast.Value->Integer.Value;
		switch(Node->Cast.Type->Class)
		{
		case TYPECLASS_INTEGER:
			switch(Node->Cast.Type->Integer.Size)
			{
			case INTSIZE_CHAR:	val &= 0xFF;
			case INTSIZE_SHORT:	val &= 0xFFFF;
			case INTSIZE_INT:	val &= 0xFFFFFFFF;	break;
			case INTSIZE_LONG:	val &= 0xFFFFFFFF;	break;
			case INTSIZE_LONGLONG:	break;
			}
			break;
		default:	return Node;
		}
		goto unaryop_common;
	
	unaryop_common:
		AST_DeleteNode(Node->UniOp.Value);
		Node->Type = NODETYPE_INTEGER;
		Node->Integer.Value = val;
		break;
	
	// -- Binary Operations
	#define OPT_BINOP(op) \
		Node->BinOp.Left = Opt1_Optimise(Node->BinOp.Left);\
		Node->BinOp.Right = Opt1_Optimise(Node->BinOp.Right);\
		if( Node->BinOp.Left->Type != NODETYPE_INTEGER )	return Node; \
		if( Node->BinOp.Right->Type != NODETYPE_INTEGER )	return Node; \
		val = Node->BinOp.Left->Integer.Value op Node->BinOp.Right->Integer.Value; \
		DEBUG("> %li %s %li = %li", Node->BinOp.Left->Integer.Value, #op, Node->BinOp.Right->Integer.Value, val);\
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
