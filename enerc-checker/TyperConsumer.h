#ifndef TYPERCONSUMER_H
#define TYPERCONSUMER_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"

class TyperConsumer : public clang::ASTConsumer {
public:
  TyperConsumer(clang::CompilerInstance& _ci) :
    ci(_ci) {}

  clang::CompilerInstance& ci;

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  void reportError(clang::Stmt* stmt, llvm::StringRef message);
};

#endif
