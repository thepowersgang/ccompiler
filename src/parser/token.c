/*
 * Acess C Compiler
 * - 
 * 
 * token.c
 * - Lexer
 */
#define _TOKEN_C	1
#include <global.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef DEBUG_ENABLED
#define DEBUG(str, v...)	printf("DEBUG %s:%i: %s "str"\n", __FILE__, __LINE__, __func__ ,## v );
#else
#define DEBUG(...)	do{}while(0)
#endif

// === PROTOTYPES ===
void	LexerError(tParser *Parser, const char *format, ...);

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
	{"return", TOK_RWORD_RETURN},
	{"continue", TOK_RWORD_CONTINUE},
	{"break", TOK_RWORD_BREAK},
	{NULL, 0}
};

// === CODE ===
int GetToken(tParser *Parser)
{
	// Explicit cast, because we actually munge the string
	char *pos = (char*)Parser->Cur.Pos;

	// Save state for PutBack
	Parser->Prev = Parser->Cur;
	
	// Check for saved state from PutBack
	if( Parser->Next.Token != TOK_NULL ) {
		Parser->Cur = Parser->Next;
		Parser->Next.Token = TOK_NULL;
		return Parser->Cur.Token;
	}
	

	// Elminate Whitespace (and preprocessor stuff)
	while( isblank(*pos) )
	{
		if( (*pos == '\n' && pos[-1] != '\r') || *pos == '\r' )
			Parser->Cur.Line ++;
		
		// C preprocessor line annotations
		if( *pos == '#' )
		{
			pos ++;
			if(*pos == ' ' && isdigit(pos[1]))
			{
				pos ++;
				Parser->Cur.Line = strtol(pos, &pos, 10);
				// Check that there's a string afterwards
				if(pos[0] != ' ' || pos[1] != '"')
					goto doPPComment;
				// Eat ' '  and "
				pos += 2;
				Parser->Cur.Filename = pos;
				while(*pos && *pos != '"')	pos ++;
				// HACK: Mutilate source string  (it doesn't matter because it's in a comment)
				*pos = '\0';
				pos ++;
				DEBUG("Set '%s':%i", Parser->Cur.Filename, Parser->Cur.Line);
			}
		doPPComment:
			while(*pos != '\r' && *pos != '\n')	pos++;
			pos ++;
		}
		
		// NOTE: These should be eliminated by the preprocessor, but just in case
		if( *pos == '/' && pos[1] == '/' )
		{
			pos += 2;
			// Line comment
			while( *pos && *pos != '\n' && *pos != '\r' )
				pos ++;
		}
		
		if( pos[0] == '/' && pos[1] == '*' )
		{
			pos += 2;
			// Block comment
			while( *pos && !(pos[0] == '*' && pos[1] == '/') )
			{
				// Detect newlines
				// - Ignore '\r' if it's part of '\r\n'
				// - handle '\n'
				if( (pos[0] == '\r' && pos[1] != '\n') || *pos == '\n' )
					Parser->Cur.Line ++;
				pos ++;
			}
		}
	}
	
	enum eTokens	token;
	Parser->Cur.TokenStart = pos;
	Parser->Cur.TokenLen = 1;
	switch(*pos++)
	{
	// Check for EOF
	case '\0':
		token = TOK_EOF;
		break;
	// Divide/Comments
	case '/':
		switch(*pos)
		{
		case '=':	// Divide Equal
			pos ++;
			token = TOK_DIV_EQU;
			break;
		default:
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
		// Compare Equals?
		if(*pos == '=') {
			pos ++;
			token = TOK_CMPEQU;
		}
		else {
			token = TOK_ASSIGNEQU;
		}
		break;
	// Multiply
	case '*':
		if(*pos == '=')	// Times-Equals
		{
			pos++;
			token = TOK_MULT_EQU;
		}
		else
			token = TOK_ASTERISK;
		break;
	// Plus
	case '+':
		if(*pos == '=')	// Plus-Equals
		{
			pos ++;
			token = TOK_PLUS_EQU;
		}
		else if(*pos == '+')	// Increment
		{
			pos ++;
			token = TOK_INC;
		}
		else
		{
			token = TOK_PLUS;
		}
		break;

	// Minus
	case '-':
		switch(*pos++)
		{
		case '|':	// Assignment Subtract
			token = TOK_MINUS_EQU;
			break;
		case '>':	// Pointer member
			token = TOK_MEMBER;
			break;
		case '-':	// Decrement
			token = TOK_DEC;
			break;
		default:
			pos --;
			token = TOK_MINUS;
			break;
		}
		break;

	// OR
	case '|':
		switch(*pos++)
		{
		case '|':	// Boolean OR
			token = TOK_LOGICOR;
			break;
		case '=':	// OR-Equals
			token = TOK_OR_EQU;
			break;
		default:
			pos --;
			token = TOK_OR;
			break;
		}
		break;

	// XOR
	case '^':
		if(*pos == '=')	// XOR-Equals
		{
			pos++;
			token = TOK_XOR_EQU;
		}
		else
		{
			token = TOK_XOR;
		}
		break;

	// AND/Address/Reference
	case '&':
		switch(*pos++)
		{
		case '&':	// Boolean AND
			token = TOK_LOGICAND;
			break;
		case '=':	// Bitwise Assignment AND
			token = TOK_AND_EQU;
			break;
		default:	// Bitwise AND / Address-of
			pos --;
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
		token = TOK_LOGICNOT;
		break;

	// String
	case '"':
		while( !(pos[0] == '"' && pos[-1] != '\\') && *pos != 0 )	// Read String
			pos ++;
		if(*pos == '\0')
			LexerError(Parser, "Unexpected EOF in string");
		pos ++;	// Eat last Quote
		token = TOK_STR;
		break;
	// Character
	case '\'':
		while( !(pos[0] == '\'' && pos[-1] != '\\') && *pos != 0 )	// Read String
			pos ++;
		if(*pos == '\0')
			LexerError(Parser, "Unexpected EOF in character constant");
		pos ++;	// Eat last Quote
		token = TOK_CHAR;
		break;
	// LT / Shift Left
	case '<':
		if(*pos == '<') {
			pos ++;
			token = TOK_SHL;
		}
		else {
			token = TOK_LT;
		}
		break;
	// GT / Shift Right
	case '>':
		if(*pos == '>') {
			pos ++;
			token = TOK_SHR;
		}
		else {
			token = TOK_GT;
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
		if( *pos == ':' ) {
			pos ++;
			token = TOK_SCOPE;
		}
		else {
			token = TOK_COLON;
		}
		break;
	// Direct member / ...
	case '.':
		if( *pos == '.' && pos[1] == '.' )
		{
			pos += 2;
			token =  TOK_VAARG;
		}
		else {
			token = TOK_DOT;
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
	
	// Numbers
	case '0':
		if( *pos == 'x' ) {
			// Hex
			pos ++;
			while( isxdigit(*pos) )
				pos ++;
			// TODO: Float
		}
		else {
			// Octal
			while( '0' <= *pos && *pos <= '7' )
				pos ++;
			// TODO: Float
		}
		token = TOK_CONST_NUM;
		break;
	case '1' ... '9':
		// Decimal / float
		while( isdigit(*pos) )
			pos ++;
		// TODO: Float
		token = TOK_CONST_NUM;
		break;
	
	// Generic (identifiers)
	default:
		// Identifier
		if(is_ident(pos[-1]))
		{
			while(is_ident(*pos))
				pos ++;
			Parser->Cur.TokenLen = pos - Parser->Cur.TokenStart;

			DEBUG("ident/rsvdwd = '%.*s'", Parser->Cur.TokenLen, Parser->Cur.TokenStart);
	
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
			fprintf(stderr, "Unknown symbol '%c' (0x%x)\n", pos[-1], pos[-1]);
			token = TOK_NULL;
		}
		break;
	}
	
	Parser->Cur.TokenLen = pos - Parser->Cur.TokenStart;
	printf("GetToken - %s '%.*s'\n",
		csaTOKEN_NAMES[token], (int)Parser->Cur.TokenLen, Parser->Cur.TokenStart);
	Parser->Cur.Token = token;
	Parser->Cur.Pos = pos;
	return token;
}

void PutBack(tParser *Parser)
{
	if( Parser->Prev.Token == TOK_NULL ) {
		assert( Parser->Prev.Token != TOK_NULL && "PutBack" );
	}
	Parser->Next = Parser->Cur;
	Parser->Cur = Parser->Prev;
	Parser->Prev.Token = TOK_NULL;
}

int LookAhead(tParser *Parser)
{
	if( Parser->Next.Token == TOK_NULL ) {
		GetToken(Parser);
		PutBack(Parser);
	}
	return Parser->Next.Token;
}

const char *GetTokenStr(enum eTokens ID)
{
	return (char*)csaTOKEN_NAMES[ID];
}

void LexerError(tParser *Parser, const char *format, ...)
{
	va_list	args;
	va_start(args, format);
	fprintf(stderr, "error:%s:%i: lex - ", Parser->Cur.Filename, Parser->Cur.Line);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	//longjmp(Parser->ErrorTarget);
}
