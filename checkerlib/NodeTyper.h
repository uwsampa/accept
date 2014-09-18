#ifndef NODETYPER_H
#define NODETYPER_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Type.h"
#include <map>
#include "TyperConsumer.h"

const uint32_t CL_LEAVE_UNCHANGED = 1 << 31;

class NodeTyper {
public:
  NodeTyper(TyperConsumer *_controller)
    : controller(_controller),
      curFunction(NULL)
  {}

  // Override these to define your type system.
  virtual uint32_t typeForQualString(llvm::StringRef s) = 0;
  virtual uint32_t defaultType(clang::Decl *decl) = 0; // decl may be null
  virtual uint32_t typeForExpr(clang::Expr *expr) = 0;
  virtual void checkStmt(clang::Stmt *stmt);

  // Probably don't need to override this: assigns types to declarations based
  // on attribute strings and default types. Override if original types need to
  // come from somewhere else.
  virtual uint32_t typeForDecl(clang::Decl *decl);

  // Override these (either the qualifier version or the full-type version) to
  // determine when assignments, function calls, etc. should be allowed to
  // adapt types.
  virtual bool compatible(uint32_t lhs, uint32_t rhs);
  virtual bool compatible(clang::QualType lhs, clang::QualType rhs);

  // Shortcuts to retrieve the current customQuals of an AST node.
  uint32_t typeOf(clang::Decl *decl);
  uint32_t typeOf(clang::Expr *expr);

  // Error message reporting.
  void typeError(clang::Stmt* stmt, llvm::StringRef message);
  void typeError(clang::Decl* decl, llvm::StringRef message);
  void assertCompatible(clang::QualType type, clang::Expr *expr,
                        llvm::StringRef error);
  
  // Track the current function (called by visitor).
  void setCurFunction(clang::FunctionDecl *fundecl) {
    curFunction = fundecl;
  }

  virtual clang::QualType withQuals(clang::QualType oldT, uint32_t type);

  // Hooks for visitor.
  void process(clang::Expr *expr);
  void process(clang::Decl *decl);
  void rebuildFuncType(clang::FunctionDecl *fd);

protected:
  TyperConsumer *controller;

  // Function scope tracking.
  clang::FunctionDecl *curFunction;

  // Helpers.
  clang::QualType copyQuals(clang::QualType fromType, clang::QualType toType);
  clang::QualType innerType(clang::QualType type);
  clang::QualType replaceQuals(clang::QualType type, clang::Qualifiers quals);

private:
  void setQuals(clang::Expr *expr, uint32_t type);
  void setQuals(clang::Decl *decl, uint32_t type);
  void setReturnQuals(clang::FunctionDecl *fd, uint32_t type);
  void rebuildFuncType(clang::FunctionDecl *fd, clang::QualType newRetType);
};

#endif
