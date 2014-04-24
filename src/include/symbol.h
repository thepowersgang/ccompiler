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

typedef struct sType	tType;
typedef struct sTypedef	tTypedef;
typedef struct sSymbol	tSymbol;
typedef struct sCodeBlock	tCodeBlock;
typedef struct sFunction	tFunction;
typedef struct sStruct	tStruct;

#include <ast.h>

// === CONSTANTS ===
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

enum eTypeClass
{
	TYPECLASS_VOID,
	TYPECLASS_POINTER,
	TYPECLASS_ARRAY,
	TYPECLASS_INTEGER,
	TYPECLASS_REAL,
	TYPECLASS_STRUCTURE,
	TYPECLASS_UNION,
	TYPECLASS_FUNCTION,
};

enum eIntegerSize
{
	INTSIZE_CHAR,
	INTSIZE_SHORT,
	INTSIZE_INT,
	INTSIZE_LONG,
	INTSIZE_LONGLONG,
};

enum eFloatSize
{
	FLOATSIZE_FLOAT,
	// TODO: Others?
	FLOATSIZE_DOUBLE,
	FLOATSIZE_LONGDOUBLE,
};

enum eStorageClass
{
	STORAGECLASS_NORMAL,
	STORAGECLASS_REGISTER,
	STORAGECLASS_STATIC,
	STORAGECLASS_EXTERN,
	STORAGECLASS_AUTO,
};

#define QUALIFIER_CONST   	0x01
#define QUALIFIER_RESTRICT	0x02
#define QUALIFIER_VOLATILE	0x04

enum eLinkage
{
	LINKAGE_GLOBAL,	// exported global
	LINKAGE_STATIC,	// unexported global
	LINKAGE_EXTERNAL,	// imported global (linkage changed if real definition seen)
};

// === STRUCTURES ===
struct sType
{
	tType	*Next;
	 int	ReferenceCount;
	enum eTypeClass	Class;	
	bool	bConst;
	bool	bVolatile;
	bool	bRestrict;
	union
	{
		struct {
			bool	bSigned;
			enum eIntegerSize	Size;
		} Integer;
		struct {
			enum eFloatSize	Size;
		} Real;
		tStruct	*StructUnion;
		const tType	*Pointer;
		struct {
			size_t	Count;
			tType *Type;
		} Array;
		// TODO: Function types
		//tFunctionSig	*Function;
	};
	//size_t	Size;	// Size in words (pointers)
};

struct sTypedef
{
	tTypedef	*Next;
	const char	*Name;
	tType	*Base;
};

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
	
	 int	Line;
	
	tSymbol	Sym;
	tType	*Return;

	 int	bVaArgs;

	tSymbol	*Arguments;
	 int	CurArgSize;
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

extern tType	*Types_GetTypeFromName(const char *Name, size_t Len);

//! \brief Register a type on the global type list
extern tType	*Types_Register(const tType *Type);
//! \brief Create a pointer to the passed type
extern tType	*Types_CreatePointerType(const tType *Type);
//! \brief Create a (non-)constant version of the passed type
extern tType	*Types_ApplyQualifiers(const tType *Type, unsigned int Qualifiers);
//! \brief Dereference a type
extern void	Types_DerefType(tType *Type);

//! \brief Get the size of a type in memory
extern size_t	Types_GetSizeOf(const tType *Type);

extern tType	*Types_CreateVoid(void);
extern tType	*Types_CreateIntegerType(bool bSigned, enum eIntegerSize Size);
extern tType	*Types_CreateFloatType(enum eFloatSize Size);

extern int	Types_Compare(const tType *T1, const tType *T2);

extern tType	*Symbol_ParseStruct(char *Name);
extern tType	*Symbol_GetStruct(char *Name);
extern tType	*Symbol_ParseUnion(char *Name);
extern tType	*Symbol_GetUnion(char *Name);
extern tType	*Symbol_ParseEnum(char *Name);
extern tType	*Symbol_GetEnum(char *Name);

extern tSymbol	*Symbol_GetLocalVariable(char *Name);
extern tSymbol	*Symbol_ResolveSymbol(char *Name);
extern  int	Symbol_AddGlobalVariable(const tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *InitValue);

extern void	Symbol_SetFunction(tFunction *Fcn);
extern void	Symbol_SetFunctionCode(tFunction *Fcn, void *Block);

extern int	Symbol_GetSymClass(tSymbol *Symbol);
extern void	*Symbol_GetFunction(tType *Return, char *Name);
extern void	Symbol_SetFunctionVariableArgs(tFunction *Func);
extern void	Symbol_SetArgument(tFunction *Func, int ID, tType *Type, char *Name);

#endif
