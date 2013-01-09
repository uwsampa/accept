#ifndef TYPERVISITOR_H
#define TYPERVISITOR_H

#include "clang/AST/AST.h"
#include "clang/AST/Type.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/Support/Debug.h"
#include <map>
#include "TyperConsumer.h"

template <class TypeT, class TyperT>
class TyperVisitor :
  public clang::RecursiveASTVisitor< TyperVisitor<TypeT, TyperT> >,
  public TyperConsumer {

  typedef clang::RecursiveASTVisitor< TyperVisitor<TypeT, TyperT> > _super;

public:
  TyperT typer;

  TyperVisitor<TypeT, TyperT>( clang::CompilerInstance& _ci ) :
      TyperConsumer( _ci ), typer(TyperT(this)) {
  }
  virtual bool HandleTopLevelDecl( clang::DeclGroupRef DG );

  // Tree walking.
  virtual bool TraverseDecl( clang::Decl* decl );
  virtual bool TraverseStmt( clang::Stmt* stmt );
};


// Type assignment walking.

template <class TypeT, class TyperT>
bool TyperVisitor<TypeT, TyperT>::TraverseDecl( clang::Decl* decl ) {
  // Is this declaration a function? If so, keep track of this.
  clang::FunctionDecl *fundecl = NULL;
  if (decl) {
    fundecl = clang::dyn_cast<clang::FunctionDecl>(decl);
    if (fundecl) {
      typer.setCurFunction(fundecl);
    }
  }

  _super::TraverseDecl(decl);
  typer.typeOf(decl); // Force type application for the declaration.

  if (fundecl) {
    typer.setCurFunction(NULL);
  }

  return true;
}

template <class TypeT, class TyperT>
bool TyperVisitor<TypeT, TyperT>::TraverseStmt( clang::Stmt* stmt ) {
  _super::TraverseStmt(stmt);
  if (stmt != NULL) {
    if (clang::Expr* expr = llvm::dyn_cast<clang::Expr>(stmt)) {
      typer.typeOf(expr);
    } else {
      typer.checkStmt(stmt);
    }
  }
  return true;
}

// Called by the plugin API to begin traversal.
template <class TypeT, class TyperT>
bool TyperVisitor<TypeT, TyperT>::HandleTopLevelDecl( clang::DeclGroupRef DG ) {
  for ( clang::DeclGroupRef::iterator i = DG.begin(), e = DG.end();
        i != e; ++i ) {
    clang::NamedDecl *ND = clang::dyn_cast<clang::NamedDecl>( *i );
    if ( ND ) {
      TraverseDecl( ND );
    }
  }
  return true;
}

#endif
