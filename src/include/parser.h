/*
PHPOS Bytecode Compiler
Definitions File for Parser
PARSER.H
*/
#ifndef _PARSER_H
#define _PARSER_H

#include <global.h>

extern int	is_whitespace(char c);
extern int	is_num(char c);
extern int	is_hex(char c);
extern int	is_ident(char c);
extern void	ParseError(char *reason);
extern void	ParseError2(int token, int expected);
extern void SyntaxError(char *reason);
extern void	SyntaxError2(int token, int expected);
extern void	SyntaxError3(int Token, ...);
extern void SyntaxAssert(int tok, int expected);
extern void	SyntaxErrorF(const char *reason, ...);
extern void	FatalError(char *reason);
extern int	GetToken();
extern void	PutBack();
extern int	LookAhead();

extern char *GetTokenStr(int ID);

#include "eTokens.enum.h"

enum eReservedWords {
	RWORD_NULL,

	RWORD_RETURN,
	RWORD_CONTINUE,
	RWORD_BREAK,

	RWORD_FOR,
	RWORD_DO,
	RWORD_WHILE,
	RWORD_IF,
	RWORD_ELSE,

	RWORD_LAST
};

#endif
