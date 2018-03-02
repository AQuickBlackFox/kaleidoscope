#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "EnumTok.h"
#include "Token.h"
#include "Parser.h"
#include "AST.h"
#include "CodeGen.h"

std::string IdentifierStr;
double NumVal;

extern std::unique_ptr<KaleidoscopeJIT> TheJIT;

void InitializeModuleAndPassManager() {
    TheModule = llvm::make_unique<llvm::Module>("my cool jit", TheContext);
    TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

    TheFPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());

    TheFPM->add(llvm::createInstructionCombiningPass());

    TheFPM->add(llvm::createReassociatePass());

    TheFPM->add(llvm::createGVNPass());

    TheFPM->add(llvm::createCFGSimplificationPass());

    TheFPM->doInitialization();
}

int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 30;
    BinopPrecedence['*'] = 40;

    fprintf(stderr, "ready> ");
    getNextToken();

    TheJIT = llvm::make_unique<KaleidoscopeJIT>();

    InitializeModuleAndPassManager();

    MainLoop();

    TheModule->print(llvm::errs(), nullptr);

}
