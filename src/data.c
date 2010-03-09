/*
PHPOS Bytecode Compiler
Data Manager
DATA.C
*/
#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_STEP	32

tConstantString	*gaStrings = NULL;
 int	giStringCount = 0;
 int	giStringSpace = 0;
 int	giStringBytes = 0;

// === CODE ===
/**
 * \fn void InitialiseData()
 */
void InitialiseData()
{
	giStringCount = 0;
	giStringSpace = STRING_STEP;
	gaStrings = (tConstantString*) malloc( giStringSpace * sizeof(tConstantString) );
}

/**
 * \fn int RegisterString(char *str, int length)
 */
int RegisterString(char *str, int length)
{
	 int	i;

	// Search for duplicate
	for( i = 0; i < giStringCount; i ++ )
	{
		if(gaStrings[i].Length != length)
			continue;
		if(strncmp(gaStrings[i].Data, str, length) == 0)
			return i;
	}
	
	i = giStringCount;
	giStringCount ++;

	if(giStringCount >= giStringSpace)
	{
		giStringSpace += STRING_STEP;
		gaStrings = (tConstantString*) realloc( gaStrings, giStringSpace * sizeof(tConstantString) );
	}

	gaStrings[i].Data = malloc(length);
	memcpy( gaStrings[i].Data, str, length );
	gaStrings[i].Length = length;
	gaStrings[i].Offset = giStringBytes;
	giStringBytes += length + 1;

	return i;
}
