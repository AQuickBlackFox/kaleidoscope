#ifndef KAL_CODEGEN_H
#define KAL_CODEGEN_H

#include "AST.h"

using namespace llvm::orc;

static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, llvm::Value*> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

llvm::Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

llvm::Function *getFunction(std::string Name) {
    if(auto *F = TheModule->getFunction(Name)) return F;

    auto FI = FunctionProtos.find(Name);
    if(FI != FunctionProtos.end())
        return FI->second->codegen();

    return nullptr;
}

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(Val));
}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = NamedValues[Name];
    if(!V) return LogErrorV("Unknown variable name");
    return V;
}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *L = LHS->codegen();
    llvm::Value *R = RHS->codegen();

    if(!L || !R) return nullptr;

    switch(Op) {
        case '+':
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFSub(L, R, "subtmp");
        case '*':
            return Builder.CreateFMul(L, R, "multmp");
        case '<':
            return Builder.CreateFCmpULT(L, R, "cmptmp");
            return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

llvm::Value *CallExprAST::codegen() {
    llvm::Function *CalleeF = TheModule->getFunction(Callee);
    if(!CalleeF) return LogErrorV("Unknown function referenced");

    if(CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");
    
    std::vector<llvm::Value*> ArgsV;

    for(unsigned i=0, e = Args.size(); i!=e;++i) {
        ArgsV.push_back(Args[i]->codegen());
        if(!ArgsV.back()) return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Function *PrototypeAST::codegen() {
    std::vector<llvm::Type*> Doubles(Args.size(), llvm::Type::getDoubleTy(TheContext));

    llvm::FunctionType *FT =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

    llvm::Function *F =
        llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());
    unsigned Idx = 0;
    for(auto &Arg : F->args()) {
        Arg.setName(Args[Idx++]);
    }

    return F;
}

llvm::Function *FunctionAST::codegen() {
    auto &P = *Proto;

    FunctionProtos[Proto->getName()] = std::move(Proto);

    llvm::Function *TheFunction = getFunction(Proto->getName());

    if(!TheFunction) return nullptr;

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();
    for(auto &Arg : TheFunction->args()) NamedValues[Arg.getName()] = &Arg;

    if(llvm::Value *RetVal = Body->codegen()) {
        Builder.CreateRet(RetVal);
        verifyFunction(*TheFunction);
        TheFPM->run(*TheFunction);
        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}



#endif
