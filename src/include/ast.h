/*
 * UDI2 Bytecode Compiler
 * - By John Hodge (thePowersGang)
 */
#ifndef _AST_H_
#define _ASH_H_

#include <stdint.h>
#include <symbol.h>

enum eAST_NodeTypes
{
	NODETYPE_NULL,
	
	// -- Leaves --
	NODETYPE_CAT_LEAF = 0x100,
	NODETYPE_INTEGER,
	NODETYPE_LOCALVAR,
	NODETYPE_SYMBOL,
	NODETYPE_STRING,

	// -- Branches --
	NODETYPE_CAT_LIST = 0x200,
	NODETYPE_BLOCK,	// Code Block { <code>; }
	NODETYPE_FUNCTIONCALL,	// <fcn>(<args>)
	
	// Statements
	NODETYPE_CAT_STATEMENT = 0x300,
	NODETYPE_IF,	// "if" statement
	NODETYPE_FOR,	// "for" statement
	NODETYPE_WHILE,	// "while" statement
	NODETYPE_RETURN,	// return <value>;

	// Unary Operations
	NODETYPE_CAT_UNARYOPS = 0x400,
	NODETYPE_NEGATE,
	NODETYPE_BWNOT,
	NODETYPE_DEREF,

	NODETYPE_POSTINC,
	NODETYPE_POSTDEC,
	NODETYPE_PREINC,
	NODETYPE_PREDEC,

	// Binary Operations
	NODETYPE_CAT_BINARYOPS = 0x500,
	NODETYPE_ASSIGNOP,	//!< Special
	NODETYPE_ASSIGN,	//!< Special

	NODETYPE_ADD,
	NODETYPE_SUBTRACT,
	NODETYPE_MULTIPLY,
	NODETYPE_DIVIDE,
	NODETYPE_MODULO,

	NODETYPE_BWOR,
	NODETYPE_BWAND,
	NODETYPE_BWXOR,
	NODETYPE_BITSHIFTLEFT,
	NODETYPE_BITSHIFTRIGHT,

	NODETYPE_EQUALS,
	NODETYPE_NOTEQUALS,
	NODETYPE_LESSTHAN,
	NODETYPE_LESSTHANEQU,
	NODETYPE_GREATERTHAN,
	NODETYPE_GREATERTHANEQU,
	NODETYPE_BOOLOR,
	NODETYPE_BOOLAND,

	NUM_NODETYPES
};

typedef struct sAST_Node
{
	 int	Type;

	 int	Line;
	struct sAST_Node	*NextSibling;	//!< Valid for Code Blocks and Function Calls

	union
	{
		// Leaves
		struct {
			uint64_t	Value;
		}	Integer;

		struct {
			size_t	Length;
			void	*Data;
		}	String;

		struct {
			tSymbol	*Sym;
		}	LocalVariable;

		struct {
			tSymbol	*Sym;	//! Symbol structure (NULL for unresolved)
			char	*Name;	//! Resolved in code generation, but checked on compilation
		}	Symbol;

		// Branches
		struct {
			struct sAST_Node	*To;	//!< Sanity Checked
			struct sAST_Node	*From;
		}	Assign;
		struct {
			struct sAST_Node	*To;
			struct sAST_Node	*From;
			 int	Op;
		}	AssignOp;

		struct {
			struct sAST_Node	*Value;
		}	UniOp;
		struct {
			struct sAST_Node	*Left;
			struct sAST_Node	*Right;
		}	BinOp;

		struct {
			struct sAST_Node	*Function;
			struct sAST_Node	*FirstArgument;
		}	FunctionCall;

		struct {
			struct sAST_Node	*FirstStatement;
			struct sAST_Node	*LastStatement;
		}	CodeBlock;

		struct {
			struct sAST_Node	*Test;
			struct sAST_Node	*True;
			struct sAST_Node	*False;
		}	If;

		struct {
			struct sAST_Node	*Init;	//!< First Part, Setup
			struct sAST_Node	*Test;	//!< Condition for continuing
			struct sAST_Node	*Next;	//!< Increment/Itterate
			struct sAST_Node	*Action;	//!< Body
		}	For;

		struct {
			struct sAST_Node	*Test;	//!< Condition for continuing
			struct sAST_Node	*Action;	//!< Body
		}	While;
	};

}	tAST_Node;

extern void	AST_DumpTree(tAST_Node *Node, int Depth);
extern tAST_Node	*AST_NewNode(int Type);
extern void	AST_DeleteNode(tAST_Node *Node);
extern tAST_Node	*AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);
extern tAST_Node	*AST_NewCodeBlock(void);
extern tAST_Node	*AST_NewIf(tAST_Node *Test, tAST_Node *True, tAST_Node *False);
extern tAST_Node	*AST_NewWhile(tAST_Node *Test, tAST_Node *Code);
extern tAST_Node	*AST_NewFor(tAST_Node *Init, tAST_Node *Test, tAST_Node *Inc, tAST_Node *Code);
extern tAST_Node	*AST_NewFunctionCall(tAST_Node *Function);
extern tAST_Node	*AST_NewAssign(tAST_Node *To, tAST_Node *From);
extern tAST_Node	*AST_NewAssignOp(tAST_Node *To, int Op, tAST_Node *From);
extern tAST_Node	*AST_NewBinOp(int Op, tAST_Node *Left, tAST_Node *Right);
extern tAST_Node	*AST_NewUniOp(int Op, tAST_Node *Value);
extern tAST_Node	*AST_NewSymbol(tSymbol *Sym, char *Name);
extern tAST_Node	*AST_NewLocalVar(tSymbol *Sym);
extern tAST_Node	*AST_NewString(void *Data, size_t Length);
extern tAST_Node	*AST_NewInteger(uint64_t Value);

#endif