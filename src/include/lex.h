/*
 * lex.h
 * - Lexer/Tokeniser definitions
 */
#ifndef _LEX_H_
#define _LEX_H_

#include "eTokens.enum.h"
#include "parser.h"

extern int	is_ident(char c);
extern int	GetToken(tParser *Parser);
extern void	PutBack(tParser *Parser);
extern int	LookAhead(tParser *Parser);

extern const char	*GetTokenStr(enum eTokens Token);

#endif

