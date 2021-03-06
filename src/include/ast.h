/*
 * UDI2 Bytecode Compiler
 * - By John Hodge (thePowersGang)
 */
#ifndef _AST_H_
#define _AST_H_

typedef struct sAST_Node	tAST_Node;
#include <stdbool.h>

#include <stddef.h>
#include <stdint.h>
#include <symbol.h>
#include <types.h>

enum eAST_NodeTypes
{
	NODETYPE_NULL,
	NODETYPE_NOOP,
	
	//  2 -- Leaves --
	NODETYPE_FLOAT,
	NODETYPE_INTEGER,
	NODETYPE_LOCALVAR,
	NODETYPE_SYMBOL,
	NODETYPE_STRING,

	//  7 -- Branches --
	NODETYPE_BLOCK,	// Code Block { <code>; }
	NODETYPE_FUNCTIONCALL,	// <fcn>(<args>)
	
	//  9 Statements
	NODETYPE_IF,	// "if" statement
	NODETYPE_FOR,	// "for" statement
	NODETYPE_WHILE,	// "while" statement
	NODETYPE_DOWHILE,	// "do{}while" statement
	NODETYPE_SWITCH,	// "switch" statement
	NODETYPE_CASE,	// "case/default"
	NODETYPE_RETURN,	// return <value>;
	NODETYPE_BREAK,
	NODETYPE_CONTINUE,

	// 18 Unary Operations
	NODETYPE_NEGATE,
	NODETYPE_BWNOT,
	NODETYPE_LOGICNOT,
	NODETYPE_DEREF,
	NODETYPE_ADDROF,

	NODETYPE_POSTINC,
	NODETYPE_POSTDEC,
	NODETYPE_PREINC,
	NODETYPE_PREDEC,
	
	NODETYPE_CAST,

	// 28 Binary Operations
	NODETYPE_ASSIGNOP,	//!< Special
	NODETYPE_ASSIGN,	//!< Special
	NODETYPE_INDEX,	//!< Special
	NODETYPE_MEMBER,	//!< Special

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
	
	NODETYPE_CONDITIONAL,
};

struct sAST_Node
{
	enum eAST_NodeTypes	Type;

	 int	Line;
	struct sAST_Node	*NextSibling;	//!< Valid for Code Blocks and Function Calls

	union
	{
		// Leaves
		struct {
			uint64_t	Value;
			const tType	*Type;
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
			const char	*Name;	//! Resolved in code generation, but checked on compilation
		}	Symbol;

		struct {
			struct sAST_Node	*Struct;
			const char	*Name;
		}	Member;
		
		struct {
			struct sAST_Node	*Value;
			const tType	*Type;
		}	Cast;
		
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
			struct sAST_Node	*Condition;
			struct sAST_Node	*FirstStatement;
			struct sAST_Node	*LastStatement;
		}	Switch;
		struct {
			struct sAST_Node	*Value1;
			struct sAST_Node	*Value2;
		}	SwitchCase;
		
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

};

extern void	AST_DumpTree(tAST_Node *Node, int Depth);
extern tAST_Node	*AST_NewNode(int Type);
#define AST_FreeNode	AST_DeleteNode
extern void	AST_DeleteNode(tAST_Node *Node);
extern tAST_Node	*AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);
extern tAST_Node	*AST_NewNoOp(void);
extern tAST_Node	*AST_NewCodeBlock(void);
extern tAST_Node	*AST_NewIf(tAST_Node *Test, tAST_Node *True, tAST_Node *False);
extern tAST_Node	*AST_NewSwitch(tAST_Node *Expr);
extern tAST_Node	*AST_NewCase(tAST_Node *First, tAST_Node *Last);
extern tAST_Node	*AST_NewWhile(tAST_Node *Test, tAST_Node *Code);
extern tAST_Node	*AST_NewDoWhile(tAST_Node *Test, tAST_Node *Code);
extern tAST_Node	*AST_NewFor(tAST_Node *Init, tAST_Node *Test, tAST_Node *Inc, tAST_Node *Code);

extern tAST_Node	*AST_NewFunctionCall(tAST_Node *Function);
extern tAST_Node	*AST_NewAssign(tAST_Node *To, tAST_Node *From);
extern tAST_Node	*AST_NewAssignOp(tAST_Node *To, int Op, tAST_Node *From);
extern tAST_Node	*AST_NewBinOp(int Op, tAST_Node *Left, tAST_Node *Right);
extern tAST_Node	*AST_NewUniOp(int Op, tAST_Node *Value);
extern tAST_Node	*AST_NewConditional(tAST_Node *Condition, tAST_Node *TrueVal, tAST_Node *FalseVal);
extern tAST_Node	*AST_NewCast(const tType *Type, tAST_Node *Value);
extern tAST_Node	*AST_NewSymbol(const char *Name);
extern tAST_Node	*AST_NewLocalVar(tSymbol *Sym);
extern tAST_Node	*AST_NewString(void *Data, size_t Length);
extern tAST_Node	*AST_NewInteger(uint64_t Value);
extern tAST_Node	*AST_NewArrayIndex(tAST_Node *Var, tAST_Node *Index);
extern tAST_Node	*AST_NewMember(tAST_Node *Struct, const char *Name, size_t NameLen);

#endif
