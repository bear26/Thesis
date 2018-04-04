#include <vector>
#include <iostream>
#include <string>

#include "ASTWalker.h"
#include "DefectStorage.h"


using namespace std;

int main(int argc, char **argv)
{
	ASTWalker walker;
	if (argc > 1)
	{
		if (argc > 2)
		{
			DefectStorage::Instance().OutputPath(string(argv[2]));
		}
		walker.WalkAST(argv[1]);
	}
	else
	{
		walker.WalkAST();
	}

	DefectStorage::Instance().SaveDefects();

	return 0;
}