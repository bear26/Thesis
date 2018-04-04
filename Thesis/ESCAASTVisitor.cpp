#include <string>
#include <memory>
#include <map>

#include <llvm/Support/raw_ostream.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>

#include "ESCAASTVisitor.h"
#include "ASTWalker.h"

#include "Variable.h"

#include "FormulaSMT.h"
#include "BinarySMT.h"
#include "VariableSMT.h"
#include "PathStorage.h"

using namespace clang;
using namespace std;

// foo name -> 
std::map<std::string, int> m;


ESCAASTVisitor::ESCAASTVisitor() : walker(0), path(new PathStorage)
{
}


bool ESCAASTVisitor::VisitFunctionDecl(FunctionDecl* f) 
{
	return ProcessFunction(f);
}

bool ESCAASTVisitor::ProcessFunction(clang::FunctionDecl* f)
{
	string funName = f->getNameInfo().getName().getAsString();
	fsm.FunctionName(funName);
	
	llvm::errs() << "Visiting function " << funName << " id: " << f->getBuiltinID() << "\n";
	auto loc = f->getLocation();
	llvm::errs() << "Location:\n";
	llvm::errs() << "\tloc.getRawEncoding(): " << loc.getRawEncoding() << " loc.isFileID(): " << loc.isFileID() << "\n";
	SourceManager &sm = f->getASTContext().getSourceManager();
	loc.dump(sm);
	auto lstr = loc.printToString(sm);
	llvm::errs() << "\ngetFromPtrEncoding dump:\n";
	loc.getFromPtrEncoding(loc.getPtrEncoding()).dump(sm);
	llvm::errs() << "\nlocation string: " << lstr << "\n";

	PathStorage ps(lstr.substr(0, lstr.substr(4).find_first_of(":") + 4));
	if (ps.Folder() != path->Folder())
	{
		return true;
	}

	f->dump();

	auto body = f->getBody();
	if (body != nullptr)
	{
		ProcessStmt(body);
		ProcessReturnNone();
		Reset();
	}

	f->dump();

    return true;
}

void ESCAASTVisitor::Reset()
{
	fsm.Reset();
	variables.clear();
}

bool ESCAASTVisitor::ProcessStmt(clang::Stmt* stmt)
{
	if (!stmt)
	{
		return true;
	}

	llvm::errs() << "~~Processing statement~~\n";
	stmt->dump();

	if (isa<CompoundStmt>(stmt))
	{
		ProcessCompound(cast<CompoundStmt>(stmt));
	}
	if (isa<DeclStmt>(stmt))
	{
		llvm::errs() << "DeclStmt!\n";
		//(stmt)->dump();
		DeclStmt* d = cast<DeclStmt>(stmt);
		if (d)
		{
			d->dump();
			auto sd = d->getSingleDecl();
			if (sd)
			{
				if (isa<VarDecl>(sd))
				{
					llvm::errs() << "VarDecl!\n";
					ProcessDeclaration(cast<VarDecl>(sd));
				}
			}
			DeclGroupRef group = d->getDeclGroup();
			if (!group.isNull())
			{
				llvm::errs() << "DeclGroup\n";
				//TODO: do something!
			}
		}

	}
	if (isa<BinaryOperator>(stmt))
	{
		BinaryOperator* bo = cast<BinaryOperator>(stmt);
		if (bo->isAssignmentOp() == true) 
		{
			ProcessAssignment(bo);
		}

	}
	if (isa<CXXDeleteExpr>(stmt))
	{
		llvm::errs() << "delete!\n";
		ProcessDelete(cast<CXXDeleteExpr>(stmt));
	}
	if (isa<ReturnStmt>(stmt))
	{
		ProcessReturn(cast<ReturnStmt>(stmt));
	}
	if (isa<IfStmt>(stmt))
	{
		ProcessIf(cast<IfStmt>(stmt));
	}
	return true;
}


bool ESCAASTVisitor::ProcessCompound(clang::CompoundStmt* body)
{
	auto iter = body->body_begin();
	for (; iter != body->body_end(); ++iter)
	{
		ProcessStmt(*iter);
	}
	return true;
}

bool ESCAASTVisitor::ProcessAssignment(clang::BinaryOperator* binop)
{
	// blablabla
	llvm::errs() << "assignment \n";

	Stmt* lhs = binop->getLHS();
	if (isa<DeclRefExpr>(lhs))
	{
		llvm::errs() << "\tto reference ";
		DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
		DeclarationNameInfo nameInfo = ref->getNameInfo();
		DeclarationName name = nameInfo.getName();
		//name.dump();
		string sname = name.getAsString();
		llvm::errs() << sname;
					
		/*
		if (variables.find(sname) == variables.end())
		{
			VersionedVariable vv(sname, "", VAR_POINTER, 0);
			std::vector<VersionedVariable> vvvector;
			vvvector.push_back(vv);
			variables[sname] = vvvector;
			//variables.insert(
		}
		*/
		//int lhsVer = ++variables[sname];
		PtrCounter &lhsCnt = variables[sname];
		int lhsVer = ++(lhsCnt.count);

		//std::vector<VersionedVariable> &vvvector = variables[sname];

		Stmt *rhs = binop->getRHS();

        rhs->dump();

		if (isa<CXXNewExpr>(rhs))
		{
			CXXNewExpr *newOp = cast<CXXNewExpr>(rhs);
			StateFSM state;
			std::string type = "";
			VersionedVariable vv(type, sname, VAR_POINTER, lhsCnt.count);
			if (newOp->isArray())
			{
				vv.MetaType(VAR_ARRAY_POINTER);
				state.allocArrays.push_back(vv);
				lhsCnt.meta = VAR_ARRAY_POINTER;
			}
			else
			{
				state.allocPointers.push_back(vv);
				lhsCnt.meta = VAR_POINTER;
			}
			//Отметить new.
			allocated.push_back(vv);
            
            shared_ptr<VariableSMT> vvForm(new VariableSMT());
            vvForm->Var(vv);
            state.formulae.push_back(vvForm);

			fsm.AddStateToLeaves(state, fairPred);
		}
		if (isa<ImplicitCastExpr>(rhs))
		{
			ImplicitCastExpr *ice = cast<ImplicitCastExpr>(rhs);
			Stmt *subexpr = ice->getSubExpr();
			if (isa<DeclRefExpr>(subexpr)) //pointer
			{
				auto rhsRefExpr = cast<DeclRefExpr>(subexpr);
				string rhsPointer = rhsRefExpr->getNameInfo().getName().getAsString();
				StateFSM state;
				VersionedVariable lhsVar("", sname, VAR_POINTER, lhsVer);
				//VariableSMT *lhsForm = new VariableSMT();
				shared_ptr<VariableSMT> lhsForm(new VariableSMT());
				lhsForm->Var(lhsVar);
				state.formulae.push_back(lhsForm);

				PtrCounter &rhsCnt = variables[rhsPointer];
				int rhsVer = rhsCnt.count;
				VersionedVariable rhsVar("", rhsPointer, VAR_POINTER, rhsVer);
				//VariableSMT *rhsForm = new VariableSMT();				
				shared_ptr<VariableSMT> rhsForm(new VariableSMT());
				rhsForm->Var(rhsVar);
				state.formulae.push_back(rhsForm);
				lhsCnt.meta = rhsCnt.meta;

				//shared_ptr<VariableSMT> leftForm(new VariableSMT);
				//leftForm->Var(lhsVar);
				//state.formulae.push_back(leftForm);
				shared_ptr<FormulaSMT> bs(new BinarySMT(lhsVar, rhsVar, EqualSMT, false));
				state.formulae.push_back(bs);

				fsm.AddStateToLeaves(state, fairPred);
				//AddToSolver(sname, variables[sname], ...);
            }
		}
        if (isa<CallExpr>(rhs)) {
            auto rhsRefExpr = cast<CallExpr>(rhs);
            auto callee = rhsRefExpr->getCallee();
            callee->dump();
            if (auto subExpr = dyn_cast<ImplicitCastExpr>(callee)) {
                if (auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr())) {
                    std::string fooName = foo->getNameInfo().getName().getAsString();

                    if (allocatedFunctions.find(fooName) != allocatedFunctions.end()) {
                        Cout << "BBBBBBBBBBB" << fooName << "\n";

                        // TODO: поддержать как царь разные варианты для массива и просто указателся, показать
                        // насколько же алгоритм пиздат, что ищет  даже такие рассхождения =)
                        StateFSM state;
                        std::string type = "";
                        VersionedVariable vv(type, sname, VAR_POINTER, lhsCnt.count);
                        {
                            state.allocPointers.push_back(vv);
                            lhsCnt.meta = VAR_POINTER;
                        }
                        //Отметить new.
                        allocated.push_back(vv);
                        fsm.AddStateToLeaves(state, fairPred);
                    }
                }
            }
        }
		//ref->get
	}
	return true;
}

bool ESCAASTVisitor::ProcessDeclaration(clang::VarDecl* vd)
{
	llvm::errs() << "\n\n";
	vd->dump();
	llvm::errs() << "isBlockPointerType: " 
	<< vd->getTypeSourceInfo()->getType().getTypePtr()->isBlockPointerType() 
	<< "\n";

	llvm::errs() << "isAggregateType: " 
		<< vd->getTypeSourceInfo()->getType().getTypePtr()->isAggregateType() 
	<< "\n";

	llvm::errs() << "isAnyComplexType: " 
		<< vd->getTypeSourceInfo()->getType().getTypePtr()->isAnyComplexType() 
	<< "\n";

	llvm::errs() << "isArrayType: " 
		<< vd->getTypeSourceInfo()->getType().getTypePtr()->isArrayType() 
	<< "\n";

	llvm::errs() << "isDependentSizedArrayType: " 
		<< vd->getTypeSourceInfo()->getType().getTypePtr()->isDependentSizedArrayType() 
	<< "\n";

	llvm::errs() << "isIntegerType: " 
		<< vd->getTypeSourceInfo()->getType().getTypePtr()->isIntegerType() 
	<< "\n";

	llvm::errs() <<"type: " << vd->getTypeSourceInfo()->getType().getAsString();

	llvm::errs() << "\n\n";

	if (vd->getTypeSourceInfo()->getType().getTypePtr()->isAnyPointerType())
	{
		auto name = vd->getNameAsString();
		auto iter = variables.find(name);
		if (iter != variables.end())
		{
			//Error: this variable is already declared.
			llvm::errs() << "Variable with name " << name << " declared twice\n";
			return true;
		}
		PtrCounter ptrCnt;
		ptrCnt.count = 0;
		ptrCnt.meta = VAR_POINTER;
		//variables[name] = ptrCnt;
		auto cntIter = variables.insert(pair<string, PtrCounter>(name, ptrCnt));

		auto init = vd->getAnyInitializer();
		if (init == nullptr)
		{
			return true;
		}
		if (!isa<CXXNewExpr>(init))
		{
            if (auto rhsRefExpr = dyn_cast<CallExpr>(init)) {
                auto callee = rhsRefExpr->getCallee();
                callee->dump();
                if (auto subExpr = dyn_cast<ImplicitCastExpr>(callee)) {
                    if (auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr())) {
                        std::string fooName = foo->getNameInfo().getName().getAsString();

                        if (allocatedFunctions.find(fooName) != allocatedFunctions.end()) {
                            Cout << "BBBBBBBBBBB" << fooName << "\n";

                            ++cntIter.first->second.count;

                            // TODO: поддержать как царь разные варианты для массива и просто указателся, показать
                            // насколько же алгоритм пиздат, что ищет  даже такие рассхождения =)
                            StateFSM state;
                            std::string type = "";
                            VersionedVariable vv(type, name, VAR_POINTER, 1);
                            {
                                state.allocPointers.push_back(vv);
                            }
                            //Отметить функцию как new.
                            shared_ptr<VariableSMT> vvForm(new VariableSMT());
                            vvForm->Var(vv);
                            state.formulae.push_back(vvForm);

                            fsm.AddStateToLeaves(state, fairPred);
                        }
                    }
                }
            }
			return 0;
		}

		++cntIter.first->second.count;

		auto newExpr = cast<CXXNewExpr>(init);
		
		StateFSM state;
		std::string type = "";
		VersionedVariable vv(type, name, VAR_POINTER, 1);

		if(newExpr->isArray()) //Declaration of array
		{
			vv.MetaType(VAR_ARRAY_POINTER);
			cntIter.first->second.meta = VAR_ARRAY_POINTER;
			state.allocArrays.push_back(vv);
		}
		else
		{
			state.allocPointers.push_back(vv);
		}
		allocated.push_back(vv);

		shared_ptr<VariableSMT> vvForm(new VariableSMT());
		vvForm->Var(vv);
		state.formulae.push_back(vvForm);

		fsm.AddStateToLeaves(state, fairPred);
	}
	return true;
}

bool ESCAASTVisitor::ProcessDelete(clang::CXXDeleteExpr* del)
{
	auto argDel = del->getArgument();
	//TODO: change this if by function that gives a full name of pointer or null when there is no pointer.
	if (isa<ImplicitCastExpr>(argDel))
	{
		auto delCast = cast<ImplicitCastExpr>(argDel);
		auto dexpr = delCast->getSubExpr();
		if (isa<DeclRefExpr>(dexpr))
		{
			auto dptr = cast<DeclRefExpr>(dexpr);
			string name = dptr->getNameInfo().getAsString();
			//StateFSM state;
			std::string type = "";
			auto cntIter = variables.find(name);
			if (cntIter == variables.end())
			{
				llvm::errs() << "We delete undeclated variable!\n";
				return false;
			}
			VersionedVariable vv(type, name, cntIter->second.meta, cntIter->second.count);
			llvm::errs() << "isArrayForm: " << del->isArrayForm() << " isArrayFormAsWritten: " 
				<< del->isArrayFormAsWritten() << "\n";
			fsm.AddDeleteState(vv, del->isArrayForm());
		}
	}
	return true;
}

bool ESCAASTVisitor::ProcessReturn(clang::ReturnStmt* ret)
{
	clang::Expr* retVal = ret->getRetValue();

    returnExpr = retVal;

    //retVal->dump();

    if (isa<ImplicitCastExpr>(retVal))
    {
        ImplicitCastExpr *ice = cast<ImplicitCastExpr>(retVal);
        Stmt *subexpr = ice->getSubExpr();
        if (isa<DeclRefExpr>(subexpr)) //pointer
        {
            DeclRefExpr *ref = cast<DeclRefExpr>(subexpr);
            DeclarationNameInfo nameInfo = ref->getNameInfo();
            DeclarationName name = nameInfo.getName();
            //name.dump();
            string sname = name.getAsString();

            fsm.SetReturnVarName(sname);
            //returnVarName = sname;
            //Cout << "BINGO " << sname << "\n";
        }
    }

	//TODO: check return value.
    bool res = ProcessReturnNone();

    bool isAllocFoo = fsm.IsAllocReturns();
    if (isAllocFoo) {
        allocatedFunctions.insert(fsm.FunctionName());
    }

    fsm.ClearReturnVarName();
    return res;
}

bool ESCAASTVisitor::ProcessReturnNone()
{
	fsm.ProcessReturnNone();
	return true;
}

bool ESCAASTVisitor::ProcessReturnPtr(clang::ReturnStmt* ret)
{
	auto retVal = ret->getRetValue();

	return true;
}

bool ESCAASTVisitor::ProcessIf(clang::IfStmt* ifstmt)
{
	llvm::errs() << "If statement!\n";

	auto cond = ifstmt->getCond();
	std::string typeS;
	llvm::raw_string_ostream lso(typeS);
	clang::LangOptions langOpts;
	langOpts.CPlusPlus11 = true;
	PrintingPolicy pol(langOpts);
	cond->printPretty(lso, 0, pol);
	string condStr = "~if - " + lso.str();
	llvm::errs() << "\tCondition: " << condStr << "\n";
	//TODO: Create the state branching.
	fsm.PushCondition(condStr);
	StateFSM s;
	fsm.AddStateToLeaves(s, fairPred, condStr, false);
	ProcessStmt(ifstmt->getThen());
	fsm.PopCondition();
	
	string elseStr = "~else - " + condStr;
	fsm.PushCondition(elseStr);
	fsm.AddStateToLeaves(s, branchPred, elseStr, true);
	ProcessStmt(ifstmt->getElse());
	fsm.PopCondition();
	return true;
}
