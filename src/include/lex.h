/*
 * lex.h
 * - Lexer/Tokeniser definitions
 */
#ifndef _LEX_H_
#define _LEX_H_

#include "eTokens.enum.h"
#include "parser.h"

extern enum eTokens	GetToken(tParser *Parser);
extern void	PutBack(tParser *Parser);
extern enum eTokens	LookAhead(tParser *Parser);

extern const char	*GetTokenStr(enum eTokens Token);

#endif

