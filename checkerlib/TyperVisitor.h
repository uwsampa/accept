#ifndef TYPERVISITOR_H
#define TYPERVISITOR_H

#include "clang/AST/AST.h"
#include "clang/AST/Type.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/Support/Debug.h"
#include <map>
#include "TyperConsumer.h"

template <class TyperT>
class TyperVisitor :
  public clang::RecursiveASTVisitor< TyperVisitor<TyperT> >,
  public TyperConsumer {

  typedef clang::RecursiveASTVisitor< TyperVisitor<TyperT> > _super;

public:
  TyperT typer;

  TyperVisitor<TyperT>( clang::CompilerInstance& _ci ) :
      TyperConsumer( _ci ), typer(TyperT(this)) {
  }
  bool HandleTopLevelDecl( clang::DeclGroupRef DG );
  bool shouldUseDataRecursionFor( clang::Stmt *S );

  // Tree walking.
  bool TraverseDecl( clang::Decl* decl );
  bool TraverseStmt( clang::Stmt* stmt );
};


// Type assignment walking.
// Note: The Clang documentation suggests that clients are supposed to use the
// Visit* methods. We instead use the Traverse* methods so that we get to see
// the expression trees in bottom-up order. (Visit* is top-down (preorder).)
// But we need to see function declarations before their bodies (to correctly
// assign their types before checking return statements, etc.).

template <class TyperT>
bool TyperVisitor<TyperT>::TraverseDecl( clang::Decl* decl ) {
  // Is this declaration a function? If so, keep track of this.
  clang::FunctionDecl *fundecl = NULL;
  if (decl) {
    fundecl = clang::dyn_cast<clang::FunctionDecl>(decl);
    if (fundecl) {
      typer.setCurFunction(fundecl);
    }
  }

  typer.process(decl);
  _super::TraverseDecl(decl);

  if (fundecl) {
    // We rebuild the function's type based on any modifications to its
    // parameter types (which may be modified in that recursive call to
    // TraverseDecl).
    // XXX This probably doesn't work for recursive calls since we have already
    // seen the body of the function.
    typer.rebuildFuncType(fundecl);
    typer.setCurFunction(NULL);
  }

  // XXX It's at this point that we should adapt the types of function
  // pointers...

  return true;
}

template <class TyperT>
bool TyperVisitor<TyperT>::TraverseStmt( clang::Stmt* stmt ) {
  _super::TraverseStmt(stmt);
  if (stmt != NULL) {
    if (clang::Expr* expr = llvm::dyn_cast<clang::Expr>(stmt)) {
      typer.process(expr);
    } else {
      typer.checkStmt(stmt);
    }
  }
  return true;
}


// Called by the plugin API to begin traversal.
template <class TyperT>
bool TyperVisitor<TyperT>::HandleTopLevelDecl( clang::DeclGroupRef DG ) {
  for ( clang::DeclGroupRef::iterator i = DG.begin(), e = DG.end();
        i != e; ++i ) {
    clang::NamedDecl *ND = clang::dyn_cast<clang::NamedDecl>( *i );
    if ( ND ) {
      TraverseDecl( ND );
    }
  }
  return true;
}

// Disable "data recursion", which seems to just mean using an explicit queue
// instead of the call stack. With this optimization enabled, some expressions
// were missed by TraverseStmt.
template <class TyperT>
bool TyperVisitor<TyperT>::shouldUseDataRecursionFor( clang::Stmt *S ) {
  return false;
}

#endif
