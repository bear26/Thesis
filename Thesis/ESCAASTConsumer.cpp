#include "ESCAASTConsumer.h"

using namespace llvm;
using namespace clang;

bool ESCAASTConsumer::HandleTopLevelDecl(DeclGroupRef DR)
{
    for (auto it : DR) {
        // Traverse the declaration using our AST visitor.
        visitor.TraverseDecl(it);
    }

    return true;
}
