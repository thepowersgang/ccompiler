/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * types.h - Type manipulation
 */
#ifndef _TYPES_H_
#define _TYPES_H_

typedef struct sType	tType;
typedef struct sFunctionSig	tFunctionSig;
typedef struct sTypedef	tTypedef;

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

#define QUALIFIER_CONST   	0x01
#define QUALIFIER_RESTRICT	0x02
#define QUALIFIER_VOLATILE	0x04

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
		tFunctionSig	*Function;
	};
};

struct sFunctionSig
{
	struct sFunctionSig	*Next;
	const tType	*Return;
	bool	bIsVarg;
	 int	nArgs;
	const tType	*ArgTypes[];
};

struct sTypedef
{
	tTypedef	*Next;
	const char	*Name;
	tType	*Base;
};


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
extern tType	*Types_CreateFunctionType(const tType *Return, bool bIsVarg, int NArgs, const tType **ArgTypes);

extern int	Types_Compare(const tType *T1, const tType *T2);
extern int	Types_CompareFcn(const tFunctionSig *S1, const tFunctionSig *S2);

#endif

