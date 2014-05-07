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

#define REG_VOID	0

// === IMPORTS ===
extern void	CompileError(tAST_Node *Node, const char *format, ...);
extern void	CompileWarning(tAST_Node *Node, const char *format, ...);

// === TYPES ===
typedef struct sCompileState	tCompileState;
typedef tIRMReg	tReg;

struct sCompileState
{
	tIRMHandle	Handle;
};

// === PROTOTYPES ===
 int	Compile_ConvertNode(tCompileState *State, tAST_Node *Node, tReg *OutReg);
tAST_Node	*Compile_OptimiseWithValues(tCompileState *State, tAST_Node *Node);
void	Compile_InitSubState(tCompileState *ParentState, tCompileState *ChildState);
void	Compile_ClearSubState(tCompileState *ChildState);
bool	Compile_GetLocalSymbol(tCompileState *State, tReg *OutReg, const char *Name);
tReg	AllocateRegister(tCompileState *State, const tType *Type);

// === GLOBALS ===
const tType	*TYPE_CHARCONSTANT;

// === CODE ===
void Compile_ConvertFunction(tAST_Node *Node, int NArgs, const char **ArgNames, const tType **ArgTypes)
{
	if( !TYPE_CHARCONSTANT )
	{
		const tType	*type_char = Types_CreateIntegerType(true, INTSIZE_CHAR);
		const tType	*type_c_char = Types_ApplyQualifiers(type_char, QUALIFIER_CONST);
		TYPE_CHARCONSTANT = Types_CreatePointerType(type_c_char);
	}
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
		if( Compile_GetLocalSymbol(State, OutReg, Node->Symbol.Name) ) {
			// All good
		}
		else {
			tSymbol	*sym = Symbol_ResolveSymbol(Node->Symbol.Name);
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
		for( tAST_Node *stmt = Node->CodeBlock.FirstStatement; stmt; stmt = stmt->NextSibling )
		{
			if( Compile_ConvertNode(&new_state, stmt, NULL) )
				return 1;
		}
		// Clear scope
		Compile_ClearSubState(&new_state);
		break; }
	}
	
	// TOOD: Check for return?
	
	return 0;
}

tAST_Node *Compile_OptimiseWithValues(tCompileState *State, tAST_Node *Node)
{
	// Return a version of the provided node, with now known values replaced by constants (and propagated up)
	
	// HACK - Do nothing, for now
	return Node;
}

void Compile_InitSubState(tCompileState *ParentState, tCompileState *ChildState)
{
}
void Compile_ClearSubState(tCompileState *ChildState)
{
}
bool Compile_GetLocalSymbol(tCompileState *State, tReg *OutReg, const char *Name)
{
	return false;
}
tReg AllocateRegister(tCompileState *State, const tType *Type)
{
	return 0;
}

