#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Parse/ParseAST.h>

#include <clang/AST/ASTContext.h>

#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/IR/Function.h>

#include "ASTWalker.h"
#include "ESCAASTConsumer.h"

using namespace clang;
using namespace llvm;
using namespace std;

ASTWalker::ASTWalker()
    : headerSearchOptions(new clang::HeaderSearchOptions())
    , astConsumer(new ESCAASTConsumer)
{
	astConsumer->SetWalker(this);
}

ASTWalker::~ASTWalker()
{
	delete astConsumer;
	delete astContext;
}

void ASTWalker::SetIncludeDirectories()
{
	//headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include", clang::frontend::Angled,
	//	false, false); 
	//headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include\\c++\\3.4.5" , clang::frontend::Angled,
	//	false, false); 
	//headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include\\c++\\3.4.5\\mingw32" , clang::frontend::Angled,
	//	false, false); 
	//headerSearchOptions->AddPath("D:\\Portable\\MinGW\\lib\\gcc\\mingw32\\3.4.5\\include" , clang::frontend::Angled,
	//	false, false); 
}

void ASTWalker::WalkAST(const string& path)
{
    clang::DiagnosticOptions diagnosticOptions;
    clang::TextDiagnosticPrinter *pTextDiagnosticPrinter = new clang::TextDiagnosticPrinter(
																llvm::outs(), &diagnosticOptions);
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;
    clang::DiagnosticsEngine* pDiagnosticsEngine = new clang::DiagnosticsEngine(pDiagIDs,
            &diagnosticOptions, pTextDiagnosticPrinter);

    clang::LangOptions languageOptions;
    clang::FileSystemOptions fileSystemOptions;
    clang::FileManager fileManager(fileSystemOptions);

    clang::SourceManager sourceManager(*pDiagnosticsEngine, fileManager);

	CompilerInvocation::setLangDefaults(languageOptions, IK_CXX);
	languageOptions.ImplicitInt = 0;

	SetIncludeDirectories();
    //llvm::IntrusiveRefCntPtr<clang::HeaderSearchOptions> headerSearchOptions(new clang::HeaderSearchOptions());
	/*
        headerSearchOptions->ResourceDir = "/opt/llvm_build" "/lib/clang/" CLANG_VERSION_STRING;
    // <Warning!!> -- Platform Specific Code lives here
    // This depends on A) that you're running linux and
    // B) that you have the same GCC LIBs installed that
    // I do.
    // Search through Clang itself for something like this,
    // go on, you won't find it. The reason why is Clang
    // has its own versions of std* which are installed under
    // /usr/local/lib/clang/<version>/include/
    // See somewhere around Driver.cpp:77 to see Clang adding
    // its version of the headers to its include path.
    for (int i = 2; i < argc; i++) 
	{
        headerSearchOptions->AddPath(argv[i], clang::frontend::Angled, false, false);
    }
    // </Warning!!> -- End of Platform Specific Code

	headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include", clang::frontend::Angled,
		false, false); 
	headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include\\c++\\3.4.5" , clang::frontend::Angled,
		false, false); 
	headerSearchOptions->AddPath("D:\\Portable\\MinGW\\include\\c++\\3.4.5\\mingw32" , clang::frontend::Angled,
		false, false); 
	headerSearchOptions->AddPath("D:\\Portable\\MinGW\\lib\\gcc\\mingw32\\3.4.5\\include" , clang::frontend::Angled,
		false, false); 
	*/

	clang::TargetOptions targetOptions;
    targetOptions.Triple = llvm::sys::getDefaultTargetTriple();

    clang::TargetInfo* pTargetInfo =
        clang::TargetInfo::CreateTargetInfo(
            *pDiagnosticsEngine,
            &targetOptions);

    clang::HeaderSearch headerSearch(headerSearchOptions,
                                     sourceManager,
                                     *pDiagnosticsEngine,
                                     languageOptions,
                                     pTargetInfo);

    //auto dir = fileManager.getDirectory("C:\Users\Yuriy\Desktop\Diplom 2018\tests\leak\testsWithLeaks");
    //headerSearch.AddSearchPath(DirectoryLookup(dir, SrcMgr::CharacteristicKind::C_User, false), false);

    clang::CompilerInstance compInst;

    llvm::IntrusiveRefCntPtr<clang::PreprocessorOptions> pOpts( new clang::PreprocessorOptions());
    clang::Preprocessor preprocessor(
        pOpts,
        *pDiagnosticsEngine,
        languageOptions,
        pTargetInfo,
        sourceManager,
        headerSearch,
        compInst);

    clang::FrontendOptions frontendOptions;
    clang::InitializePreprocessor(
        preprocessor,
        *pOpts,
        *headerSearchOptions,
        frontendOptions);


    std::string testRoot = "C:\\Users\\Yuriy\\Desktop\\Diplom 2018\\tests\\leak\\testsWithLeaks\\";
    //{
    //    const clang::FileEntry *pFile = fileManager.getFile(testRoot + "test0.h");
    //    auto fileId = sourceManager.createFileID(pFile, clang::SourceLocation(), clang::SrcMgr::C_User);
    //}
    {
        const clang::FileEntry *pFile = fileManager.getFile(testRoot + "test0_0.cpp");
        auto fileId = sourceManager.createFileID(pFile, clang::SourceLocation(), clang::SrcMgr::C_User);
    }

	const clang::FileEntry* pFile = fileManager.getFile(path.c_str());
    sourceManager.createMainFileID(pFile);

    const clang::TargetInfo &targetInfo = *pTargetInfo;

    clang::IdentifierTable identifierTable(languageOptions);
    clang::SelectorTable selectorTable;

    clang::Builtin::Context builtinContext;
    builtinContext.InitializeTarget(targetInfo);

    //sourceManager.setExternalSLocEntrySource

	astContext = new ASTContext(languageOptions, sourceManager, pTargetInfo, identifierTable, selectorTable,
		builtinContext, 0);
	/*
    clang::ASTContext astContext(
        languageOptions,
        sourceManager,
        pTargetInfo,
        identifierTable,
        selectorTable,
        builtinContext,
        0 /* size_reserve* /);
	*/
   //MyASTConsumer astConsumer;
	
    astConsumer->SetPath(path);
    pTextDiagnosticPrinter->BeginSourceFile(languageOptions, &preprocessor);
    clang::ParseAST(preprocessor, astConsumer, *astContext);
    pTextDiagnosticPrinter->EndSourceFile();
}

void ASTWalker::DumpStmt(clang::Stmt* s)
{
	//s->dump(llvm::errs, astContext->getSourceManager());
	s->dump();
}
