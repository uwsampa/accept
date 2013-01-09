#ifndef NODETYPER_H
#define NODETYPER_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Type.h"
#include <map>
#include "TyperConsumer.h"

template <class T>
class NodeTyper {
public:
  NodeTyper(TyperConsumer *_controller)
    : controller(_controller),
      curFunction(NULL)
  {}

  // Override these to define your type system.
  virtual T typeForQualString(llvm::StringRef s) = 0;
  virtual T defaultType(clang::Decl *decl) = 0; // decl may be null
  virtual T typeForExpr(clang::Expr *expr) = 0;
  virtual void checkStmt(clang::Stmt *stmt);
  virtual uint32_t flattenType(T type);

  // Probably don't need to override this: assigns types to declarations based
  // on attribute strings and default types. Override if original types need to
  // come from somewhere else.
  virtual T typeForDecl(clang::Decl *decl);

  // "External" interface: memoizes calls to the internal abstract methods
  // that implement the type system.
  virtual T typeOf(clang::Decl *decl);
  virtual T typeOf(clang::Expr *expr);

  // Pass error reports to the controller.
  void typeError(clang::Stmt* stmt, llvm::StringRef message) {
    controller->reportError(stmt, message);
  }

  // Track the current function (called by visitor).
  void setCurFunction(clang::FunctionDecl *fundecl) {
    curFunction = fundecl;
  }

protected:
  TyperConsumer *controller;

  // Type memoization.
  std::map<clang::Decl*, T> declTypes;
  std::map<clang::Expr*, T> exprTypes;

  // Function scope tracking.
  clang::FunctionDecl *curFunction;

// Helpers for adding flattened types to AST nodes.
private:
  clang::QualType withQuals(clang::QualType oldT, T type) {
    // Don't qualify the null type. (Leads to a segfault.)
    if (oldT.isNull())
      return oldT;
    // Don't add anything if this is the default type (zero). This avoids making
    // a new (unique) QualType in the folding set for fast-qualified types.
    // Otherwise, there would be two versions of "int": fast-qualified int and
    // accidentally-ExtQuals-ified int. This leads to horrible type-checking
    // problems.
    // NOTE: As a side effect, we assume here that the default type is always
    // zero when flattened to an integer! This places a weird constraint on the
    // implementation of type systems.
    uint32_t flatType = flattenType(type);
    if (flatType == 0)
      return oldT;

    clang::Qualifiers quals = oldT.getQualifiers();
    quals = clang::Qualifiers::fromOpaqueValue(quals.getAsOpaqueValue());
    quals.setCustomQuals(flatType);
    clang::QualType newType = controller->ci.getASTContext().
                              getExtQualType(oldT.getTypePtr(), quals);
    return newType;
  }
  void setQuals(clang::Expr *expr, T type) {
    if (!expr) return;

    clang::QualType newType = withQuals(expr->getType(), type);
    expr->setType(newType);
  }
  void setQuals(clang::Decl *decl, T type) {
    if (!decl) return;

    // Value (variable, field, function, etc.) declarations.
    clang::ValueDecl *vdecl = llvm::dyn_cast<clang::ValueDecl>(decl);
    if (vdecl) {
      // For function declarations, qualify the return type. Otherwise, qualify
      // the type itself.
      if (clang::FunctionDecl *fundecl = clang::dyn_cast<clang::FunctionDecl>(decl)) {
        setReturnQuals(fundecl, type);
      } else {
        vdecl->setType(withQuals(vdecl->getType(), type));
      }
    } else {
      // TODO: typedefs, etc.?
    }
  }
  // Stolen from Joe.
  void setReturnQuals(clang::FunctionDecl *fd, T type) {
    // Skip void functions.
    if ( fd->getResultType().getTypePtr()->isVoidType() )
      return;
    
    // Get parameter types (to preserve them in the new type).
    std::vector<clang::QualType> args =
        std::vector<clang::QualType>(fd->getNumParams());
    {
      int i = 0;
      clang::FunctionDecl::param_const_iterator pi = fd->param_begin(); 
      for ( ; pi != fd->param_end(); ++pi, ++i ) {
        args.at(i) = (*pi)->getType();
      }
    }
    
    // Add qualifiers to return type.
    clang::QualType newRetType = withQuals(fd->getResultType(), type);
    
    clang::FunctionProtoType::ExtProtoInfo epi; // TODO: is it ok to use the default ctor here?
    const clang::FunctionProtoType* funT;
    if ( (funT = fd->getType()->getAs<clang::FunctionProtoType>()) ) {
      epi = funT->getExtProtoInfo();
    }
    
    // create a new function type
    clang::QualType newType = fd->getASTContext().getFunctionType(
                                  newRetType, &(args.front()), 
                                  fd->getNumParams(), epi);

    fd->setType(newType);
  }
};

template <class T>
T NodeTyper<T>::typeForDecl(clang::Decl *decl) {
  if (decl == NULL) {
    return defaultType(decl);
  }

  clang::TypeQualifierAttr *tqa = decl->getAttr<clang::TypeQualifierAttr>();
  if (tqa) {
    llvm::StringRef name = tqa->getName();
    return typeForQualString(name);
  } else {
    return defaultType(decl);
  }
}

template <class T>
uint32_t NodeTyper<T>::flattenType(T type) {
  // A meaningless implementation for systems that don't need information
  // in generated code.
  return 0;
}


template <class T>
void NodeTyper<T>::checkStmt(clang::Stmt *stmt) {
  // Default implementation accepts everything.
}


// Memoized interface.

template <class T>
T NodeTyper<T>::typeOf(clang::Decl *decl) {
  if (declTypes.count(decl)) {
    return declTypes[decl];
  } else {
    T type = typeForDecl(decl);
    if (decl != NULL) {
      // For non-null declarations, store the type in our memoization and in the
      // node itself.
      declTypes[decl] = type;
      setQuals(decl, type);
    }
    return type;
  }
}

template <class T>
T NodeTyper<T>::typeOf(clang::Expr *expr) {
  if (exprTypes.count(expr)) {
    return exprTypes[expr];
  } else {
    T type = typeForExpr(expr);
    exprTypes[expr] = type;
    setQuals(expr, type);
    return type;
  }
}

#endif
