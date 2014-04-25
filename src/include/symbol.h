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

typedef struct sSymbol	tSymbol;
typedef struct sCodeBlock	tCodeBlock;
typedef struct sFunction	tFunction;
typedef struct sStruct	tStruct;

#include <ast.h>
#include <types.h>

// === CONSTANTS ===
enum eStorageClass
{
	STORAGECLASS_NORMAL,
	STORAGECLASS_REGISTER,
	STORAGECLASS_STATIC,
	STORAGECLASS_EXTERN,
	STORAGECLASS_AUTO,
};

enum eLinkage
{
	LINKAGE_GLOBAL,	// exported global
	LINKAGE_STATIC,	// unexported global
	LINKAGE_EXTERNAL,	// imported global (linkage changed if real definition seen)
};

// === STRUCTURES ===
struct sSymbol
{
	struct sSymbol	*Next;
	enum eLinkage	Linkage;
	
	const char	*Name;
	const tType	*Type;
	
	 int	Line;

	
	 int	Offset;
	
	tAST_Node	*Value;
};

struct sCodeBlock
{
	struct sCodeBlock	*Parent;
	tSymbol	*LocalVariables;
};

struct sFunction
{
	struct sFunction	*Next;
	
	enum eLinkage	Linkage;
	tSymbol	Sym;

	const char *ArgNames[];
};

struct sStruct
{
	struct sStruct	*Next;
	tType	*Type;
	char	*Name;
	 int	NumElements;
	struct {
		tType	*Type;
		char	*Name;
	}	*Elements;
};

// === GLOBALS ===
extern tFunction	*gpFunctions;
extern tSymbol	*gpGlobalSymbols;

// === FUNCTIONS ===
extern void	Symbol_EnterBlock(void);
extern void	Symbol_LeaveBlock(void);


extern tType	*Symbol_ParseStruct(char *Name);
extern tType	*Symbol_GetStruct(char *Name);
extern tType	*Symbol_ParseUnion(char *Name);
extern tType	*Symbol_GetUnion(char *Name);
extern tType	*Symbol_ParseEnum(char *Name);
extern tType	*Symbol_GetEnum(char *Name);

extern tSymbol	*Symbol_GetLocalVariable(char *Name);
extern tSymbol	*Symbol_ResolveSymbol(char *Name);
extern  int	Symbol_AddGlobalVariable(const tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *InitValue);

extern  int	Symbol_AddFunction(const tType *FcnType, enum eLinkage Linkage, const char *Name, tAST_Node *Code);

extern void	Symbol_SetFunction(tFunction *Fcn);
extern void	Symbol_SetFunctionCode(tFunction *Fcn, void *Block);

extern int	Symbol_GetSymClass(tSymbol *Symbol);
extern void	*Symbol_GetFunction(tType *Return, char *Name);

#endif
