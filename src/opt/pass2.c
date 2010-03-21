/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * optimiser/pass2.c - Pass 2 Static Optimiser
 * 
 * Optimises out compile-time determinied IF statements
 */
#include <global.h>
#include <ast.h>
#include <optimiser.h>

// === CODE ===
tAST_Node *Opt2_Optimise(tAST_Node *Node)
{
	tAST_Node	*ret;
	switch(Node->Type)
	{
	case NODETYPE_IF:
		if( Node->If.Test->Type != NODETYPE_INTEGER )	return Node;
		break;
	
	default:
		return Node;
	}
	
	if( Node->If.Test->Integer.Value )
	{
		ret = Node->If.True;
		Node->If.True = NULL;
	}
	else
	{
		ret = Node->If.False;
		Node->If.False = NULL;
	}
	return ret;
}
