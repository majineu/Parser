#include <locale.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

void Training(const char * pszCfg);
bool Parsing(const char *, const char *, const char *);
bool ParsingHis(const char *, const char *, const char *);

int usage(const char *pName)
{
	fprintf(stderr, "usage: %s	training  configFile\n", pName);
	fprintf(stderr, "usage: %s	parsing   modelPath  testFile  resFile\n", pName);
	fprintf(stderr, "usage: %s	parsingHis   modelPath  testFile  resFile\n", pName);
//	fprintf(stderr, "usage: %s	parsingHis   modelPath  testFile > hisFile\n", pName);
	return 1;
}

int Run(int argc, char ** argv)
{
	int ret = 0;
	if (argc < 2)
		return usage(*argv);

	const char *pszAct = argv[1];
	if (strcmp(pszAct, "training") == 0)
	{
		if (argc != 3)
			return usage(*argv);
		else
			Training(argv[2]);
	} 
	else if (strcmp(pszAct, "parsing") == 0)
	{
		return Parsing(argv[2], argv[3], argv[4]);//, nBS, bEng) == true;
	}
	else if (strcmp(pszAct, "parsingHis") == 0)
	{
		return ParsingHis(argv[2], argv[3], argv[4]);//, nBS, bEng) == true;
	}
	else
	{
		fprintf(stderr, "unknown command %s\n", argv[1]);
		usage(*argv);
	}

	return ret;
}

int main(int argc, char ** argv)
{
	return Run(argc, argv);
}
