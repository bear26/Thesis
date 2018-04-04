#ifndef ESCAASTVisitor_H
#define ESCAASTVisitor_H

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <set>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>

#include "VersionedVariable.h"
#include "FSM.h"
#include "PtrCounter.h"
#include "LeafPredicate.h"
#include "PathStorage.h"


class ASTWalker;

class ESCAASTVisitor : public clang::RecursiveASTVisitor<ESCAASTVisitor> 
{
public:
	ESCAASTVisitor();
public:
	bool VisitFunctionDecl(clang::FunctionDecl* f);

public:
	inline void SetWalker(ASTWalker* ast_walker) { walker = ast_walker; }

	inline void SetPath(const std::string& _path) { path->Path(_path); }

private:
	void Reset();
	bool ProcessFunction(clang::FunctionDecl* f);
	bool ProcessStmt(clang::Stmt* stmt);
	bool ProcessCompound(clang::CompoundStmt* body);
	bool ProcessAssignment(clang::BinaryOperator* binop);
	bool ProcessDeclaration(clang::VarDecl* vd);
	bool ProcessDelete(clang::CXXDeleteExpr* del);
	bool ProcessReturn(clang::ReturnStmt* ret);
	bool ProcessIf(clang::IfStmt* ifstmt);
	bool ProcessReturnNone(); //Pointers are not returned.
	bool ProcessReturnPtr(clang::ReturnStmt* ret); //Pointers are returned.
private:
	ASTWalker* walker;
	std::shared_ptr<PathStorage> path;

	//std::map<std::string, std::vector<VersionedVariable> > variables;
	//std::map<std::string, int> variables;
	std::map<std::string, PtrCounter> variables;

	std::vector<VersionedVariable> allocated;
	FSM fsm;

	FairLeafPredicate fairPred;
	BranchLeafPredicate branchPred;

    std::set<std::string> allocatedFunctions;

    clang::Expr* returnExpr = nullptr;
    std::string returnVarName;
};
#endif
