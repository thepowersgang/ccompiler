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
	
	switch(Node->Type & 0xF00)
	{
	case NODETYPE_CAT_UNARYOPS:
		if( Node->UniOp.Value->Type != NODETYPE_INTEGER )	return Node;
		break;
	
	case NODETYPE_CAT_BINARYOPS:
		if( Node->BinOp.Left->Type != NODETYPE_INTEGER )	return Node;
		if( Node->BinOp.Right->Type != NODETYPE_INTEGER )	return Node;
		break;
	
	default:
		return Node;
	}
	
	switch(Node->Type)
	{
	// -- Unary Operations
	case NODETYPE_NEGATE:
		val = 0 - Node->UniOp.Value->Integer.Value;
		goto unaryop_common;
	case NODETYPE_BWNOT:
		val = ~Node->UniOp.Value->Integer.Value;
		goto unaryop_common;
		
	unaryop_common:
		AST_DeleteNode(Node->UniOp.Value);
		Node->Type = NODETYPE_INTEGER;
		Node->Integer.Value = val;
		break;
	
	// -- Binary Operations
	case NODETYPE_ADD:
		val = Node->BinOp.Left->Integer.Value + Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_SUBTRACT:
		val = Node->BinOp.Left->Integer.Value - Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_MULTIPLY:
		val = Node->BinOp.Left->Integer.Value * Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_DIVIDE:
		val = Node->BinOp.Left->Integer.Value / Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_MODULO:
		val = Node->BinOp.Left->Integer.Value % Node->BinOp.Right->Integer.Value;
		goto binop_common;
	
	case NODETYPE_BWOR:
		val = Node->BinOp.Left->Integer.Value | Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_BWAND:
		val = Node->BinOp.Left->Integer.Value & Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_BWXOR:
		val = Node->BinOp.Left->Integer.Value ^ Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_BITSHIFTLEFT:
		val = Node->BinOp.Left->Integer.Value << Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_BITSHIFTRIGHT:
		val = Node->BinOp.Left->Integer.Value >> Node->BinOp.Right->Integer.Value;
		goto binop_common;
	
	case NODETYPE_EQUALS:
		val = (Node->BinOp.Left->Integer.Value == Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_NOTEQUALS:
		val = (Node->BinOp.Left->Integer.Value != Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_LESSTHAN:
		val = (Node->BinOp.Left->Integer.Value < Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_LESSTHANEQU:
		val = (Node->BinOp.Left->Integer.Value <= Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_GREATERTHAN:
		val = (Node->BinOp.Left->Integer.Value > Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_GREATERTHANEQU:
		val = (Node->BinOp.Left->Integer.Value >= Node->BinOp.Right->Integer.Value);
		goto binop_common;
	case NODETYPE_BOOLOR:
		val = Node->BinOp.Left->Integer.Value || Node->BinOp.Right->Integer.Value;
		goto binop_common;
	case NODETYPE_BOOLAND:
		val = Node->BinOp.Left->Integer.Value && Node->BinOp.Right->Integer.Value;
		goto binop_common;
	
	binop_common:
		AST_DeleteNode(Node->BinOp.Left);
		AST_DeleteNode(Node->BinOp.Right);
		Node->Type = NODETYPE_INTEGER;
		Node->Integer.Value = val;
		break;
	}
	return Node;
}
