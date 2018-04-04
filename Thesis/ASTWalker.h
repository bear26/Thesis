#ifndef ASTWalker_h
#define ASTWalker_h

#include <string>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/AST/Stmt.h>

namespace clang
{
	class ASTContext;
}
class ESCAASTConsumer;


class ASTWalker
{
public:
	ASTWalker();
	~ASTWalker();
	inline void WalkAST() { WalkAST("D:\\Programming\\testcode\\test1_1_2.cpp"); }
	void WalkAST(const std::string& path);

public:
	void DumpStmt(clang::Stmt* s);

protected:
	virtual void SetIncludeDirectories();

protected:
	llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> headerSearchOptions;
	clang::ASTContext* astContext;
	ESCAASTConsumer* astConsumer;
};

#endif
