/*
 * PHPOS Bytecode Compiler
 * Bytecode Compiler
 * MAIN.C
 */
#include <global.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <parser.h>

// == Imported Functions ===
extern void	DoStatement(void);
extern void	GetDefinition(void);
extern void	InitialiseAST(void);
extern void	InitialiseData(void);
extern void	Symbol_DumpTree(void);
extern void	Optimiser_ProcessTree(void);
extern int	SetOutputArch(const char *Name);
extern void	GenerateOutput(const char *File);

// Parser Variables
const char	*gsInputFile = NULL;
const char	*gsOutputFile = "out.asm";
const char	*gsOutputArch;

int ParseCommandLine(int argc, char *argv[]);
void PrintUsage(const char *exename);

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	if( ParseCommandLine(argc, argv) )
		return 1;

	if( gsOutputArch )
	{
		if( SetOutputArch(gsOutputArch) < 0 )
			return -1;
	}

	InitialiseData();


	FILE *infile;
	
	if( !gsInputFile || strcmp(gsInputFile, "-") == 0 )
		infile = stdin;
	else
		infile = fopen(gsInputFile, "r");
	if(!infile)	{
		fprintf(stderr, "Unable to open file '%s'\n", gsInputFile);
		exit(1);
	}

	tParser parser = {
		.FP = infile,
		.Cur = {.Filename = gsInputFile, .Line = 1}
	};
	
	Parse_CodeRoot(&parser);

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
int ParseCommandLine(int argc, char *argv[])
{
	for( int i = 1; i < argc; i ++ )
	{
		const char *arg = argv[i];
		if(arg[0] != '-')
		{
			// Bare arguments
			if( !gsInputFile )
				gsInputFile = arg;
			else {
				fprintf(stderr, "Extra file passed ('%s')\n", arg);
				return 1;
			}
		}
		else if(arg[1] != '-')
		{
			// Single-char arguments
			switch(arg[1])
			{
			case '\0':
				gsInputFile = "-";
				break;
			case 'a':
				gsOutputArch = argv[++i];
				break;
			case 'o':
				gsOutputFile = argv[++i];
				break;
			default:
				fprintf(stderr, "Unknown command line option '-%c'\n", arg[1]);
				PrintUsage(argv[0]);
				return 1;
			case 'h':
				PrintUsage(argv[0]);
				exit(0);
				break;
			}
		}
		else {
			// Long arguments
			if( strcmp(arg, "--help") == 0 ) {
				PrintUsage(argv[0]);
				exit(0);
			}
			else
			{
				fprintf(stderr, "Unknown command line option '%s'\n", arg);
				PrintUsage(argv[0]);
				return 1;
			}
		}
	}

	if(!gsInputFile)
	{
		fprintf(stderr, "An input filename is required.\n");
		return -1;
	}
	
	return 0;
}

void PrintUsage(const char *exename)
{
	fprintf(stderr, 
		"Usage: %s [-o <output file>] <input file>\n"
		" -o <output file>\t Specify Output file\n"
		" -h\t\t Print this message\n"
		"", exename );
}
