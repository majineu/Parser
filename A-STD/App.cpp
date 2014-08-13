#include "App.h"


CApp::CApp(void)
{
	parser = NULL;
}


CApp::~CApp(void)
{
	if(parser != NULL)
		delete parser;
	parser = NULL;
}
