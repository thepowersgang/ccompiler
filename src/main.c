/*
 * PHPOS Bytecode Compiler
 * Bytecode Compiler
 * MAIN.C
 */
#include <global.h>
#include <stdlib.h>
#include <stdio.h>

// == Imported Functions ===
extern void	DoStatement(void);
extern void	GetDefinition(void);
extern void	InitialiseAST(void);
extern void	InitialiseData(void);
extern void Symbol_DumpTree(void);
extern void	Optimiser_ProcessTree(void);
extern void	GenerateOutput(char *File);

// Parser Variables
char	*gsNextChar = NULL;
 int	giToken = 0;
char	*gsTokenStart = NULL;
 int	giTokenLength = 0;
 int	giLine = 1;
 int	giStatement = 1;
 int	giInputLength = 0;
char	*gsCurFile = NULL;
char	*gsInputFile = NULL;
char	*gsOutputFile = "out.phc";
char	*gsInputData = NULL;

void ParseCommandLine(int argc, char *argv[]);
void PrintSyntax(char *exename);

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	tok;

	ParseCommandLine(argc, argv);

	InitialiseData();

	gsNextChar = gsInputData;

	while( (tok = LookAhead()) != TOK_EOF )
	{
		switch(tok)
		{
		#if 0
		// Reserved Word
		case TOK_RSVDWORD:
			GetToken();
			tok = GetReservedWord();
			switch(tok)
			{
			//case RWORD_STRUCT:
			//	break;
			default:
				SyntaxError("Unexpected reserved word");
				break;
			}
			break;
		#endif

		// Type Definition
		case TOK_IDENT:	GetDefinition();	break;

		default:
			SyntaxError2(tok, TOK_IDENT);
		}
	}

	Symbol_DumpTree();

	Optimiser_ProcessTree();
	
	Symbol_DumpTree();

	// Output
	GenerateOutput(gsOutputFile);

	return 0;
}

/**
 * \fn void ParseCommandLine(int argc, char *argv[])
 */
void ParseCommandLine(int argc, char *argv[])
{
	int i = 1, len;
	FILE	*infile = NULL;

	for( i = 1; i < argc; i ++ )
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'o':
				gsOutputFile = argv[++i];
				break;
			default:
				fprintf(stderr, "Unknow command line option '-%c'\n", argv[i][1]);
			case 'h':
				PrintSyntax(argv[0]);
				exit(0);

			}
		} else {
			gsInputFile = argv[i];
		}
	}

	if(!gsInputFile)
	{
		fprintf(stderr, "An input filename is required.\n");
		exit(-1);
	}

	infile = fopen(gsInputFile, "r");
	if(!infile)	{
		fprintf(stderr, "Unable to open file '%s'\n", gsInputFile);
		exit(-1);
	}

	fseek(infile, 0, SEEK_END);
	giInputLength = len = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	gsCurFile = gsInputFile;

	gsInputData = malloc(len+1);
	fread(gsInputData, 1, len, infile);
	gsInputData[len] = 0;

	fclose(infile);
}

void PrintSyntax(char *exename)
{
	fprintf(stderr, 
		"Usage: %s [-o <output file>] <input file>\n"
		" -o <output file>\t Specify Output file\n"
		" -h\t\t Print this message\n"
		"", exename );
}
