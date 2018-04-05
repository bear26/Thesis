#include <vector>
#include <iostream>
#include <string>

#include "ASTWalker.h"
#include "DefectStorage.h"

#include "file.h"

#include <llvm/Support/raw_ostream.h>

using namespace std;

extern Target::Context ctx;
extern std::map<std::string, Target::Function*> allFunctions;
extern std::set<std::string> allocatedFunctions;

std::set<std::string> processedFunctions;

int main(int argc, char **argv)
{
	ASTWalker walker;
    walker.WalkAST("C:\\Users\\Yuriy\\Desktop\\Diplom 2018\\tests\\leak2\\leak2\\1.cpp");
    walker.WalkAST("C:\\Users\\Yuriy\\Desktop\\Diplom 2018\\tests\\leak2\\leak2\\2.cpp");

    llvm::errs() << "---------------------------------------";

    allocatedFunctions.clear();

    for (auto p : allFunctions) {
        p.second->process();
    }

    //allFunctions["foo"]->process();
    //allFunctions["main"]->process();

    //Target::Module module = ctx.module;
	//if (argc > 1)
	//{
	//	if (argc > 2)
	//	{
	//		DefectStorage::Instance().OutputPath(string(argv[2]));
	//	}
	//	//walker.WalkAST(argv[1]);

	//}
	//else
	//{
	//	walker.WalkAST();
	//}

	DefectStorage::Instance().SaveDefects();

	return 0;
}