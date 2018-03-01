#ifndef KAL_PARSER_H
#define KAL_PARSER_H

#include "Token.h"

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = llvm::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if(!V) return nullptr;

    if(CurTok != ')') return LogError("expected )");
    getNextToken();
    return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken();

    if(CurTok != '(') return llvm::make_unique<VariableExprAST>(IdName);

    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if(CurTok != ')') {
        while(true) {
            if(auto Arg = ParseExpression()){ Args.push_back(std::move(Arg)); }
            else{ return nullptr; }

            if(CurTok == ')') break;

            if(CurTok != ',') return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken();

    return llvm::make_unique<CallExprAST> (IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParsePrimary() {
    switch(CurTok) {
        default:
        return LogError("unknown token when expected an expression");
        case tok_identifier:
        return ParseIdentifierExpr();
        case tok_number:
        return ParseNumberExpr();
        case '(':
        return ParseParenExpr();
    }
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                    std::unique_ptr<ExprAST> LHS) {
    while(true) {
        int TokPrec = GetTokPrecedence();

        if(TokPrec < ExprPrec) return LHS;

        int BinOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if(!RHS) return nullptr;

        int NextPrec = GetTokPrecedence();
        if(TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if(!RHS) return nullptr;
        }

        LHS = llvm::make_unique<BinaryExprAST> (BinOp, std::move(LHS), std::move(RHS));
    }
}

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();

    if(!LHS) return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if(CurTok != tok_identifier) 
        return LogErrorP("Expected function name in prototype");
    
    std::string FnName = IdentifierStr;

    getNextToken();

    if(CurTok != '(') return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;

    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    
    if(CurTok != ')') 
        return LogErrorP("Expected ')' in prototype");
    
    getNextToken();

    return llvm::make_unique<PrototypeAST> (FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();
    auto Proto = ParsePrototype();

    if(!Proto) return nullptr;

    if(auto E = ParseExpression())
        return llvm::make_unique<FunctionAST> (std::move(Proto), std::move(E));
    
    return nullptr;
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if(auto E = ParseExpression()) {
        auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr",
                            std::vector<std::string>());
        return llvm::make_unique<FunctionAST> (std::move(Proto), std::move(E));
    }
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

static void HandleDefinition() {
    if(auto FnAST = ParseDefinition()) {
        if(auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Parsed a function definition.\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleExtern() {
    if(auto ProtoAST = ParseExtern()) {
        if(auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Parsed an extern\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if(auto FnAST = ParseTopLevelExpr()) {
        if(auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Parsed a top-level expr\n");
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void MainLoop() {
    while(true) {
        fprintf(stderr, "ready> ");
        switch(CurTok) {
        case tok_eof:
            return;
        case ';':
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}


#endif