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

int main() {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 30;
    BinopPrecedence['*'] = 40;

    fprintf(stderr, "ready> ");
    getNextToken();

    TheModule = llvm::make_unique<llvm::Module>("my cool jit", TheContext);

    MainLoop();

    TheModule->print(llvm::errs(), nullptr);

}
