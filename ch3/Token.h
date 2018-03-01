#ifndef KAL_TOKEN_H
#define KAL_TOKEN_H

#include "AST.h"

extern std::string IdentifierStr;
extern double NumVal;

static int gettok() {
    static int LastChar = ' ';

    while(isspace(LastChar)) LastChar = getchar();

    if(isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while(isalnum(LastChar = getchar())) IdentifierStr += LastChar;

        if(IdentifierStr == "def") return tok_def;

        if(IdentifierStr == "extern") return tok_extern;

        return tok_identifier;
    }

    if(isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if(LastChar == '#') {
        while(LastChar != EOF && LastChar != '\n' && LastChar != '\r') {
            LastChar = getchar();
        }

        if(LastChar != EOF) return gettok();
    }

    if(LastChar == EOF) return tok_eof;

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
    if(!isascii(CurTok)) return -1;

    int TokPrec = BinopPrecedence[CurTok];

    if(TokPrec <= 0) return -1;

    return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}


#endif