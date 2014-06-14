/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * expr.c - Token to AST Parser
 */
#define DEBUG_ENABLED
#include "global.h"
#include <parser.h>
#include <stdio.h>
#include <strings.h>
#include <memory.h>
#include <symbol.h>
#include <ast.h>
#include <stdbool.h>
#include <assert.h>

// === MACROS ===
#define CMPTOK(str)	(strlen((str))==giTokenLength&&strncmp((str),gsTokenStart,giTokenLength)==0)
#define GETTOKSTR(name)	char name[Parser->Cur.TokenLen+1]; memcpy(name,Parser->Cur.TokenStart,Parser->Cur.TokenLen);name[Parser->Cur.TokenLen]=0

// === IMPORTS ===
extern tAST_Node	*Optimiser_StaticOpt(tAST_Node *Node);

// === Prototypes ===
void	Parse_CodeRoot(tParser *Parser);
const tType	*Parse_GetType(tParser *Parser, char **NamePtr, const tType **BaseType, char ***VarNames);
char	**Parse_DoFcnProto(tParser *Parser, const tType *Type, const tType **OutType);
 int	Parse_DoDefinition(tParser *Parser, tAST_Node *CodeNode);
 int	Parse_DoDefinition_VarActual(tParser *Parser, const tType *Type, const char *Name, tAST_Node *CodeNode);
tAST_Node	*DoCodeBlock(tParser *Parser);
tAST_Node	*DoStatement(tParser *Parser, tAST_Node *CodeNode);
tAST_Node	*DoIf(tParser *Parser);
tAST_Node	*DoWhile(tParser *Parser);
tAST_Node	*DoFor(tParser *Parser);

tAST_Node	*DoExpr0(tParser *Parser);	// Assignment
tAST_Node	*DoExpr1(tParser *Parser);	// Ternary
tAST_Node	*DoExpr2(tParser *Parser);	// Boolean
tAST_Node	*DoExpr3(tParser *Parser);	// Bitwise
tAST_Node	*DoExpr4(tParser *Parser);	// Comparison
tAST_Node	*DoExpr5(tParser *Parser);	// Shifts
tAST_Node	*DoExpr6(tParser *Parser);	// Arithmatic
tAST_Node	*DoExpr7(tParser *Parser);	// Mult/Div
tAST_Node	*DoExpr8(tParser *Parser);	// Right Unaries
tAST_Node	*DoExpr9(tParser *Parser);	// Left Unaries
tAST_Node	*DoMember(tParser *Parser);	// Member dereference
tAST_Node	*DoParen(tParser *Parser);	// 2nd Last - Parens
tAST_Node	*DoValue(tParser *Parser);	// FINAL - Values
tAST_Node	*GetString(tParser *Parser);
tAST_Node	*GetCharConst(tParser *Parser);
tAST_Node	*GetIdent(tParser *Parser);
tAST_Node	*GetNumeric(tParser *Parser);
tAST_Node	*GetSizeof(tParser *Parser);

// === CODE ===
/**
 * \brief Handle the root of the parse tree
 */
void Parse_CodeRoot(tParser *Parser)
{
	while( LookAhead(Parser) != TOK_EOF )
	{
		if( LookAhead(Parser) == TOK_RWORD_TYPEDEF )
		{
			// Type definition.
			DEBUG("++ Typedef");
			GetToken(Parser);
			char *name;
			const tType *type = Parse_GetType(Parser, &name, NULL, NULL);
			if( !type )
				return ;

			DEBUG("- typedef %p %s", type, name);
			if( !name ) {
				SyntaxError(Parser, "Expected name for typedef");
				return ;
			}
			int rv = Types_RegisterTypedef(name, strlen(name), type);
			free(name);
			if( rv < 0 ) {
				SyntaxError(Parser, "Incompatible redefinition of '%s'", name);
				return ;
			}
		
			if( SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON) )
				return ;
			DEBUG("-- Typedef");
		}
		else {
			DEBUG("++ Definition");
			// Only other option is a variable/function definition
			if( Parse_DoDefinition(Parser, NULL) )
				return ;
			DEBUG("-- Definition");
		}
	}
}

const tType *Parse_GetType_Base(tParser *Parser, enum eStorageClass *StorageClassOut)
{
	const tType	*type = NULL;
	unsigned int	qualifiers = 0;
	enum eStorageClass	storage_class = STORAGECLASS_NORMAL;
	
	bool	is_size_set = false;
	enum eIntegerSize	int_size = INTSIZE_INT;
	bool	is_signed_set = false;
	bool	is_signed = true;
	bool	is_double = false;
	bool	is_long_double = false;
	
	static const char	*const size_names[] = {"char","short","int","long","long long"};
	#define ASSERT_NO_TYPE(name)	do{if(type){SyntaxError(Parser, "Multiple types in definition (%s)",name);return NULL;}}while(0)
	#define ASSERT_NO_SIZE(name)	do{if(is_size_set){ \
		SyntaxError(Parser, "Invalid use of '%s' with '%.*s'",\
			size_names[int_size], Parser->Cur.TokenLen, Parser->Cur.TokenStart);return NULL;\
		}}while(0)
	#define ASSERT_NO_SIGN(name)	do{if(is_signed_set){ \
		SyntaxError(Parser, "Invalid use of '%ssigned' with '%.*s'", (is_signed?"":"un"));return NULL;\
		}}while(0)
	#define ASSERT_NO_STCLASS(name) do{ if(storage_class != STORAGECLASS_NORMAL) { \
		SyntaxError(Parser, ""); return NULL;\
		}}while(0)
	// 1. Get the base type
	// - Including storage classes on it
	bool getting_base_type = true;
	while( getting_base_type )
	{
		switch(GetToken(Parser))
		{
		// Signedness
		case TOK_RWORD_SIGNED:
			ASSERT_NO_SIGN("signed");
		case TOK_RWORD_UNSIGNED:
			ASSERT_NO_SIGN("unsigned");
			ASSERT_NO_TYPE("unsigned");
			is_signed_set = true;
			is_signed = (Parser->Cur.Token == TOK_RWORD_SIGNED);
			break;
		
		case TOK_RWORD_BOOL:
			TODO("bool");
			break;
		case TOK_RWORD_COMPLEX:
			TODO("complex");
			break;
	
		// void - No possible modifiers
		case TOK_RWORD_VOID:
			ASSERT_NO_TYPE("void");
			ASSERT_NO_SIZE("void");
			ASSERT_NO_SIGN("void");
			// void - we have our base type now?
			type = Types_CreateVoid();
			break;
		
		// char - allows (un)signed
		case TOK_RWORD_CHAR:
			ASSERT_NO_TYPE("char");
			ASSERT_NO_SIZE("void");
			int_size = INTSIZE_CHAR;
			is_size_set = true;
			break;
		// short - allows sign and 'int'
		case TOK_RWORD_SHORT:
			ASSERT_NO_TYPE("short");
			ASSERT_NO_SIZE("short");
			int_size = INTSIZE_SHORT;
			is_size_set = true;
			break;
		// int - Default to INTSIZE_INT if not yet specified
		// > Special catch for attempting 'int char'
		case TOK_RWORD_INT:
			ASSERT_NO_TYPE("int");
			if(!is_size_set)
				int_size = INTSIZE_INT;
			else if( int_size == INTSIZE_CHAR )
				ASSERT_NO_SIZE("int");
			else
				;
			is_size_set = true;
			break;
		// long - Special casing for:
		// > 'double long' (reorder of 'long double')
		// > 'int long' or 'long int'
		// > 'long long'
		case TOK_RWORD_LONG:
			DEBUG("long");
			ASSERT_NO_TYPE("long");
			if( is_double ) {
				if( is_long_double ) {
					SyntaxError(Parser, "Unexpected second 'long' on 'double'");
					return NULL;
				}
				is_long_double = true;
			}
			else if( !is_size_set || int_size == INTSIZE_INT )
				int_size = INTSIZE_LONG;
			else if( int_size == INTSIZE_LONG )
				int_size = INTSIZE_LONGLONG;
			else
				ASSERT_NO_SIZE("long");
			is_size_set = true;
			break;
		
		// Floating point - no possible modifiers
		case TOK_RWORD_FLOAT:
			DEBUG("float");
			ASSERT_NO_TYPE("float");
			ASSERT_NO_SIZE("float");
			ASSERT_NO_SIGN("float");
			type = Types_CreateFloatType(FLOATSIZE_FLOAT);
			break;
		// Double - Handle 'long double'
		case TOK_RWORD_DOUBLE:
			DEBUG("double");
			ASSERT_NO_TYPE("double");
			ASSERT_NO_SIGN("double");
			is_double = true;
			if( is_size_set ) {
				if( int_size != INTSIZE_LONG )
					ASSERT_NO_SIZE("double");
				is_long_double = true;
			}
			break;

		// User-defined type
		case TOK_IDENT:
			DEBUG("IDENT");
			if( type || is_size_set || is_signed_set || is_double )
			{
				DEBUG("Ident with integer/double, breaking");
				getting_base_type = false;
				PutBack(Parser);
			}
			else
			{
				ASSERT_NO_TYPE("IDENT");
				DEBUG("Ident, getting type");
				ASSERT_NO_SIGN("userdef");
				type = Types_GetTypeFromName(Parser->Cur.TokenStart, Parser->Cur.TokenLen);
				if( !type ) {
					SyntaxError(Parser, "'%.*s' does not describe a type",
						Parser->Cur.TokenLen, Parser->Cur.TokenStart);
					return NULL;
				}
			}
			break;
		
		// Compound types
		case TOK_RWORD_ENUM: {
			DEBUG("enum");
			GetToken(Parser);
			tEnum	*enum_info;
			if( Parser->Cur.Token == TOK_IDENT ) {
				// Tagged enum, definition is now optional
				// - Look up / create type descriptor
				GETTOKSTR(tag);
				enum_info = Types_GetEnum(tag, true);
				GetToken(Parser);
			}
			else if( Parser->Cur.Token != TOK_BRACE_OPEN ) {
				SyntaxError(Parser, "Expected TOK_IDENT or TOK_BRACE_OPEN after 'enum'");
				return NULL;
			}
			else {
				// fall
				enum_info = Types_GetEnum(NULL, true);
			}
			if( Parser->Cur.Token == TOK_BRACE_OPEN )
			{
				// parse enum definition
				uint64_t	val = 0;
				do {
					GetToken(Parser);
					if(Parser->Cur.Token == TOK_BRACE_CLOSE) {
						break;
					}
					
					if(SyntaxAssert(Parser, Parser->Cur.Token, TOK_IDENT))
						return NULL;
					GETTOKSTR(name);
					
					if( LookAhead(Parser) == TOK_ASSIGNEQU )
					{
						GetToken(Parser);
						if( SyntaxAssert(Parser, GetToken(Parser), TOK_CONST_NUM) )
							return NULL;
						val = Parser->Cur.Integer;
					}
					
					Types_AddEnumValue(enum_info, name, val);
					
					val ++;
				} while(GetToken(Parser) == TOK_COMMA);
				
				if( SyntaxAssert(Parser, Parser->Cur.Token, TOK_BRACE_CLOSE) )
					return NULL;
			}
			else
			{
				// Live with existing
				PutBack(Parser);
			}
			getting_base_type = false;
			return Types_CreateEnumType(enum_info); }
		case TOK_RWORD_UNION:	DEBUG("union"); if(0)
		case TOK_RWORD_STRUCT:	DEBUG("struct"); {
			tStruct	*su_info = NULL;
			bool	is_union = (Parser->Cur.Token == TOK_RWORD_UNION);
			DEBUG("struct/union");
			GetToken(Parser);
			if( Parser->Cur.Token == TOK_IDENT ) {
				// Tagged struct/union, definition is now optional
				// - Look up / create type descriptor
				DEBUG("Tagged");
				GETTOKSTR(tag);
				su_info = Types_GetStructUnion(is_union, tag, true);
				GetToken(Parser);
			}
			else if( Parser->Cur.Token != TOK_BRACE_OPEN ) {
				SyntaxError(Parser, "Expected TOK_IDENT or TOK_BRACE_OPEN after struct/union");
				return NULL;
			}
			else {
				// fall
				DEBUG("Untagged");
				su_info = Types_GetStructUnion(is_union, NULL, true);
			}
			assert(su_info);
			if( Parser->Cur.Token == TOK_BRACE_OPEN )
			{
				// parse struct definition
				if( su_info->IsPopulated ) {
					SyntaxError(Parser, "Duplicate definition of %s '%s'",
						(is_union?"union":"struct"), su_info->Tag);
					return NULL;
				}
				DEBUG("Definition");
				
				while(LookAhead(Parser) != TOK_BRACE_CLOSE)
				{
					const tType	*base_type = NULL;
					do {
						char	*name = NULL;
						const tType	*fld_type = Parse_GetType(Parser, &name, &base_type, NULL);
						if(!fld_type)
							return NULL;
						if( LookAhead(Parser) == TOK_COLON ) {
							GetToken(Parser);
							if(SyntaxAssert(Parser, GetToken(Parser), TOK_CONST_NUM)) {
								free(name);
								return NULL;
							}
							// TODO: Bitfields
						}
						if( Types_AddStructField(su_info, fld_type, name) ) {
							SyntaxError(Parser, "Duplicate definition of field '%s' of %s '%s'",
								name, (is_union?"union":"struct"), su_info->Tag);
							free(name);
							return NULL;
						}
						free(name);
					} while(GetToken(Parser) == TOK_COMMA);
					if( SyntaxAssert(Parser, Parser->Cur.Token, TOK_SEMICOLON) )
						return NULL;
				}
				GetToken(Parser);
				su_info->IsPopulated = true;
			}
			else
			{
				// Live with existing
				DEBUG("Just used");
				PutBack(Parser);
			}
			getting_base_type = false;
			type = Types_CreateStructUnionType(is_union, su_info);
			DEBUG("struct/union parsed, breaking");
			break; }
		
		// Qualifiers
		case TOK_RWORD_CONST:    qualifiers |= QUALIFIER_CONST;    break;
		case TOK_RWORD_RESTRICT: qualifiers |= QUALIFIER_RESTRICT; break;
		case TOK_RWORD_VOLATILE: qualifiers |= QUALIFIER_VOLATILE; break;
		
		// TODO: Handle inline in a non-trivial way
		case TOK_RWORD_INLINE:	break;
		
		// Storage class
		case TOK_RWORD_EXTERN:
			ASSERT_NO_STCLASS("extern");
			storage_class = STORAGECLASS_EXTERN;
			break;
		case TOK_RWORD_STATIC:
			ASSERT_NO_STCLASS("static");
			storage_class = STORAGECLASS_STATIC;
			break;
		case TOK_RWORD_AUTO:
			ASSERT_NO_STCLASS("auto");
			storage_class = STORAGECLASS_AUTO;
			break;
		case TOK_RWORD_REGISTER:
			ASSERT_NO_STCLASS("register");
			storage_class = STORAGECLASS_REGISTER;
			break;
		
		default:
			DEBUG("Unk: %i '%.*s'", Parser->Cur.Token, (int)Parser->Cur.TokenLen, Parser->Cur.TokenStart);
			getting_base_type = false;
			DEBUG("Hit unknown, putting back");
			PutBack(Parser);
			break;
		}
	}
	
	// Build type if not already built
	if( !type )
	{
		DEBUG("is_double=%i,is_signed_set=%i,is_size_set=%i,int_size=%i,is_long_double=%i",
			is_double, is_signed_set, is_size_set, int_size, is_long_double);
		if( is_double ) {
			// Floating point type
			// Some asserts to make sure nothing stupid has happened
			assert( !is_signed_set );
			assert( !is_size_set || (int_size == INTSIZE_LONG) );
			type = Types_CreateFloatType( (is_long_double ? FLOATSIZE_LONGDOUBLE : FLOATSIZE_DOUBLE) );
		}
		else if( is_size_set ) {
			// Intger type
			type = Types_CreateIntegerType( is_signed, int_size );
		}
		else {
			// No size specified, error?
			GetToken(Parser);
			SyntaxError(Parser, "Expected type");
			return NULL;
		}
	}
	
	type = Types_ApplyQualifiers(type, qualifiers);
	*StorageClassOut = storage_class;
	return type;
	#undef ASSERT_NO_TYPE
	#undef ASSERT_NO_SIZE
	#undef ASSERT_NO_SIGN
	#undef ASSERT_NO_STCLASS
}

const tType *Parse_GetType_Ext(tParser *Parser, char **NamePtr, const tType *BaseType, char ***VarNames)
{
	const tType	*upper_type = NULL;
	const tType	*type = BaseType;
	
	unsigned int qualifiers = 0;
	// 2. Handle qualifiers and pointers
	bool	loop = true;
	while( loop )
	{
		switch( GetToken(Parser) )
		{
		case TOK_ASTERISK:
			DEBUG("Pointer");
			// Pointer
			// - Determine type
			type = Types_ApplyQualifiers(type, qualifiers);
			type = Types_CreatePointerType(type);
			// - Clear
			qualifiers = 0;
			// - Continue on
			break;
		case TOK_RWORD_CONST:    DEBUG("const");	qualifiers |= QUALIFIER_CONST;    break;
		case TOK_RWORD_RESTRICT: DEBUG("restrict");	qualifiers |= QUALIFIER_RESTRICT; break;
		case TOK_RWORD_VOLATILE: DEBUG("volatile");	qualifiers |= QUALIFIER_VOLATILE; break;
		
		default:
			DEBUG("Qualifiers done");
			PutBack(Parser);
			loop = false;
			break;
		}
	}
	if( qualifiers )
	{
		type = Types_ApplyQualifiers(type, qualifiers);
		qualifiers = 0;
	}
	
	// TODO: Handle stuff like "int (*fcnptr)(void);
	// - When brackets open, qualifiers are set to apply after further resolution
	if( LookAhead(Parser) == TOK_PAREN_OPEN )
	{
		// Recurse on a void type, then recreate path replacing void type with current level.
		GetToken(Parser);
		upper_type = Parse_GetType_Ext(Parser, NamePtr, Types_CreateVoid(), VarNames);
		if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE) )
			return NULL;
	}
	// 3. Name
	else if( NamePtr )
	{
		if( LookAhead(Parser) == TOK_IDENT )
		{
			GetToken(Parser);
			GETTOKSTR(name);
			DEBUG("name=%s", name);
			*NamePtr = strdup(name);
		}
		else
		{
			DEBUG("No name");
			*NamePtr = NULL;
		}
	}
	else
	{
	}
	
	if( VarNames )
		*VarNames = NULL;
	
	// 4. Function/array
	if( LookAhead(Parser) == TOK_PAREN_OPEN )
	{
		DEBUG("Function");
		char **varnames = Parse_DoFcnProto(Parser, type, &type);
		if( qualifiers == 0 && VarNames && !*VarNames )
			*VarNames = varnames;
		else {
			if( VarNames ) {
				assert(!*VarNames);
				*VarNames = NULL;
			}
			assert( type->Class == TYPECLASS_FUNCTION );
			for( size_t i = 0; i < type->Function->nArgs; i ++ )
				free(varnames[i]);
			free(varnames);
		}
	}
	else if( LookAhead(Parser) == TOK_SQUARE_OPEN )
	{
		DEBUG("Array");
		GetToken(Parser);
		if( LookAhead(Parser) == TOK_SQUARE_CLOSE )
		{
			// Variable-sized array
			GetToken(Parser);
			type = Types_CreateArrayType(type, -1);
		}
		else
		{
			extern tAST_Node *Opt1_Optimise(tAST_Node *Node);
			tAST_Node *size = DoExpr0(Parser);
			if(SyntaxAssert(Parser, GetToken(Parser), TOK_SQUARE_CLOSE))
				return NULL;
			size = Opt1_Optimise(size);
			if( size->Type == NODETYPE_INTEGER ) {
				DEBUG("size={Value:%lli}", (long long)size->Integer.Value);
				type = Types_CreateArrayType(type, size->Integer.Value);
			}
			else {
				TODO("variable-sized arrays");
			}
		}
		while( GetToken(Parser) == TOK_SQUARE_OPEN )
		{
			// Must be a constant expression
			TODO("Multi-dimensional arrays");
			if(SyntaxAssert(Parser, GetToken(Parser), TOK_SQUARE_CLOSE))
				return NULL;
		}
		PutBack(Parser);
	}
	
	// Handle deep types
	if( upper_type )
	{
		DEBUG("Deep type handling");
		DEBUG_NL("Current: ");	IF_DEBUG(Types_Print(stdout, type));	DEBUG_S("\n");
		DEBUG_NL("Upper:   ");	IF_DEBUG(Types_Print(stdout, upper_type));	DEBUG_S("\n");
		type = Types_Merge(upper_type, type);
		DEBUG_NL("Result: ");	IF_DEBUG(Types_Print(stdout, type));	DEBUG_S("\n");
	}
	
	return type;
}

/**
 * \brief Handle storage classes, structs, unions, enums, primative types
 *
 * \param NamePtr	Output pointer in which to store strdup'd name
 * \param BaseType	Base type (before pointer application), allowed to be NULL
 */
const tType *Parse_GetType(tParser *Parser, char **NamePtr, const tType **BaseType, char ***VarNames)
{
	DEBUG("NamePtr=%p,BaseType=%p,VarNames=%p",
		NamePtr, BaseType, VarNames);
	enum eStorageClass	storage_class;
	const tType *type;
	
	if( BaseType && *BaseType )
	{
		type = *BaseType;
	}
	else
	{
		type = Parse_GetType_Base(Parser, &storage_class);
		if( !type )
			return NULL;
		if( BaseType )
			*BaseType = type;
	}

	if(VarNames)
		*VarNames = NULL;	
	type = Parse_GetType_Ext(Parser, NamePtr, type, VarNames);
	
	// TODO: storage class
	if( storage_class != STORAGECLASS_NORMAL )
		DEBUG("TODO: Handle storage class %i", storage_class);
	
	DEBUG("<<<");
	return type;
}

char **Parse_DoFcnProto(tParser *Parser, const tType *Type, const tType **OutType)
{
	const int MAX_ARGS = 16;
	 int	nArgs = 0;
	const tType	*argtypes[MAX_ARGS];
	char	*argnames[MAX_ARGS];
	bool	isVArg = false;
	GetToken(Parser);	// '('
	do {
		if( LookAhead(Parser) == TOK_VAARG ) {
			GetToken(Parser);	// eat ...
			GetToken(Parser);	// prep for check on ')'
			isVArg = true;
			break;
		}
		// Limit check
		if( nArgs >= MAX_ARGS ) {
			SyntaxError(Parser, "Too many arguments (max %i)", MAX_ARGS);
			goto _err;
		}
		
		// Get Type
		char	*varname;
		const tType *argtype = Parse_GetType(Parser, &varname, NULL, NULL);
		if(!argtype) {
			goto _err;
		}
		
		argtypes[nArgs] = argtype;
		
		// optionally get variable name
		if( LookAhead(Parser) == TOK_IDENT )
		{
			GetToken(Parser);

			GETTOKSTR(varname);
			
			argnames[nArgs] = strdup(varname);
		}
		else
		{
			argnames[nArgs] = NULL;
		}
		nArgs ++;
	} while(GetToken(Parser) == TOK_COMMA);
	
	if( SyntaxAssert(Parser, Parser->Cur.Token, TOK_PAREN_CLOSE) )
		goto _err;
	
	DEBUG("nArgs = %i, isVArg=%i", nArgs, isVArg);
	
	*OutType = Types_CreateFunctionType(Type, isVArg, nArgs, argtypes);
	assert( *OutType );
	
	char **ret = malloc( sizeof(char*) * nArgs );
	for(int i = 0; i < nArgs; i ++)
		ret[i] = argnames[i];
	return ret;
_err:
	for( int i = 0; i < nArgs; i ++ )
	{
		free(argnames[i]);
	}
	return NULL;
}

/**
 * \brief Handle variable/function definition
 */
int Parse_DoDefinition(tParser *Parser, tAST_Node *CodeNode)
{
	const tType *basetype = NULL;
	do {
		char	**argnames;
		char	*name;
		const tType *type = Parse_GetType(Parser, &name, &basetype, &argnames);
		if(!type)
			return 1;
		
		if( argnames )
		{
			if( !name ) {
				SyntaxError(Parser, "Expected name after type");
				return 1;
			}

			// if set, it was a bare function
			if( LookAhead(Parser) == TOK_BRACE_OPEN )
			{
				// Definition
				// - Add an unbound symbol first to allow for recursion
				Symbol_AddFunction(type, LINKAGE_GLOBAL, name, NULL);
				tAST_Node *code = DoCodeBlock(Parser);
				if( !code )	return 1;
				Symbol_AddFunction(type, LINKAGE_GLOBAL, name, code);
			}
			else if( LookAhead(Parser) == TOK_SEMICOLON )
			{
				GetToken(Parser);
				// prototype
				Symbol_AddFunction(type, LINKAGE_GLOBAL, name, NULL);
			}
			else
			{
				SyntaxError_T(Parser, GetToken(Parser), "Expected TOK_BRACE_OPEN or TOK_SEMICOLON");
				return 1;
			}
			// Multiple definitions in one line are not allowed for function types
			free(name);
			return 0;
		}
		else
		{
			if( name == NULL ) {
				if( type->Class == TYPECLASS_STRUCTURE || type->Class == TYPECLASS_UNION || type->Class == TYPECLASS_ENUM ) {
					// perfectly valid
					// - But, can't continue on
					GetToken(Parser);
					break;
				}
				
				SyntaxError(Parser, "Expected name after type");
				return 1;
			}
			
			if( Parse_DoDefinition_VarActual(Parser, type, name, CodeNode) ) {
				free(name);
				return 1;
			}
		}
		free(name);
	} while(GetToken(Parser) == TOK_COMMA);
	
	if( SyntaxAssert(Parser, Parser->Cur.Token, TOK_SEMICOLON) )
		return 1;
	DEBUG("<<<");
	return 0;
}

int Parse_DoDefinition_VarActual(tParser *Parser, const tType *Type, const char *Name, tAST_Node *CodeNode)
{
	DEBUG("Name='%s'", Name);
	
	// TODO: Arrays (need to propagate nodes from Parse_GetType)
	
	tAST_Node	*init_value = NULL;
	if( GetToken(Parser) == TOK_ASSIGNEQU )
	{
		if( LookAhead(Parser) == TOK_BRACE_OPEN ) {
			// Only valid if this type is a compound type (array or struct)
			TODO("Braced assign");
		}
		else {
			init_value = DoExpr0(Parser);
			if( !init_value )
				return 1;
		}
	}
	else
	{
		PutBack(Parser);
	}
	
	if( CodeNode )
	{
		//tAST_Node *ret = AST_NewVariableDef(type, name);
		//AST_AppendNode(CodeNode, ret);
		TODO("Local variables");
	}
	else
	{
		Symbol_AddGlobalVariable(Type, LINKAGE_GLOBAL, Name, init_value);
	}
	DEBUG("<<<");
	return 0;
}

/**
 * \fn tAST_Node *DoCodeBlock()
 * \brief Parses a code block, or a single statement
 */
tAST_Node *DoCodeBlock(tParser *Parser)
{
	if(LookAhead(Parser) == TOK_BRACE_OPEN)
	{
		GetToken(Parser);
		tAST_Node	*ret = AST_NewCodeBlock();
		// Parse Block
		while(LookAhead(Parser) != TOK_BRACE_CLOSE)
		{
			DEBUG("Line");
			tAST_Node *line = DoStatement(Parser, ret);
			if(!line) {
				AST_FreeNode(line);
				return NULL;
			}
			AST_AppendNode( ret, line );
		}
		GetToken(Parser);
		return ret;
	}
	else
	{
		return DoStatement(Parser, ACC_ERRPTR);
	}
}

/**
 * \brief Parses a statement _within_ a function
 */
tAST_Node *DoStatement(tParser *Parser, tAST_Node *CodeNode)
{
	tAST_Node	*ret;

	switch(LookAhead(Parser))
	{
	case TOK_EOF:
		return NULL;
	case TOK_RWORD_IF:
		GetToken(Parser);
		return DoIf(Parser);

	case TOK_RWORD_RETURN:
		GetToken(Parser);
		ret = AST_NewUniOp( NODETYPE_RETURN, DoExpr0(Parser) );
		break;
	case TOK_IDENT: {
		const tType *type = Types_GetTypeFromName(Parser->Cur.TokenStart, Parser->Cur.TokenLen);
		if( type ) {
			Parse_DoDefinition(Parser, CodeNode);
			return ACC_ERRPTR;
		}
		else {
			ret = DoExpr0(Parser);
		}
		break; }
	default:
		ret = DoExpr0(Parser);
		break;
	}
	if( SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON ) ) {
		AST_FreeNode(ret);
		return NULL;
	}
	return ret;
}

/**
 * \brief Parses an if block
 */
tAST_Node *DoIf(tParser *Parser)
{
	tAST_Node	*test = NULL, *true_code = NULL;
	tAST_Node	*false_code = NULL;

	if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN) )	// Eat (
		goto _err;
	test = DoExpr0(Parser);
	if( !test )
		goto _err;
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE) )	// Eat )
		goto _err;

	// Contents
	true_code = DoCodeBlock(Parser);
	if( !true_code )
		goto _err;

	if(GetToken(Parser) == TOK_RWORD_ELSE) {
		false_code = DoCodeBlock(Parser);
	}
	else {
		false_code = AST_NewNoOp();
		PutBack(Parser);
	}
	if( !false_code )
		goto _err;

	return AST_NewIf(test, true_code, false_code);
_err:
	AST_FreeNode(test);
	AST_FreeNode(true_code);
	AST_FreeNode(false_code);
	return NULL;
}

/**
 * \brief Parses a while block
 */
tAST_Node *DoWhile(tParser *Parser)
{
	tAST_Node	*test, *code;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);

	// Contents
	code = DoCodeBlock(Parser);

	return AST_NewWhile(test, code);
}

/**
 * \fn tAST_Node *DoFor()
 * \brief Handle FOR loops
 */
tAST_Node *DoFor(tParser *Parser)
{
	tAST_Node	*init = NULL, *test = NULL, *post = NULL, *code = NULL;

	if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN) )	// Eat (
		goto _err;

	// Initialiser (definition valid)
	// TODO: Support defining variables in initialiser
	init = DoExpr0(Parser);
	if( !init )
		goto _err;
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON) )	// Eat ;
		goto _err;

	// Condition
	test = DoExpr0(Parser);
	if( !test )
		goto _err;
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON) )	// Eat ;
		goto _err;

	// Postscript
	post = DoExpr0(Parser);
	if( !post )
		goto _err;
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE) )	// Eat )
		goto _err;

	// Contents
	code = DoCodeBlock(Parser);
	if( !code )
		goto _err;

	return AST_NewFor(init, test, post, code);
_err:
	AST_FreeNode(init);
	AST_FreeNode(test);
	AST_FreeNode(post);
	AST_FreeNode(code);
	return NULL;
}

// --------------------
// Expression 0 - Assignments
// --------------------
tAST_Node *DoExpr0(tParser *Parser)
{
	#define _next	DoExpr1
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_ASSIGNEQU:
			ret = AST_NewAssign(ret, _next(Parser));
			break;
		case TOK_DIV_EQU:
			ret = AST_NewAssignOp(ret, NODETYPE_DIVIDE, _next(Parser));
			break;
		case TOK_MULT_EQU:
			ret = AST_NewAssignOp(ret, NODETYPE_MULTIPLY, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 1 - Ternary
// TODO: Implement Ternary Operator
// --------------------
tAST_Node *DoExpr1(tParser *Parser)
{
	#define _next	DoExpr2
	tAST_Node *ret = _next(Parser);
	if( LookAhead(Parser) == TOK_QMARK )
	{
		TODO("Ternary operator");
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 2 - Boolean AND/OR
// --------------------
tAST_Node *DoExpr2(tParser *Parser)
{
	#define _next	DoExpr3
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_LOGICOR:
			ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, _next(Parser));
			break;
		case TOK_LOGICAND:
			ret = AST_NewBinOp(NODETYPE_BOOLAND, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 3 - Bitwise Operators
// --------------------
tAST_Node *DoExpr3(tParser *Parser)
{
	#define _next	DoExpr4
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_OR:
			ret = AST_NewBinOp(NODETYPE_BWOR, ret, _next(Parser));
			break;
		case TOK_AMP:
			ret = AST_NewBinOp(NODETYPE_BWAND, ret, _next(Parser));
			break;
		case TOK_XOR:
			ret = AST_NewBinOp(NODETYPE_BWXOR, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 4 - Comparison Operators
// --------------------
tAST_Node *DoExpr4(tParser *Parser)
{
	#define _next	DoExpr5
	tAST_Node	*ret = _next(Parser);

	// Check token
	bool cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_CMPEQU:
			ret = AST_NewBinOp(NODETYPE_EQUALS, ret, _next(Parser));
			break;
		case TOK_LT:
			ret = AST_NewBinOp(NODETYPE_LESSTHAN, ret, _next(Parser));
			break;
		case TOK_GT:
			ret = AST_NewBinOp(NODETYPE_GREATERTHAN, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 5 - Shifts
// --------------------
tAST_Node *DoExpr5(tParser *Parser)
{
	#define _next	DoExpr6
	tAST_Node *ret = _next(Parser);
	bool	cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_SHL:
			ret = AST_NewBinOp(NODETYPE_BITSHIFTLEFT, ret, _next(Parser));
			break;
		case TOK_SHR:
			ret = AST_NewBinOp(NODETYPE_BITSHIFTRIGHT, ret, _next(Parser));
			break;
		default:
			cont = false;
			PutBack(Parser);
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 6 - Arithmatic
// --------------------
tAST_Node *DoExpr6(tParser *Parser)
{
	#define _next	DoExpr7
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;

	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_PLUS:
			ret = AST_NewBinOp(NODETYPE_ADD, ret, _next(Parser));
			break;
		case TOK_MINUS:
			ret = AST_NewBinOp(NODETYPE_SUBTRACT, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 7 - Multiplcation / Division
// --------------------
tAST_Node *DoExpr7(tParser *Parser)
{
	#define _next	DoExpr8
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_ASTERISK:
			DEBUG("Multiply");
			ret = AST_NewBinOp(NODETYPE_MULTIPLY, ret, _next(Parser));
			break;
		case TOK_DIVIDE:
			DEBUG("Divide");
			ret = AST_NewBinOp(NODETYPE_DIVIDE, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 8 - Unaries Right
// --------------------
tAST_Node *DoExpr8(tParser *Parser)
{
	tAST_Node	*ret = DoExpr9(Parser);

	switch(GetToken(Parser))
	{
	case TOK_INC:
		ret = AST_NewUniOp(NODETYPE_POSTINC, ret);
		break;
	case TOK_DEC:
		ret = AST_NewUniOp(NODETYPE_POSTDEC, ret);
		break;
	default:
		PutBack(Parser);
		break;
	}

	return ret;
}

// --------------------
// Expression 9 - Unaries Left
// --------------------
tAST_Node *DoExpr9(tParser *Parser)
{
	#define _next	DoMember
	tAST_Node	*ret;
	switch(GetToken(Parser))
	{
	case TOK_INC:
		ret = AST_NewUniOp(NODETYPE_PREINC, _next(Parser));
		break;
	case TOK_DEC:
		ret = AST_NewUniOp(NODETYPE_PREDEC, _next(Parser));
		break;
	case TOK_MINUS:
		ret = AST_NewUniOp(NODETYPE_NEGATE, _next(Parser));
		break;
	case TOK_NOT:
		ret = AST_NewUniOp(NODETYPE_BWNOT, _next(Parser));
		break;
	case TOK_AMP:
		ret = AST_NewUniOp(NODETYPE_ADDROF, _next(Parser));
		break;
	case TOK_ASTERISK:
		DEBUG("Deref");
		ret = AST_NewUniOp(NODETYPE_DEREF, _next(Parser));
		break;

	default:
		PutBack(Parser);
		ret = _next(Parser);
		break;
	}
	return ret;
	#undef _next
}

// ---
// Member dereferences (. and ->)
// TODO: Fix
// ---
tAST_Node *DoMember(tParser *Parser)
{
	tAST_Node	*ret = DoParen(Parser);
	switch(GetToken(Parser))
	{
	case TOK_DOT:	// .
		ret = AST_NewBinOp(NODETYPE_MEMBER, ret, DoMember(Parser));
		break;
	case TOK_MEMBER:	// ->
		ret = AST_NewBinOp(NODETYPE_MEMBER, AST_NewUniOp(NODETYPE_DEREF, ret), DoMember(Parser));
		break;
	case TOK_PAREN_OPEN:
		ret = AST_NewFunctionCall(ret);
		if( GetToken(Parser) != TOK_PAREN_CLOSE )
		{
			PutBack(Parser);
			do {
				tAST_Node *arg = DoExpr0(Parser);
				if(!arg) {
					AST_FreeNode(ret);
					return NULL;
				}
				AST_AppendNode(ret, arg);
			} while( GetToken(Parser) == TOK_COMMA );
		}
		if(SyntaxAssert(Parser, Parser->Cur.Token, TOK_PAREN_CLOSE)) {
			AST_FreeNode(ret);
			return NULL;
		}
		break;
	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

// --------------------
// 2nd Last Expression - Parens
// --------------------
tAST_Node *DoParen(tParser *Parser)
{
	if(LookAhead(Parser) != TOK_PAREN_OPEN)
		return DoValue(Parser);
	GetToken(Parser);

	tAST_Node	*ret;
	
	switch(LookAhead(Parser))
	{
	case TOK_RWORD_SIZEOF:
	case TOK_CONST_NUM:
		break;
	case TOK_IDENT:
		// if it's not a type, break out and do expression
		if( !Types_GetTypeFromName(Parser->Cur.TokenStart, Parser->Cur.TokenLen) )
			break;
	default: {
		// assume cast
		const tType *type = Parse_GetType(Parser, NULL, NULL, NULL);
		if(SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE))
			return NULL;
		ret = AST_NewCast(type, DoParen(Parser));
		return ret; }
	}
	
	ret = DoExpr0(Parser);
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE) ) {
		AST_FreeNode(ret);
		return NULL;
	}
	return ret;
}

// --------------------
// Last Expression - Value
// --------------------
tAST_Node *DoValue(tParser *Parser)
{
	switch(LookAhead(Parser))
	{
	case TOK_STR:	return GetString(Parser);
	case TOK_CHAR:	return GetCharConst(Parser);
	case TOK_CONST_NUM:	return GetNumeric(Parser);
	case TOK_IDENT:	return GetIdent(Parser);

	case TOK_RWORD_SIZEOF:
		return GetSizeof(Parser);
	
	default:
		SyntaxAssert(Parser, GetToken(Parser), TOK_T_VALUE);
		return NULL;
	}
}

/**
 * \fn tAST_Node *GetSimpleString()
 * \brief Parses a simple string
 */
tAST_Node *GetString(tParser *Parser)
{
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_STR) )
		return NULL;

	char *data = malloc(Parser->Cur.TokenLen+1);
	memcpy(data, Parser->Cur.TokenStart, Parser->Cur.TokenLen);
	data[Parser->Cur.TokenLen] = 0;
	return AST_NewString( data, Parser->Cur.TokenLen );
}

/**
 * \fn tAST_Node *GetCharConst()
 * \brief Parses a character constant
 */
tAST_Node *GetCharConst(tParser *Parser)
{
	uint64_t	val;

	if( SyntaxAssert(Parser, GetToken(Parser), TOK_STR) )
		return NULL;

//	if( giTokenLength > 1 )
//		SyntaxWarning("Multi-byte character constants are not recommended");

	// Parse constant (local machine endian?)
	if( Parser->Cur.TokenLen > 8 ) {
		SyntaxError(Parser, "Character constant is over 8-bytes");
		return NULL;
	}
	
	memcpy(&val, Parser->Cur.TokenStart, Parser->Cur.TokenLen);

	return AST_NewInteger(val);
}


tAST_Node *GetIdent(tParser *Parser)
{
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT) )
		return NULL;

	GETTOKSTR(name);
	
	return AST_NewSymbol(name);
}

/**
 * \fn tAST_Node *GetNumeric()
 * \brief Reads a numeric value
 */
tAST_Node *GetNumeric(tParser *Parser)
{
	// Check token
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_CONST_NUM) )
		return NULL;

	return AST_NewInteger( Parser->Cur.Integer );
}

tAST_Node *GetSizeof(tParser *Parser)
{
	if( SyntaxAssert(Parser, GetToken(Parser), TOK_RWORD_SIZEOF) )
		return NULL;

	bool brackets = false;
	if( LookAhead(Parser) == TOK_PAREN_OPEN ) {
		brackets = true;
		GetToken(Parser);
	}
	
	const tType *type = Parse_GetType(Parser, NULL, NULL, NULL);
	if(!type)	return NULL;
	
	if( brackets && SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE) )
		return NULL;
	
	return AST_NewInteger( Types_GetSizeOf(type) );
}

