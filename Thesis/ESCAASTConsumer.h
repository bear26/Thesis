#ifndef ESCAASTConsumer_h
#define ESCAASTConsumer_h

#include <string>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclGroup.h>

#include "ESCAASTVisitor.h"

class ASTWalker;

class ESCAASTConsumer : public clang::ASTConsumer
{
public:
    ESCAASTConsumer() = default;

	virtual bool HandleTopLevelDecl(clang::DeclGroupRef DR) override;

public:
	inline void SetWalker(ASTWalker* walker) { visitor.SetWalker(walker); }
	inline void SetPath(const std::string& path) { visitor.SetPath(path); }
private:
	ESCAASTVisitor visitor;
};

#endif
