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

tType *Types_ApplyQualifiers(const tType *SrcType, unsigned int Qualifiers)
{
	tType ret = *SrcType;
	ret.bConst    = !!(Qualifiers & QUALIFIER_CONST);
	ret.bRestrict = !!(Qualifiers & QUALIFIER_RESTRICT);
	ret.bVolatile = !!(Qualifiers & QUALIFIER_VOLATILE);
	return Types_Register(&ret);
}

int Types_Compare(const tType *T1, const tType *T2)
{
	 int	rv;
	#define CMPFLD(fld)	if(T1->fld != T2->fld) return (T1->fld - T2->fld)
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
		rv = Types_Compare(T1->Pointer, T2->Pointer);
		if(rv)	return rv;
		break;
	case TYPECLASS_ARRAY:
		rv = Types_Compare(T1->Array.Type, T2->Array.Type);
		if(rv)	return rv;
		break;
	case TYPECLASS_REAL:
		CMPFLD(Real.Size);
		break;
	case TYPECLASS_STRUCTURE:
	case TYPECLASS_UNION:
		rv = strcmp(T1->StructUnion->Name, T2->StructUnion->Name);
		if(rv)	return rv;
		break;
	case TYPECLASS_FUNCTION:
		assert(!"TODO: Function types");
		break;
	}
	return 0;
	#undef CMPFLD
}
