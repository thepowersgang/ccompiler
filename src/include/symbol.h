/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * symbol.h - Global and Local symbol manipulation header
 */
#ifndef _SYMBOL_H_
#define _SYMBOL_H_

enum eSymbolClasses
{
	SYMCLASS_NULL,

	SYMCLASS_VAL,
	SYMCLASS_PTR,
	SYMCLASS_FCNPTR,
	SYMCLASS_FCN,

	SYMCLASS_VOID,

	NUM_SYMCLASSES
};

// === STRUCTURES ===
typedef struct sType
{
	 int	Type;	// 0: Void, 1: Integer, 2: Structure, 3: Union
	 int	Linkage;	// 0: Def, 1: Local, 2: External
	 int	bConst;
	 int	PtrDepth;
	union
	{
		struct {
			 int	bSigned;
			 int	Bits;
		}	Integer;
	};
}	tType;

typedef struct sTypedef
{
	char	*Name;
	tType	*Base;
}	tTypedef;

typedef struct sSymbol
{
	struct sSymbol	*Next;
	 int	Class;
	tType	*Type;

	char	*Name;
}	tSymbol;

typedef struct sCodeBlock
{
	struct sCodeBlock	*Parent;
	tSymbol	*LocalVariables;
}	tCodeBlock;

typedef struct sFunction
{
	struct sFunction	*Next;
	 int	Line;
	tSymbol	Sym;
	tType	*Return;
	char	*Name;

	void	*Code;	// Actually a tAST_Node
	tSymbol	*Arguments;
}	tFunction;

// === FUNCTIONS ===
extern void	Symbol_EnterBlock(void);
extern void	Symbol_LeaveBlock(void);
extern tType	*Symbol_ResolveTypedef(char *Name, int Depth);
extern tType	*Symbol_CreateIntegralType(int bSigned, int bConst, int Linkage, int Size, int Depth);
extern tSymbol	*Symbol_GetLocalVariable(char *Name);
extern tSymbol	*Symbol_ResolveSymbol(char *Name);
extern int	Symbol_GetSymClass(tSymbol *Symbol);
extern void	*Symbol_GetFunction(tType *Return, char *Name);
extern void	Symbol_SetArgument(tFunction *Func, int ID, tType *Type, char *Name);
extern void	Symbol_SetFunction(tFunction *Fcn);
extern void	Symbol_SetFunctionCode(tFunction *Fcn, void *Block);

#endif
