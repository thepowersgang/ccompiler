/*
 * Acess C Compiler
 * - By John Hodge (thePowersGang)
 *
 * compile.c
 * - Conversion from AST to infinite register machine
 */
#include <ast.h>
#include <symbol.h>
#include <irm.h>

// === TYPES ===
struct sCompileState
{
	
};

// === CODE ===
void Compile_ConvertFunction(tAST_Node *Node, int NArgs, const char **ArgNames, const tType **ArgTypes)
{
	// Define the arguments from a special register set
}

#define NO_RESULT()	do{ \
	if(OutReg) {\
		CompileError(Node, "Getting result of void operation");\
		return 1;\
	}\
}while(0)
#define WARN_UNUSED()	do { \
	if(!OutReg) { \
		CompileWarning(Node, "Ignoring result of operation");\
		*OutReg = REG_VOID;\
		return 0;\
	}\
}while(0)

int Compile_ConvertNode(tCompileState *State, tAST_Node *Node, tReg *OutReg)
{
	Node = Compile_OptimiseWithValues(State, Node);
	switch(Node->Type)
	{
	case NODETYPE_NOOP:
		NO_RESULT();
		break;
	// - Values
	case NODETYPE_INTEGER:
		WARN_UNUSED();
		*OutReg = AllocateRegister(State, Node->Integer.Type);
		IRM_AppendConstant(State->Handle, *OutReg, Node->Integer.Value);
		break;
	//  > String = pointer to .rodata
	case NODETYPE_STRING:
		WARN_UNUSED();
		*OutReg = AllocateRegister(State, TYPE_CHARCONSTANT);
		IRM_AppendCharacterConstant(State->Handle, *OutReg, Node->String.Length, Node->String.Data);
		break;
	// NOTE: This covers implicit dereferences, '&ptr' is handled explicitly
	case NODETYPE_SYMBOL:
		WARN_UNUSED();
		// 1. Local variable/symbol (Stored in State)
		if( GetLocalSymbol(State, OutReg, Node->Symbol.Name) ) {
			// All good
		}
		else {
			tSymbol	*sym = Symbol_LookupGlobal(Node->Symbol.Name);
			if( !sym ) {
				CompileError(Node, "Undefined reference to %s", Node->Symbol.Name);
				return 1;
			}
			*OutReg = AllocateRegister(State, sym->Type);
			IRM_AppendSymbol(State->Handle, *OutReg, sym);
		}
		break;
	
	// --- "List" Nodes
	// > Code block
	case NODETYPE_BLOCK: {
		// Create new scope
		tCompileState	new_state;
		Compile_InitSubState(State, &new_state);
		// Iterate nodes
		for( tAST_Node *stmt = Node->CodeBlock.FirstStatement; stmt; stmt = stmt->Next )
		{
			if( Compile_ConvertNode(new_state, stmt, NULL) )
				return 1;
		}
		// Clear scope
		Compile_ClearSubState(&new_state);
		break; }
	}
}

