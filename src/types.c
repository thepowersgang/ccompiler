/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * types.c - Datatype manipulation
 */
#define DEBUG	0
#include <global.h>
#include <symbol.h>
#include <string.h>
#include <assert.h>

// === GLOBALS ===
tTypedef	*gpTypedefs = NULL;
 int	giTypeCacheSize;
tType	**gpTypeCache;
 int	giFcnSigCacheSize;
tFunctionSig	**gpFcnSigCache;

// === CODE ===
tType *Types_GetTypeFromName(const char *Name, size_t Len)
{
	for( tTypedef *td = gpTypedefs; td; td = td->Next )
	{
		int cmp = strncmp(td->Name, Name, Len);
		if( cmp > 0 )
			break;
		if( cmp == 0 && strlen(td->Name) == Len )
			return td->Base;
	}
	return NULL;
}

int Types_Compare_I(const void *TP1, const void *TP2)
{
	return Types_Compare(*(void**)TP1, *(void**)TP2);
}

tType *Types_Register(const tType *Type)
{
	tType *ret = bsearch(&Type, gpTypeCache, giTypeCacheSize, sizeof(void*), Types_Compare_I);
	if(ret)
		return ret;
	
	gpTypeCache = realloc(gpTypeCache, (giTypeCacheSize+1)*sizeof(void*));
	assert(gpTypeCache);
	ret = malloc( sizeof(tType) );
	*ret = *Type;
	gpTypeCache[giTypeCacheSize] = ret;
	giTypeCacheSize ++;

	qsort(gpTypeCache, giTypeCacheSize, sizeof(void*), Types_Compare_I);

	return ret;
}

void Types_DerefType(tType *Type)
{
	// Maybe noop?
}

size_t max_size_t(size_t a, size_t b)
{
	return (a < b ? b : a);
}

size_t Types_GetSizeOf(const tType *Type)
{
	// TODO: machine sizes
	switch(Type->Class)
	{
	case TYPECLASS_VOID:
		return 0;
	case TYPECLASS_INTEGER:
		switch(Type->Integer.Size)
		{
		case INTSIZE_CHAR:	return 1;
		case INTSIZE_SHORT:	return 2;
		case INTSIZE_INT:	return 4;
		case INTSIZE_LONG:	return 4;
		case INTSIZE_LONGLONG:	return 8;
		}
		break;
	case TYPECLASS_REAL:
		switch(Type->Real.Size)
		{
		case FLOATSIZE_FLOAT:
			return 4;
		case FLOATSIZE_DOUBLE:
			return 8;
		case FLOATSIZE_LONGDOUBLE:
			return 10;
		}
		break;
	case TYPECLASS_POINTER:
		return 4;
	case TYPECLASS_ARRAY:
		return Type->Array.Count * Types_GetSizeOf(Type->Array.Type);
	case TYPECLASS_STRUCTURE: {
		size_t	ret = 0;
		for(int i = 0; i < Type->StructUnion->NumElements; i ++ )
			ret += Types_GetSizeOf(Type->StructUnion->Elements[i].Type);
		return ret; }
	case TYPECLASS_UNION: {
		size_t	ret = 0;
		for(int i = 0; i < Type->StructUnion->NumElements; i ++ )
			ret = max_size_t(ret, Types_GetSizeOf(Type->StructUnion->Elements[i].Type));
		return ret; }
	case TYPECLASS_FUNCTION:
		// Function can't be 'sizeof'd
		return 0;
	}
	return 0;
}

tType *Types_CreateVoid(void)
{
	tType	ret = {.Class=TYPECLASS_VOID};
	return Types_Register(&ret);
}

tType *Types_CreateIntegerType(bool bSigned, enum eIntegerSize Size)
{
	tType	ret = {
		.Class = TYPECLASS_INTEGER,
		.Integer = {
			.bSigned = bSigned,
			.Size = Size
			}
		};
	
	return Types_Register(&ret);
}

tType *Types_CreateFloatType(enum eFloatSize Size)
{
	tType	ret = {
		.Class = TYPECLASS_REAL,
		.Real = {.Size = Size}
		};
	
	return Types_Register(&ret);
}

tType *Types_CreatePointerType(const tType *TgtType)
{
	tType ret = {
		.Class = TYPECLASS_POINTER,
		.Pointer = TgtType
		};
	return Types_Register(&ret);
}

int Types_CompareFcn_I(const void *TP1, const void *TP2)
{
	return Types_CompareFcn(*(void**)TP1, *(void**)TP2);
}

tFunctionSig *Types_int_GetFunctionSig(const tType *Return, bool VariableArgs, int NArgs, const tType **ArgTypes)
{
	struct {
		tFunctionSig	sig;
		const tType	args[NArgs];
	} key;
	void	*keyptr = &key;
	
	key.sig.Return = Return;
	key.sig.bIsVarg = VariableArgs;
	key.sig.nArgs = NArgs;
	for(int i = 0; i < NArgs; i ++)
		key.sig.ArgTypes[i] = ArgTypes[i];
	
	
	tFunctionSig *ret = bsearch(&keyptr, gpFcnSigCache, giFcnSigCacheSize, sizeof(void*), Types_CompareFcn_I);
	if(ret)
		return ret;
	
	gpFcnSigCache = realloc(gpFcnSigCache, (giFcnSigCacheSize+1)*sizeof(void*));
	assert(gpFcnSigCache);
	size_t	size = sizeof(tFunctionSig) + sizeof(const tType*)*NArgs;
	ret = malloc( size );
	memcpy( ret, &key, size);
	gpFcnSigCache[giFcnSigCacheSize] = ret;
	giFcnSigCacheSize ++;

	qsort(gpFcnSigCache, giFcnSigCacheSize, sizeof(void*), Types_CompareFcn_I);

	return ret;
}

tType *Types_CreateFunctionType(const tType *Return, bool VariableArgs, int NArgs, const tType **ArgTypes)
{
	tFunctionSig	*fsig = Types_int_GetFunctionSig(Return, VariableArgs, NArgs, ArgTypes);
	tType ret = {
		.Class = TYPECLASS_FUNCTION,
		.Function = fsig
	};
	return Types_Register(&ret);
}

tType *Types_ApplyQualifiers(const tType *SrcType, unsigned int Qualifiers)
{
	tType ret = *SrcType;
	ret.bConst    = !!(Qualifiers & QUALIFIER_CONST);
	ret.bRestrict = !!(Qualifiers & QUALIFIER_RESTRICT);
	ret.bVolatile = !!(Qualifiers & QUALIFIER_VOLATILE);
	return Types_Register(&ret);
}

int Types_CompareFcn(const tFunctionSig *S1, const tFunctionSig *S2)
{
	if( S1 == S2 )	return 0;
	 int	rv;
	#define CMPFLD(fld)	if(S1->fld != S2->fld) return (S1->fld - S2->fld)
	#define CMPEXT(fcn, fld)	rv = fcn(S1->fld, S2->fld); if(rv) return rv
	
	CMPFLD(nArgs);
	CMPFLD(bIsVarg);
	CMPEXT(Types_Compare, Return);
	for( int i = 0; i < S1->nArgs; i ++ )
	{
		CMPEXT(Types_Compare, ArgTypes[i]);
	}

	// assert(S1 == S2);
	return 0;
	#undef CMPFLD
	#undef CMPEXT
}

int Types_Compare(const tType *T1, const tType *T2)
{
	if( T1 == T2 )	return 0;
	 int	rv;
	#define CMPFLD(fld)	if(T1->fld != T2->fld) return (T1->fld - T2->fld)
	#define CMPEXT(fcn, fld)	rv = fcn(T1->fld, T2->fld); if(rv) return rv
	CMPFLD(Class);
	CMPFLD(bConst);
	CMPFLD(bRestrict);
	CMPFLD(bVolatile);
	switch(T1->Class)
	{
	case TYPECLASS_VOID:
		break;
	case TYPECLASS_INTEGER:
		CMPFLD(Integer.bSigned);
		CMPFLD(Integer.Size);
		break;
	case TYPECLASS_POINTER:
		CMPEXT(Types_Compare, Pointer);
		break;
	case TYPECLASS_ARRAY:
		CMPEXT(Types_Compare, Array.Type);
		// TODO: Compare sizes
		break;
	case TYPECLASS_REAL:
		CMPFLD(Real.Size);
		break;
	case TYPECLASS_STRUCTURE:
	case TYPECLASS_UNION:
		CMPEXT(strcmp, StructUnion->Name);
		break;
	case TYPECLASS_FUNCTION:
		CMPEXT(Types_CompareFcn, Function);
		assert(!"TODO: Function types");
		break;
	}
	// assert(T1 == T2);
	return 0;
	#undef CMPFLD
	#undef CMPEXT
}
