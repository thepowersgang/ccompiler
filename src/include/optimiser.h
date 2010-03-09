/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * optimiser.h - Opimiser common header
 */
#ifndef _OPTIMISER_H_
#define _OPTIMISER_H_

typedef tAST_Node	*tOptimiseCallback(tAST_Node *Node);

extern void	Optimiser_Expand(tAST_Node *Node, tOptimiseCallback *Callback);

#endif
