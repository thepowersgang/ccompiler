/*
 * Acess C Compiler
 * - 
 * 
 * token.c
 * - Lexer
 */
#define _TOKEN_C	1
//#define DEBUG_ENABLED
#include <global.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <parser.h>

// === PROTOTYPES ===
enum eTokens	GetToken_Int(tParser *Parser);
bool	is_ident(char ch);
void	Parse_MoveState(struct sParser_State *Dst, struct sParser_State *Src);

// === GLOBALS ===
const struct sRsvdWord {
	const char	*String;
	enum eTokens	Token;
} caRESERVED_WORDS[] = {
	{"if", TOK_RWORD_IF},
	{"else", TOK_RWORD_ELSE},
	{"for", TOK_RWORD_FOR},
	{"while", TOK_RWORD_WHILE},
	{"do", TOK_RWORD_DO},
	{"switch", TOK_RWORD_SWITCH},
	{"return", TOK_RWORD_RETURN},
	{"continue", TOK_RWORD_CONTINUE},
	{"break", TOK_RWORD_BREAK},
	{"case", TOK_RWORD_CASE},
	{"default", TOK_RWORD_DEFAULT},

	// Types
	{"void", TOK_RWORD_VOID},
	{"char",  TOK_RWORD_CHAR},
	{"short", TOK_RWORD_SHORT},
	{"int",   TOK_RWORD_INT},
	{"long",  TOK_RWORD_LONG},
	{"float",  TOK_RWORD_FLOAT},
	{"double", TOK_RWORD_DOUBLE},
	{"signed",   TOK_RWORD_SIGNED},
	{"unsigned", TOK_RWORD_UNSIGNED},
	{"_Bool",    TOK_RWORD_BOOL},
	{"_Complex", TOK_RWORD_COMPLEX},
	// Linkage
	{"extern", TOK_RWORD_EXTERN},
	{"static", TOK_RWORD_STATIC},
	{"inline", TOK_RWORD_INLINE},
	{"register", TOK_RWORD_REGISTER},
	{"typedef", TOK_RWORD_TYPEDEF},
	{"auto", TOK_RWORD_AUTO},
	// Class
	{"const", TOK_RWORD_CONST},
	{"restrict", TOK_RWORD_RESTRICT},
	{"volatile", TOK_RWORD_VOLATILE},
	// Compound
	{"struct", TOK_RWORD_STRUCT},
	{"union", TOK_RWORD_UNION},
	{"enum", TOK_RWORD_ENUM},
	{"sizeof", TOK_RWORD_SIZEOF},
};

// === CODE ===
enum eTokens GetToken(tParser *Parser)
{
	bool is_first = (Parser->Cur.Token == TOK_NULL);
	// Save state for PutBack
	Parse_MoveState(&Parser->Prev, &Parser->Cur);
	
	// Check for saved state from PutBack
	if( Parser->Next.Token != TOK_NULL )
	{
		Parse_MoveState(&Parser->Cur, &Parser->Next);
		DEBUG("Fast ret %s", csaTOKEN_NAMES[Parser->Cur.Token]);
		return Parser->Cur.Token;
	}
	
	GetToken_Int(Parser);
	while( Parser->Cur.Token == TOK_NEWLINE || is_first )
	{
		enum eTokens	ret = Parser->Cur.Token;
		is_first = false;
		DEBUG("Checking for preprocessor / newlines (%s)", csaTOKEN_NAMES[ret]);
		while( ret == TOK_NEWLINE ) {
			Parser->Cur.Line ++;
			ret = GetToken_Int(Parser);
		}
		if( ret == TOK_HASH )
		{
			// Integer
			if( GetToken_Int(Parser) != TOK_CONST_NUM ) {
				// ERROR!
				while( Parser->Cur.Token != TOK_NEWLINE )
					GetToken_Int(Parser);
				continue ;
			}
			Parser->Cur.Line = Parser->Cur.Integer-1;
			DEBUG("- Line updated to %i", Parser->Cur.Line+1);
			// String
			if( GetToken_Int(Parser) != TOK_STR ) {
				// ERROR!
				while( Parser->Cur.Token != TOK_NEWLINE )
					GetToken_Int(Parser);
				continue ;
			}
			DEBUG("- Update file to '%.*s'", (int)Parser->Cur.TokenLen, Parser->Cur.TokenStart);
			Deref(Parser->Cur.Filename);
			Parser->Cur.Filename = CreateRef(Parser->Cur.TokenStart, Parser->Cur.TokenLen);
			
			// Several integers (optional)
			while( Parser->Cur.Token != TOK_NEWLINE )
				GetToken_Int(Parser);
		}
	}
	return Parser->Cur.Token;
}

enum eTokens GetToken_Int(tParser *Parser)
{
	char last_ch;
	char ch = fgetc(Parser->FP);
	// Elminate Whitespace (and preprocessor stuff)
	for( ; isspace(ch); ch = fgetc(Parser->FP) )
	{
		if( ch == '\r' ) {
			ch = fgetc(Parser->FP);
			if( ch != '\n' ) {
				DEBUG("Carriage Return");
				ungetc(ch, Parser->FP);
				return (Parser->Cur.Token = TOK_NEWLINE);
			}
		}
		if( ch == '\n' ) {
			DEBUG("Newline");
			return (Parser->Cur.Token = TOK_NEWLINE);
		}
	}
	
	enum eTokens	token;
	Parser->Cur.TokenLen = 0;
	Parser->Cur.TokenStart = NULL;
	switch( ch )
	{
	// Check for EOF
	case -1:
	case '\0':
		token = TOK_EOF;
		break;
	// Divide/Comments
	case '/':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// Divide Equal
			token = TOK_DIV_EQU;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_DIVIDE;
			break;
		}
		break;

	// Question Mark
	case '?':
		token = TOK_QMARK;
		break;

	// Equals
	case '=':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// Compare Equals?
			token = TOK_CMPEQU;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_ASSIGNEQU;
			break;
		}
		break;
	// Multiply
	case '*':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// Multiply Equals?
			token = TOK_MULT_EQU;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_ASTERISK;
		}
		break;
	// Plus
	case '+':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// Plus-Equals
			token = TOK_PLUS_EQU;
			break;
		case '+':	// Increment
			token = TOK_INC;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_PLUS;
			break;
		}
		break;

	// Minus
	case '-':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// Assignment Subtract
			token = TOK_MINUS_EQU;
			break;
		case '>':	// Pointer member
			token = TOK_MEMBER;
			break;
		case '-':	// Decrement
			token = TOK_DEC;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_MINUS;
			break;
		}
		break;

	// OR
	case '|':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '|':	// Boolean OR
			token = TOK_LOGICOR;
			break;
		case '=':	// OR-Equals
			token = TOK_OR_EQU;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_OR;
			break;
		}
		break;

	// XOR
	case '^':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':	// XOR-Equals
			token = TOK_XOR_EQU;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_XOR;
			break;
		}
		break;

	// AND/Address/Reference
	case '&':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '&':	// Boolean AND
			token = TOK_LOGICAND;
			break;
		case '=':	// Bitwise Assignment AND
			token = TOK_AND_EQU;
			break;
		default:	// Bitwise AND / Address-of
			ungetc(ch, Parser->FP);
			token = TOK_AMP;
			break;
		}
		break;

	// Bitwise NOT
	case '~':
		token = TOK_NOT;
		break;
	// Boolean NOT
	case '!':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '=':
			token = TOK_CMPNEQ;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_LOGICNOT;
			break;
		}
		break;

	// String
	case '"':
	// Character constant
	case '\'': {
		const char end_ch = ch;
		size_t	len = 0;
		char	*buf = Parser->Cur.LocalBuffer;
		bool escaping = false;
		do {
			last_ch = ch;
			ch = fgetc(Parser->FP);
			if( escaping ) {
				switch(ch)
				{
				case '"':	ch = '"';	break;
				case '\'':	ch = '\'';	break;
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
					LexerError(Parser, "Unknown escape sequence '\\%c'", ch);
					break;
				}
			}
			else if( ch == '\\' ) {
				escaping = true;
				continue ;
			}
			else {
				// fall
			}
			// Push `ch`
			if( len == sizeof(Parser->Cur.LocalBuffer) ) {
				buf = malloc( len + 1 );
				memcpy(buf, Parser->Cur.LocalBuffer, len);
			}
			else if( len > sizeof(Parser->Cur.LocalBuffer) ) {
				buf = realloc(buf, len + 1);
			}
			else {
			}
			buf[len++] = ch;
		} while( !feof(Parser->FP) && !(ch == end_ch && last_ch != '\\') );
		
		// Ignore closing quote
		len --;
		
		Parser->Cur.TokenLen = len;
		Parser->Cur.TokenStart = buf;
		
		if( end_ch == '"' ) {
			if( feof(Parser->FP) )
				LexerError(Parser, "Unexpected EOF in string");
			token = TOK_STR;
		}
		else {
			if( feof(Parser->FP) )
				LexerError(Parser, "Unexpected EOF in character constant");
			token = TOK_CHAR;
		}
		
		break; }
	// LT / Shift Left
	case '<':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '<':
			token = TOK_SHL;
			break;
		case '=':
			token = TOK_LTE;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_LT;
			break;
		}
		break;
	// GT / Shift Right
	case '>':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '>':
			token = TOK_SHR;
			break;
		case '=':
			token = TOK_GTE;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_GT;
			break;
		}
		break;
	// End of Statement (Semicolon)
	case ';':
		token = TOK_SEMICOLON;
		break;
	// Comma
	case ',':
		token = TOK_COMMA;
		break;
	// Scope (C++) / Label
	case ':':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case ':':	// C++ Scope
			token = TOK_SCOPE;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_COLON;
			break;
		}
		break;
	// Direct member / ...
	case '.':
		switch( (ch = fgetc(Parser->FP)) )
		{
		case '.':
			if( fgetc(Parser->FP) != '.' ) {
				LexerError(Parser, ".. is not a valid token");
				break;
			}
			token =  TOK_VAARG;
			break;
		default:
			ungetc(ch, Parser->FP);
			token = TOK_DOT;
			break;
		}
		break;
	// Braces
	case '{':	token = TOK_BRACE_OPEN;	break;
	case '}':	token = TOK_BRACE_CLOSE;	break;
	// Parens
	case '(':	token = TOK_PAREN_OPEN;	break;
	case ')':	token = TOK_PAREN_CLOSE;	break;
	// Brackets
	case '[':	token = TOK_SQUARE_OPEN;	break;
	case ']':	token = TOK_SQUARE_CLOSE;	break;

	case '#':	token = TOK_HASH;	break;	

	// Numbers
	case '0':
		if( (ch = fgetc(Parser->FP)) == 'x' )
		{
			// Hex
			fscanf(Parser->FP, "%llx", &Parser->Cur.Integer);
			// TODO: Float (look for a .)
		}
		else {
			ungetc(ch, Parser->FP);
			// Octal
			fscanf(Parser->FP, "%llo", &Parser->Cur.Integer);
			// TODO: Float
		}
		token = TOK_CONST_NUM;
		break;
	case '1' ... '9':
		ungetc(ch, Parser->FP);
		// Decimal / float
		fscanf(Parser->FP, "%lld", &Parser->Cur.Integer);
		// TODO: Float
		token = TOK_CONST_NUM;
		break;
	
	// Generic (identifiers)
	default:
		// Identifier
		if(is_ident(ch))
		{
			size_t	pos = 0;
			do {
				if( pos == sizeof(Parser->Cur.LocalBuffer) ) {
					LexerError(Parser, "Identifer too long (>%zi bytes)", pos);
					break;
				}
				Parser->Cur.LocalBuffer[pos++] = ch;
			} while( is_ident( (ch = fgetc(Parser->FP)) ) );
			// Restore final character
			ungetc(ch, Parser->FP);
			Parser->Cur.TokenStart = Parser->Cur.LocalBuffer;
			Parser->Cur.TokenLen = pos;

			DEBUG("ident/rsvdwd = '%.*s'", (int)Parser->Cur.TokenLen, Parser->Cur.TokenStart);
	
			// Check for reserved words	
			token = TOK_IDENT;
			for( int i = 0; i < sizeof(caRESERVED_WORDS)/sizeof(caRESERVED_WORDS[0]); i ++ ) {
				const struct sRsvdWord *rwd = &caRESERVED_WORDS[i];
				if( Parser->Cur.TokenLen != strlen(rwd->String) )
					continue ;
				if( strncmp(Parser->Cur.TokenStart, rwd->String, strlen(rwd->String)) != 0 )
					continue ;
				token = rwd->Token;
				break;
			}
		}
		else {
			fprintf(stderr, "Unknown symbol '%c' (0x%x)\n", ch, ch);
			token = TOK_NULL;
		}
		break;
	}
	
	Parser->Cur.Token = token;
	return token;
}

void PutBack(tParser *Parser)
{
	assert( Parser->Cur.Token != TOK_NULL );
	DEBUG("PutBack: %s", csaTOKEN_NAMES[Parser->Cur.Token]);
	Parse_MoveState(&Parser->Next, &Parser->Cur);
	Parse_MoveState(&Parser->Cur, &Parser->Prev);
}

enum eTokens LookAhead(tParser *Parser)
{
	if( Parser->Next.Token == TOK_NULL ) {
		DEBUG("LookAhead: --");
		GetToken(Parser);
		PutBack(Parser);
	}
	DEBUG("LookAhead: %s", csaTOKEN_NAMES[Parser->Next.Token]);
	return Parser->Next.Token;
}

const char *GetTokenStr(enum eTokens ID)
{
	return (char*)csaTOKEN_NAMES[ID];
}

bool is_ident(char c)
{
	if('0' <= c && c <= '9')
		return 1;
	if('a' <= c && c <= 'z')
		return 1;
	if('A' <= c && c <= 'Z')
		return 1;
	if(c == '_')
		return 1;
	return 0;
}

void Parse_MoveState(struct sParser_State *Dst, struct sParser_State *Src)
{
	Deref(Dst->Filename);
	Ref(Src->Filename);
	*Dst = *Src;
	Src->Token = TOK_NULL;
}
