/*
PHPOS Bytecode Compiler
Definitions File for Parser
PARSER.H
*/
#ifndef _PARSER_H
#define _PARSER_H

#include <global.h>

typedef struct sParser	tParser;

extern void	Parse_CodeRoot(tParser *Parser);

#include "lex.h"

struct sParser
{
	FILE	*FP;
	
	struct sParser_State {
		const char *Filename;
		 int	Line;
		enum eTokens	Token;
		
		long long int	Integer;
		
		const char	*TokenStart;
		size_t	TokenLen;
		char	LocalBuffer[64];	// Functionally the max identifier length
	} Cur, Prev, Next;
};

extern void	SyntaxError_T(tParser *Parser, enum eTokens Tok, const char *reason, ...);
extern void	SyntaxError(tParser *Parser, const char *reason, ...);
extern int	SyntaxAssert(tParser *Parser, enum eTokens tok, enum eTokens expected);
extern void	LexerError(tParser *Parser, const char *reason, ...);

#endif
