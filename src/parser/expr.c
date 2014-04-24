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

#ifdef DEBUG_ENABLED
#define DEBUG(str, v...)	printf("DEBUG %s:%i: %s "str"\n", __FILE__, __LINE__, __func__ ,## v );
#else
#define DEBUG(...)	do{}while(0)
#endif

// === IMPORTS ===
extern tAST_Node	*Optimiser_StaticOpt(tAST_Node *Node);

// === Prototypes ===
void	Parse_CodeRoot(tParser *Parser);
tType	*Parse_GetType(tParser *Parser);
 int	Parse_DoDefinition(tParser *Parser, const tType *Type, tAST_Node *CodeNode);
 int	Parse_DoDefinition_VarActual(tParser *Parser, const tType *BaseType, tAST_Node *CodeNode);
tAST_Node	*DoCodeBlock(tParser *Parser);
tAST_Node	*DoStatement(tParser *Parser, tAST_Node *CodeNode);
tAST_Node	*DoIf(tParser *Parser);
tAST_Node	*DoWhile(tParser *Parser);
tAST_Node	*DoFor(tParser *Parser);
tAST_Node	*DoReturn(tParser *Parser);

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
 int	GetReservedWord(void);


// === CODE ===
/**
 * \brief Handle the root of the parse tree
 */
void Parse_CodeRoot(tParser *Parser)
{
	while( LookAhead(Parser) != TOK_EOF )
	{
		if( LookAhead(Parser) == TOK_RWORD_TYPEDEF ) {
			// Type definition.
			TODO("root typedef");
			GetToken(Parser);
			//tType *type = Parse_GetType(Parser);
			//Parse_DoTypedef(Parser, NULL);
		}
		else {
			// Only other option is a variable/function definition
			tType *type = Parse_GetType(Parser);
			if(!type)
				return ;
			Parse_DoDefinition(Parser, type, NULL);
		}
	}
}

tType *Parse_GetType_Base(tParser *Parser, enum eStorageClass *StorageClassOut)
{
	tType	*type = NULL;
	unsigned int	qualifiers = 0;
	enum eStorageClass	storage_class = STORAGECLASS_NORMAL;
	
	bool	is_size_set = false;
	enum eIntegerSize	int_size = INTSIZE_INT;
	bool	is_signed_set = false;
	bool	is_signed = true;
	bool	is_double = true;
	bool	is_long_double = false;
	
	static const char	*const size_names[] = {"char","short","int","long","long long"};
	#define ASSERT_NO_TYPE()	do{if(type){SyntaxError(Parser, "Multiple types in definition");return NULL;}}while(0)
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
			ASSERT_NO_TYPE();
			is_signed_set = true;
			is_signed = (Parser->Cur.Token == TOK_RWORD_SIGNED);
			break;
		
		// void - No possible modifiers
		case TOK_RWORD_VOID:
			ASSERT_NO_TYPE();
			ASSERT_NO_SIZE("void");
			ASSERT_NO_SIGN("void");
			// void - we have our base type now?
			type = Types_CreateVoid();
			break;
		
		// char - allows (un)signed
		case TOK_RWORD_CHAR:
			ASSERT_NO_TYPE();
			ASSERT_NO_SIZE("void");
			int_size = INTSIZE_CHAR;
			is_size_set = true;
			break;
		// short - allows sign and 'int'
		case TOK_RWORD_SHORT:
			ASSERT_NO_TYPE();
			ASSERT_NO_SIZE("short");
			int_size = INTSIZE_SHORT;
			is_size_set = true;
			break;
		// int - Default to INTSIZE_INT if not yet specified
		// > Special catch for attempting 'int char'
		case TOK_RWORD_INT:
			ASSERT_NO_TYPE();
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
			ASSERT_NO_TYPE();
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
			ASSERT_NO_TYPE();
			ASSERT_NO_SIZE("float");
			ASSERT_NO_SIGN("float");
			type = Types_CreateFloatType(FLOATSIZE_FLOAT);
			break;
		// Double - Handle 'long double'
		case TOK_RWORD_DOUBLE:
			ASSERT_NO_TYPE();
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
			ASSERT_NO_TYPE();
			if( is_signed_set || is_double )
				getting_base_type = false;
			else
			{
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
		case TOK_RWORD_ENUM:
		case TOK_RWORD_UNION:
		case TOK_RWORD_STRUCT:
			SyntaxError(Parser, "TODO: Compound types");
			return NULL;
			break;
		
		// Qualifiers
		case TOK_RWORD_CONST:    qualifiers |= QUALIFIER_CONST;    break;
		case TOK_RWORD_RESTRICT: qualifiers |= QUALIFIER_RESTRICT; break;
		case TOK_RWORD_VOLATILE: qualifiers |= QUALIFIER_VOLATILE; break;
		
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
			getting_base_type = false;
			break;
		}
	}
	PutBack(Parser);
	
	// Build type if not already built
	if( !type )
	{
		if( is_double ) {
			// Floating point type
			// Some asserts to make sure nothing stupid has happened
			assert( !is_signed_set );
			assert( !is_size_set || (int_size != INTSIZE_LONG) );
			type = Types_CreateFloatType( (is_long_double ? FLOATSIZE_LONGDOUBLE : FLOATSIZE_DOUBLE) );
		}
		else if( is_size_set ) {
			// Intger type
			type = Types_CreateIntegerType( is_signed, int_size );
		}
		else {
			// No size specified, error?
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

/**
 * \brief Handle storage classes, structs, unions, enums, primative types
 */
tType *Parse_GetType(tParser *Parser)
{
	enum eStorageClass	storage_class;
	tType *type = Parse_GetType_Base(Parser, &storage_class);
	if( !type )
		return NULL;
	unsigned int qualifiers = 0;
	// 2. Handle qualifiers and pointers
	while( LookAhead(Parser) != TOK_IDENT )
	{
		switch( GetToken(Parser) )
		{
		case TOK_ASTERISK:
			// Pointer
			// - Determine type
			type = Types_ApplyQualifiers(type, qualifiers);
			type = Types_CreatePointerType(type);
			// - Clear
			qualifiers = 0;
			// - Continue on
			break;
		case TOK_RWORD_CONST:    qualifiers |= QUALIFIER_CONST;    break;
		case TOK_RWORD_RESTRICT: qualifiers |= QUALIFIER_RESTRICT; break;
		case TOK_RWORD_VOLATILE: qualifiers |= QUALIFIER_VOLATILE; break;
		
		default:
			SyntaxError_T(Parser, Parser->Cur.Token, "Expected * or qualifier");
			Types_DerefType(type);
			return NULL;
		}
		GetToken(Parser);
	}
	return type;
}

/**
 * \brief Handle variable/function definition
 */
int Parse_DoDefinition(tParser *Parser, const tType *Type, tAST_Node *CodeNode)
{
	SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT);
	
	// Current token is variable/function name
	
	// - next '(' = Declare function
	if( LookAhead(Parser) == TOK_PAREN_OPEN )
	{
		// Parse function arguments
		TODO("Function definition arguments");
	}
	// - next * = Declare variable
	else
	{
		// TODO: Multiple variable definitions work off base type, not final type
		// i.e. pointer-ness binds to name, not type
		
		const tType	*basetype = Type;	// not actually base type, pointers trimmed if needed
		
		// Define variable based off this type
		Parse_DoDefinition_VarActual(Parser, Type, CodeNode);
		
		if( LookAhead(Parser) == TOK_COMMA )
		{
			// Work the type back until it's not a pointer
			while( basetype->Class == TYPECLASS_POINTER )
				basetype = basetype->Pointer;
			
			Type = basetype;
			
			// Get extra definitions
			while( GetToken(Parser) == TOK_COMMA )
			{
				// Handle pointers and attributes (const/volatile/restrict)
				// TODO:
				// Get identifier
				SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT);
				Parse_DoDefinition_VarActual(Parser, Type, CodeNode);
			}
			PutBack(Parser);
		}
	
		SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);
	}
	DEBUG("<<<");
	return 1;
}

int Parse_DoDefinition_VarActual(tParser *Parser, const tType *BaseType, tAST_Node *CodeNode)
{
	const tType	*type = BaseType;
	SyntaxAssert(Parser, Parser->Cur.Token, TOK_IDENT);
	
	char name[ Parser->Cur.TokenLen+1 ];
	strncpy(name, Parser->Cur.TokenStart, Parser->Cur.TokenLen+1);
	
	// TODO: Arrays (first level size is optional)
	#if 0
	while(GetToken(Parser) == TOK_SQUARE_OPEN)
	{
		tAST_Node *size = DoExpr0(Parser);
		SyntaxAssert(Parser, GetToken(Parser), TOK_SQUARE_CLOSE);
	}
	PutBack(Parser);
	#endif
	
	tAST_Node	*init_value = NULL;
	if( LookAhead(Parser) == TOK_ASSIGNEQU )
	{
		GetToken(Parser);
		if( LookAhead(Parser) == TOK_BRACE_OPEN ) {
			// Only valid if this type is a compound type (array or struct)
		}
		else {
			init_value = DoExpr0(Parser);
			if( !init_value )
				return 1;
		}
	}
	
	if( CodeNode )
	{
		//tAST_Node *ret = AST_NewVariableDef(type, name);
		//AST_AppendNode(CodeNode, ret);
		TODO("Local variables");
	}
	else
	{
		// TODO: Linkage
		Symbol_AddGlobalVariable(type, LINKAGE_GLOBAL, name, init_value);
	}
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
			AST_AppendNode( ret, DoStatement(Parser, ret) );
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
		ret = DoReturn(Parser);
		SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		break;
	case TOK_IDENT: {
		tType *type = Types_GetTypeFromName(Parser->Cur.TokenStart, Parser->Cur.TokenLen);
		if( type ) {
			Parse_DoDefinition(Parser, type, CodeNode);
			return ACC_ERRPTR;
		}
		else {
			ret = DoExpr0(Parser);
			SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		}
		break; }
	default:
		ret = DoExpr0(Parser);
		SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		break;
	}
	return ret;
}

/**
 * \brief Parses an if block
 */
tAST_Node *DoIf(tParser *Parser)
{
	tAST_Node	*test, *true_code;
	tAST_Node	*false_code = NULL;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);		// Eat (
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);	// Eat )

	// Contents
	true_code = DoCodeBlock(Parser);

	if(GetToken(Parser) == TOK_RWORD_ELSE)
	{
		false_code = DoCodeBlock(Parser);
	}

	return AST_NewIf(test, true_code, false_code);
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
	tAST_Node	*init, *test, *post, *code;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);	// Eat (

	// Initialiser (definition valid)
	// TODO: Support defining variables in initialiser
	init = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);	// Eat ;

	// Condition
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);	// Eat ;

	// Postscript
	post = DoExpr0(Parser);

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);	// Eat )

	// Contents
	code = DoCodeBlock(Parser);

	return AST_NewFor(init, test, post, code);
}

/**
 * \brief Handle a return statement
 */
tAST_Node *DoReturn(tParser *Parser)
{
	return AST_NewUniOp( NODETYPE_RETURN, DoExpr0(Parser) );
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
			ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, _next(Parser));
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
		switch(LookAhead(Parser))
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
			ret = AST_NewBinOp(NODETYPE_MULTIPLY, ret, _next(Parser));
			break;
		case TOK_DIVIDE:
			ret = AST_NewBinOp(NODETYPE_DIVIDE, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
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
	tAST_Node	*ret = DoMember(Parser);
	switch(GetToken(Parser))
	{
	case TOK_INC:
		ret = AST_NewUniOp(NODETYPE_PREINC, ret);
		break;
	case TOK_DEC:
		ret = AST_NewUniOp(NODETYPE_PREDEC, ret);
		break;
	case TOK_MINUS:
		ret = AST_NewUniOp(NODETYPE_NEGATE, ret);
		break;
	case TOK_NOT:
		ret = AST_NewUniOp(NODETYPE_BWNOT, ret);
		break;
	case TOK_AMP:
		ret = AST_NewUniOp(NODETYPE_ADDROF, ret);
		break;
	case TOK_ASTERISK:
		ret = AST_NewUniOp(NODETYPE_DEREF, ret);
		break;

	default:
		PutBack(Parser);
		break;
	}
	return ret;
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
	if(LookAhead(Parser) == TOK_PAREN_OPEN)
	{
		tAST_Node	*ret;
		GetToken(Parser);
		ret = DoExpr0(Parser);
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
		return ret;
	}
	else
		return DoValue(Parser);
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

	default:
		SyntaxAssert(Parser, GetToken(Parser), TOK_T_VALUE);
		return NULL;
	}
}

size_t Parse_ConvertString(tParser *Parser, char *Output, const char *Src, size_t SrcLen)
{
	size_t	ret = 0;
	for( int i = 0; i < SrcLen; i++ )
	{
		if( Src[i] == '\\' )
		{
			i ++;
			assert(i != SrcLen);	// Lexer shouldn't allow, as it would have escaped closing "
			char ch = 0;
			switch(Src[i])
			{
			case '"':	ch = '"';	break;
			case 'n':	ch = '\n';	break;
			case 'r':	ch = '\r';	break;
			case 't':	ch = '\t';	break;
			case 'b':	ch = '\b';	break;
			case '0' ... '7':
				// Octal constant
				TODO("Octal constants in strings");
				break;
			case 'x':
				// Hex constant
				TODO("Hex constants in strings");
				break;
			default:
				// oopsie
				break;
			}
			if(Output)
				Output[ret] = ch;
			ret ++;
		}
		else
		{
			if(Output)
				Output[ret] = Src[i];
			ret ++;
		}
	}
	return ret;
}

/**
 * \fn tAST_Node *GetSimpleString()
 * \brief Parses a simple string
 */
tAST_Node *GetString(tParser *Parser)
{
	SyntaxAssert(Parser, GetToken(Parser), TOK_STR);

	// Parse String
	size_t	len = Parse_ConvertString(Parser, NULL, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	char *buf = malloc( len+1 );
	assert(buf);
	Parse_ConvertString(Parser, buf, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	buf[len] = '\0';

	return AST_NewString( buf, len );
}

/**
 * \fn tAST_Node *GetCharConst()
 * \brief Parses a character constant
 */
tAST_Node *GetCharConst(tParser *Parser)
{
	uint64_t	val;

	SyntaxAssert(Parser, GetToken(Parser), TOK_STR);

//	if( giTokenLength > 1 )
//		SyntaxWarning("Multi-byte character constants are not recommended");

	// Parse constant (local machine endian?)
	size_t len = Parse_ConvertString(Parser, NULL, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	if( len > 8 )
		SyntaxError(Parser, "Character constant is over 8-bytes");
	
	Parse_ConvertString(Parser, (void*)&val, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);

	return AST_NewInteger(val);
}


tAST_Node *GetIdent(tParser *Parser)
{
	return NULL;
}

/**
 * \fn tAST_Node *GetNumeric()
 * \brief Reads a numeric value
 */
tAST_Node *GetNumeric(tParser *Parser)
{
	// Check token
	SyntaxAssert(Parser, GetToken(Parser), TOK_CONST_NUM);

	// Create Temporary String
	char	temp[Parser->Cur.TokenLen+1];
	for( int i = 0; i < Parser->Cur.TokenLen; i ++ )
		temp[ i ] = Parser->Cur.TokenStart[ i ];
	temp[ Parser->Cur.TokenLen ] = '\0';

	// Get value
	long value = strtol( temp, NULL, 0 );
	return AST_NewInteger( value );
}

