/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * types.c - Datatype manipulation
 */
#define DEBUG_ENABLED
#include <global.h>
#include <symbol.h>
#include <string.h>
#include <assert.h>

// === PROTOTYPES ===

// === GLOBALS ===
tTypedef	*gpTypedefs = NULL;
 int	giTypeCacheSize;
tType	**gpTypeCache;
 int	giFcnSigCacheSize;
tFunctionSig	**gpFcnSigCache;
tStruct	*gpStructures;
tStruct	*gpUnions;
tEnumValue	*gaEnumValues;
tEnum	*gpEnums;

// === CODE ===
const tType *Types_GetTypeFromName(const char *Name, size_t Len)
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

int Types_RegisterTypedef(const char *Name, size_t NameLen, const tType *Type)
{
	DEBUG_NL("(Name=%.*s, Type=", (int)NameLen, Name);
	IF_DEBUG( Types_Print(stdout, Type) );
	DEBUG_S(")\n");
	tTypedef **pnp = &gpTypedefs;
	for( tTypedef *td = gpTypedefs; td; pnp = &td->Next, td = td->Next )
	{
		int cmp = strncmp(td->Name, Name, NameLen);
		if( cmp > 0 )
			break;
		if( cmp == 0 && strlen(td->Name) == NameLen )
		{
			if( Types_Compare(td->Base, Type) != 0 ) {
				// Error! Incompatible redefinition
				return -1;
			}
			// Compatible redefinition
			return 1;
		}
	}
	
	tTypedef *td = malloc( sizeof(tTypedef) + NameLen + 1 );
	td->Name = (char*)(td + 1);
	memcpy(td+1, Name, NameLen);
	((char*)(td+1))[NameLen] = 0;
	td->Base = Type;
	
	td->Next = *pnp;
	*pnp = td;
	
	return 0;
}

int Types_Compare_I(const void *TP1, const void *TP2)
{
	return Types_Compare(*(void**)TP1, *(void**)TP2);
}

tType *Types_Register(const tType *Type)
{
	DEBUG("(Type=%p)", Type);
	DEBUG_NL("Type=");
	IF_DEBUG( Types_Print(stdout, Type) );
	DEBUG_S("\n");
	tType **retp = bsearch(&Type, gpTypeCache, giTypeCacheSize, sizeof(void*), Types_Compare_I);
	if(retp) {
		assert(*retp);
		DEBUG("RETURN %p (cached)", *retp);
		return *retp;
	}
	
	gpTypeCache = realloc(gpTypeCache, (giTypeCacheSize+1)*sizeof(void*));
	assert(gpTypeCache);
	tType *ret = malloc( sizeof(tType) );
	*ret = *Type;
	gpTypeCache[giTypeCacheSize] = ret;
	giTypeCacheSize ++;

	qsort(gpTypeCache, giTypeCacheSize, sizeof(void*), Types_Compare_I);

	DEBUG("RETURN %p (new)", ret);
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
		for(int i = 0; i < Type->StructUnion->nFields; i ++ )
			ret += Types_GetSizeOf(Type->StructUnion->Entries[i].Type);
		return ret; }
	case TYPECLASS_UNION: {
		size_t	ret = 0;
		for(int i = 0; i < Type->StructUnion->nFields; i ++ )
			ret = max_size_t(ret, Types_GetSizeOf(Type->StructUnion->Entries[i].Type));
		return ret; }
	case TYPECLASS_ENUM:
		if( Type->Enum->Max == 0 )
			return 0;
		if( Type->Enum->Max < 256 )
			return 1;
		if( Type->Enum->Max < 256*256 )
			return 2;
		return 4;
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

tType *Types_CreateArrayType(const tType *Inner, size_t Size)
{
	tType ret = {
		.Class = TYPECLASS_ARRAY,
		.Array = {Size, Inner}
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
	
	
	tFunctionSig **ret_p = bsearch(&keyptr, gpFcnSigCache, giFcnSigCacheSize, sizeof(void*), Types_CompareFcn_I);
	if(ret_p) {
		assert(*ret_p);
		return *ret_p;
	}
	
	gpFcnSigCache = realloc(gpFcnSigCache, (giFcnSigCacheSize+1)*sizeof(void*));
	assert(gpFcnSigCache);
	size_t	size = sizeof(tFunctionSig) + sizeof(const tType*)*NArgs;
	tFunctionSig *ret = malloc( size );
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

tType *Types_CreateStructUnionType(bool IsUnion, const tStruct *StructUnion)
{
	tType ret = {
		.Class = (IsUnion ? TYPECLASS_UNION : TYPECLASS_STRUCTURE),
		.StructUnion = StructUnion
	};
	return Types_Register(&ret);
}
tType *Types_CreateEnumType(const tEnum *Enum)
{
	tType ret = {
		.Class = TYPECLASS_ENUM,
		.Enum = Enum
	};
	return Types_Register(&ret);
}

tStruct *Types_GetStructUnion(bool IsUnion, const char *Tag, bool Create)
{
	tStruct	**head = (IsUnion ? &gpUnions : &gpStructures);
	// Search cache for a structure with this tag
	if( Tag )
	{
		for( tStruct *ele = *head; ele; ele = ele->Next )
		{
			if(ele->Tag && strcmp(ele->Tag, Tag) == 0)
			{
				return ele;
			}
		}
	}
	
	if( !Create )
		return NULL;
	
	tStruct	*ret = calloc( 1, sizeof(tStruct) + (Tag ? strlen(Tag)+1 : 0) );
	
	if( Tag ) {
		ret->Tag = (char*)(ret + 1);
		strcpy(ret->Tag, Tag);
	}
	else {
		ret->Tag = NULL;
	}
	
	ret->Next = *head;
	*head = ret;
	
	return ret;
}

int Types_AddStructField(tStruct *StructUnion, const tType *Type, const char *Name)
{
	if( Name ) {
		for( int i = 0; i < StructUnion->nFields; i ++ ) {
			if( strcmp(StructUnion->Entries[i].Name, Name) == 0 )
				return 1;
		}
	}
	void *tmp = realloc(StructUnion->Entries, (StructUnion->nFields+1)*sizeof(*StructUnion->Entries));
	if(!tmp)	return 2;
	StructUnion->Entries = tmp;
	
	StructUnion->Entries[StructUnion->nFields].Name = (Name ? strdup(Name) : NULL);
	StructUnion->Entries[StructUnion->nFields].Type = Type;
	StructUnion->nFields += 1;
	return 0;
}

tEnum *Types_GetEnum(const char *Tag, bool Create)
{
	// Search cache for a structure with this tag
	if( Tag )
	{
		for( tEnum *ele = gpEnums; ele; ele = ele->Next )
		{
			if(ele->Tag && strcmp(ele->Tag, Tag) == 0)
			{
				return ele;
			}
		}
	}
	
	if( !Create )
		return NULL;
	
	tEnum	*ret = calloc( 1, sizeof(tEnum) + (Tag ? strlen(Tag)+1 : 0) );
	
	if( Tag ) {
		ret->Tag = (char*)(ret + 1);
		strcpy(ret->Tag, Tag);
	}
	else {
		ret->Tag = NULL;
	}
	
	ret->Next = gpEnums;
	gpEnums = ret;
	
	return ret;
}
int Types_AddEnumValue(tEnum *Enum, const char *Name, uint64_t Value)
{
	//TODO("AddEnumValue");
	return 1;
}

const tType *Types_ApplyQualifiers(const tType *SrcType, unsigned int Qualifiers)
{
	DEBUG("(SrcType=%p, Qualifiers=%u)", SrcType, Qualifiers);
	if( Qualifiers == 0 )
		return SrcType;
	tType ret = *SrcType;
	ret.bConst    = !!(Qualifiers & QUALIFIER_CONST);
	ret.bRestrict = !!(Qualifiers & QUALIFIER_RESTRICT);
	ret.bVolatile = !!(Qualifiers & QUALIFIER_VOLATILE);
	return Types_Register(&ret);
}

const tType *Types_Merge(const tType *Outer, const tType *Inner)
{
	tType	ret;
	switch( Outer->Class )
	{
	case TYPECLASS_VOID:
		return Inner;
	case TYPECLASS_POINTER:
		ret = *Outer;
		ret.Pointer = Types_Merge(Outer->Pointer, Inner);
		break;
	case TYPECLASS_ARRAY:
		ret = *Outer;
		ret.Array.Type = Types_Merge(Outer->Array.Type, Inner);
		break;
	default:
		assert(!"Types_Merge called on invalid type");
		return NULL;
	}
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
		CMPFLD(StructUnion);
		break;
	case TYPECLASS_ENUM:
		CMPFLD(Enum);
		break;
	case TYPECLASS_FUNCTION:
		CMPEXT(Types_CompareFcn, Function);
		break;
	}
	// assert(T1 == T2);
	return 0;
	#undef CMPFLD
	#undef CMPEXT
}

void Types_Print(FILE *fp, const tType *Type)
{
	//fprintf(fp,"(");
	if(Type->bConst)	fprintf(fp,"const ");
	if(Type->bRestrict)	fprintf(fp,"restrict ");
	if(Type->bVolatile)	fprintf(fp,"volatile ");
	switch(Type->Class)
	{
	case TYPECLASS_VOID:	fprintf(fp,"void");	break;
	case TYPECLASS_POINTER:
		Types_Print(fp, Type->Pointer);
		fprintf(fp,"*");
		break;
	case TYPECLASS_ARRAY:
		Types_Print(fp, Type->Array.Type);
		fprintf(fp,"[%zi]", Type->Array.Count);
		break;
	case TYPECLASS_INTEGER:
		if(Type->Integer.bSigned)	fprintf(fp,"signed ");
		switch(Type->Integer.Size)
		{
		case INTSIZE_CHAR:	fprintf(fp,"char");  	break;
		case INTSIZE_SHORT:	fprintf(fp,"short"); 	break;
		case INTSIZE_INT:	fprintf(fp,"int");   	break;
		case INTSIZE_LONG:	fprintf(fp,"long");  	break;
		case INTSIZE_LONGLONG:	fprintf(fp,"long long");	break;
		}
		break;
	case TYPECLASS_REAL:
		switch(Type->Real.Size)
		{
		case FLOATSIZE_FLOAT:	fprintf(fp, "float");	break;
		case FLOATSIZE_DOUBLE:	fprintf(fp, "double");	break;
		case FLOATSIZE_LONGDOUBLE:	fprintf(fp, "long double");	break;
		}
		break;
	case TYPECLASS_STRUCTURE:	fprintf(fp, "struct %s", Type->StructUnion->Tag);	break;
	case TYPECLASS_UNION:	fprintf(fp, "union %s", Type->StructUnion->Tag);	break;
	case TYPECLASS_ENUM:	fprintf(fp, "enum %s", Type->Enum->Tag);	break;
	case TYPECLASS_FUNCTION:
		Types_Print(fp, Type->Function->Return);
		fprintf(fp, " ()(");
		for( int i = 0; i < Type->Function->nArgs; i ++ )
		{
			if(i>0)	fprintf(fp, ", ");
			Types_Print(fp, Type->Function->ArgTypes[i]);
		}
		if(Type->Function->bIsVarg)
			fprintf(fp, ", ...");
		fprintf(fp, ")");
		break;
	}
	//fprintf(fp,")");
}

