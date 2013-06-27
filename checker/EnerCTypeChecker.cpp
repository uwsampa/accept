#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "NodeTyper.h"
#include "TyperVisitor.h"
#include "llvm/Support/Debug.h"
#include "enerc.h"
#include <iostream>

using namespace clang;

namespace {

// The types in the EnerC type system.
const uint32_t ecPrecise = 0;
const uint32_t ecApprox = 1;

// The typer: assign types to AST nodes.
class EnerCTyper : public NodeTyper {
public:
  virtual uint32_t typeForQualString(llvm::StringRef s);
  virtual uint32_t defaultType(clang::Decl *decl);
  virtual uint32_t typeForExpr(clang::Expr *expr);
  virtual void checkStmt(clang::Stmt *stmt);
  virtual QualType withQuals(QualType oldT, uint32_t type);
  bool compatible(uint32_t lhs, uint32_t rhs);
  bool compatible(clang::QualType lhs, clang::QualType rhs);
  bool qualsEqual(clang::QualType lhs, clang::QualType rhs);
  void assertFlow(clang::QualType type, clang::Expr *expr);

  EnerCTyper(TyperConsumer *_controller) :
    NodeTyper(_controller) {}

private:
  void checkCondition(clang::Expr *cond);
};

// Plugin hooks for Clang driver.
class EnerCTypeCheckerAction: public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer( CompilerInstance &CI, llvm::StringRef ) {
    // Pass the type-checking visitor back to Clang so we get called to
    // traverse all ASTs.
    return new TyperVisitor<EnerCTyper>( CI );
  }

  bool ParseArgs( const CompilerInstance &CI,
                  const std::vector<std::string>& args ) {
    // No arguments.
    return true;
  }

  void PrintHelp( llvm::raw_ostream& ros ) {
    ros << "TODO\n";
  }

  virtual ~EnerCTypeCheckerAction() {
  }
};

} // end namespace

// Register the plugin.
static FrontendPluginRegistry::Add<EnerCTypeCheckerAction> X(
    "enerc-type-checker", "perform EnerC type checking" );


// Assign qualifiers to declarations based on type annotations.
uint32_t EnerCTyper::typeForQualString(llvm::StringRef s) {
  if (s.equals("approx")) {
    return ecApprox;
  } else if (s.equals("precise")) {
    return ecPrecise;
  } else {
    std::cerr << "INVALID QUALIFIER: " << s.str() << "\n";
    return ecPrecise;
  }
}

// Default type for unannotated declarations.
uint32_t EnerCTyper::defaultType(clang::Decl *decl) {
  return ecPrecise;
}

// Type compatibility: the information flow rule.
bool EnerCTyper::compatible(uint32_t lhs, uint32_t rhs) {
  if (lhs == ecPrecise) {
    if (rhs == ecPrecise)
      return true;
    else
      return false;
  } else {
    return true;
  }
}

bool EnerCTyper::compatible(clang::QualType lhs, clang::QualType rhs) {
  if (lhs.isNull() || rhs.isNull())
    return true;

  // Current type level.
  if (!compatible(lhs.getQualifiers().getCustomQuals(),
                  rhs.getQualifiers().getCustomQuals())) {
    return false;
  }

  // Possibly recuse into referent types. After the first level, all
  // qualifiers must be *equal*. This incompatibility prevents unsoundness
  // that arises when different types alias.
  if (controller->ci.getASTContext().UnwrapSimilarPointerTypes(lhs, rhs)) {
    return qualsEqual(lhs, rhs);
  } else {
    return true;
  }
}

bool EnerCTyper::qualsEqual(clang::QualType lhs, clang::QualType rhs) {
  if (lhs.isNull() || rhs.isNull())
    return true;
  if (lhs.getQualifiers().getCustomQuals() !=
      rhs.getQualifiers().getCustomQuals()) {
    return false;
  }
  if (controller->ci.getASTContext().UnwrapSimilarPointerTypes(lhs, rhs)) {
    return qualsEqual(lhs, rhs);
  } else {
    return true;
  }
}

void EnerCTyper::assertFlow(clang::QualType type, clang::Expr *expr) {
  // Special-case some function calls (malloc and math).
  clang::Expr *innerExpr = expr->IgnoreImplicit();
  if (clang::CastExpr *castExpr = llvm::dyn_cast<clang::CastExpr>(innerExpr)) {
    innerExpr = castExpr->getSubExpr();
    innerExpr = innerExpr->IgnoreImplicit();
  }
  if (clang::CallExpr *callExpr = llvm::dyn_cast<clang::CallExpr>(innerExpr))
    if (clang::FunctionDecl *fdecl = callExpr->getDirectCallee())
      if (fdecl->getNameAsString() == "malloc")
        return;

  // Special-case literals (APPROX int* p = 0;).
  if (llvm::isa<clang::IntegerLiteral>(innerExpr))
    return;

  // Other cases.
  assertCompatible(type, expr, "precision flow violation");
}


// Give types to expressions.
uint32_t EnerCTyper::typeForExpr(clang::Expr *expr) {
  // Tolerate null.
  if (!expr)
    return ecPrecise;

  switch ( expr->getStmtClass() ) {

  // LITERALS
  case clang::Stmt::CharacterLiteralClass:
  case clang::Stmt::IntegerLiteralClass:
  case clang::Stmt::StringLiteralClass:
  case clang::Stmt::ImaginaryLiteralClass:
  case clang::Stmt::CompoundLiteralExprClass:
  case clang::Stmt::CXXBoolLiteralExprClass:
  case clang::Stmt::CXXNullPtrLiteralExprClass:
  case clang::Stmt::GNUNullExprClass:
  case clang::Stmt::FloatingLiteralClass:
  case clang::Stmt::UnaryTypeTraitExprClass:
  case clang::Stmt::CXXConstructExprClass:
  case clang::Stmt::CXXNewExprClass:
  case clang::Stmt::CXXDeleteExprClass:
  case clang::Stmt::OffsetOfExprClass:
  case clang::Stmt::SizeOfPackExprClass:
    return ecPrecise;

  // VARIABLE REFERENCES
//  case clang::Stmt::BlockDeclRefExprClass: {
//    clang::BlockDeclRefExpr* tex = cast<clang::BlockDeclRefExpr>(expr);
//    return typeOf(tex->getDecl());
//  }
  case clang::Stmt::DeclRefExprClass:
    return CL_LEAVE_UNCHANGED;
  case clang::Stmt::DependentScopeDeclRefExprClass: {
    DEBUG(llvm::errs() << "UNSOUND: template argument untypable\n");
    return ecPrecise;
  }
  case clang::Stmt::MemberExprClass:
    return CL_LEAVE_UNCHANGED;

  // CASTS
  case clang::Stmt::ImplicitCastExprClass:
  case clang::Stmt::CStyleCastExprClass:
  case clang::Stmt::CXXFunctionalCastExprClass:
  case clang::Stmt::CXXConstCastExprClass:
  case clang::Stmt::CXXDynamicCastExprClass:
  case clang::Stmt::CXXReinterpretCastExprClass:
  case clang::Stmt::CXXStaticCastExprClass:
    return CL_LEAVE_UNCHANGED;


  // BINARY OPERATORS
  case clang::Stmt::BinaryOperatorClass: {
    clang::BinaryOperator *bop = llvm::cast<BinaryOperator>(expr);
    DEBUG(llvm::errs() << "binary operator " << bop->getOpcode() << "\n");
    DEBUG(bop->getLHS()->dump());
    DEBUG(bop->getRHS()->dump());
    uint32_t ltype = typeOf(bop->getLHS());
    uint32_t rtype = typeOf(bop->getRHS());

    DEBUG(llvm::errs() << "left " << ltype << " right " << rtype << "\n");

    switch (bop->getOpcode()) {
      case BO_Mul:
      case BO_Div:
      case BO_Rem:
      case BO_Add:
      case BO_Sub:
      case BO_Shl:
      case BO_Shr:
      case BO_LT:
      case BO_GT:
      case BO_LE:
      case BO_GE:
      case BO_EQ:
      case BO_NE:
      case BO_And:
      case BO_Xor:
      case BO_Or:
      case BO_LAnd:
      case BO_LOr:
        DEBUG(llvm::errs() << "arithmetic\n");
        if (ltype == ecApprox || rtype == ecApprox)
          return ecApprox;
        else
          return CL_LEAVE_UNCHANGED;

      case BO_Assign:
      case BO_MulAssign:
      case BO_DivAssign:
      case BO_RemAssign:
      case BO_AddAssign:
      case BO_SubAssign:
      case BO_ShlAssign:
      case BO_ShrAssign:
      case BO_AndAssign:
      case BO_XorAssign:
      case BO_OrAssign:
        DEBUG(llvm::errs() << "assignment\n");
        assertFlow(bop->getLHS()->getType(), bop->getRHS());
        return ltype;

      case BO_Comma:
        {
          // Endorsements are encoded as comma expressions!
          clang::IntegerLiteral *literal =
              dyn_cast<clang::IntegerLiteral>(bop->getLHS());
          if (literal && literal->getValue() == TAG_ENDORSEMENT) {
            // This is an endorsement of the right-hand expression.
            return ecPrecise;
          } else if (literal && literal->getValue() == TAG_DEDORSEMENT) {
            // This is a "dedorsement" of the right-hand expression ---
            // converting to approximate for the purposes of pointer
            // assignment.
            return ecApprox;
          } else {
            // An ordinary comma expression.
            return CL_LEAVE_UNCHANGED;
          }
        }

      case BO_PtrMemD:
      case BO_PtrMemI:
        DEBUG(llvm::errs() << "UNSOUND: unimplemented: pointer-to-mem\n");
        return ecPrecise;
    }
  }


  // UNARY OPERATORS
  case clang::Stmt::UnaryOperatorClass:
    return CL_LEAVE_UNCHANGED;


  // ARRAYS
  case clang::Stmt::ArraySubscriptExprClass:
    return CL_LEAVE_UNCHANGED;


  // FUNCTION CALLS
  case clang::Stmt::CXXMemberCallExprClass: 
  case clang::Stmt::CXXOperatorCallExprClass: 
  case clang::Stmt::CallExprClass: {
    clang::CallExpr* call = cast<CallExpr>(expr);

    clang::FunctionDecl *callee = call->getDirectCallee();
    if (!callee) {
      DEBUG(llvm::errs() << "UNSOUND: could not find callee for ");
      DEBUG(call->dump());
      DEBUG(llvm::errs() << "\n");
      return ecPrecise;
    }

    // Special cases for certain pointer-related library functions.
    // The funny-looking names below are functions hidden in macro expansions
    // of the standard names.
    std::string name = callee->getNameAsString();
    if (name == "free" || name == "memcpy" || name == "memset" ||
        name == "__builtin_object_size" || name == "__builtin___memcpy_chk" ||
        name == "__inline_memcpy_chk" || name == "__builtin___memset_chk" ||
        name == "__inline_memset_chk" ||
        name == "__builtin_expect"  // assert
        ) {

      // We want to leave the arguments unchecked, but we have to change each
      // argument expression's type to avoid assertion failures later in the
      // Clang pipeline.
      for (clang::CallExpr::arg_iterator ai = call->arg_begin();
           ai != call->arg_end(); ++ai) {
        (*ai)->setType(withQuals((*ai)->getType(), ecPrecise));
      }

      return CL_LEAVE_UNCHANGED;
    }

    // Special cases for standard unary math functions.
    if (name == "abs" || name == "cos") {
      Expr *arg = call->getArg(0);
      // Parametric-esque: return qualifier is the argument qualifier.
      uint32_t outType = typeOf(arg);
      // Ignore type errors on the argument.
      arg->setType(withQuals(arg->getType(), ecPrecise));
      return outType;
    }

    // Check parameters.
    // XXX this should be moved to the framework
    // XXX variadic, default args: when param/arg list lengths are not equal
    clang::FunctionDecl::param_iterator pi = callee->param_begin();
    clang::CallExpr::arg_iterator ai = call->arg_begin();
    for (; pi != callee->param_end() && ai != call->arg_end(); ++pi, ++ai) {
      assertFlow((*pi)->getType(), *ai);
    }

    DEBUG(llvm::errs() << "callee: ");
    DEBUG(callee->dump());
    DEBUG(llvm::errs() << " type: " << typeOf(callee) << "\n");

    return CL_LEAVE_UNCHANGED;
  }


  // MISC
  case clang::Stmt::ParenExprClass:
    return CL_LEAVE_UNCHANGED;
                                           
  default:
    return CL_LEAVE_UNCHANGED;
  }
}


// Apply EnerC types to the Clang type structures.
QualType EnerCTyper::withQuals(QualType oldT, uint32_t type) {
  if (oldT.isNull())
    return oldT;

  ASTContext &ctx = controller->ci.getASTContext();

  if (const PointerType *ptrT = dyn_cast<PointerType>(oldT.getTypePtr())) {

    // For pointers, apply to the pointed-to type (recursively).
    QualType innerType = withQuals(ptrT->getPointeeType(), type);
    innerType = ctx.getCanonicalType(innerType);
    QualType out = ctx.getPointerType(innerType);
    out = replaceQuals(out, oldT.getQualifiers());

    if (innerType->isVoidType() && type) {
      llvm::errs() << "approx void\n";
      out.dump();
    }

    return out;

  } else if (const ArrayType *arrT = dyn_cast<ArrayType>(oldT.getTypePtr())) {

    // Array types. We have four different kinds.
    QualType innerType = withQuals(arrT->getElementType(), type);
    QualType out;
    if (const ConstantArrayType *at = dyn_cast<ConstantArrayType>(arrT)) {
      out = ctx.getConstantArrayType(innerType, at->getSize(),
                                     at->getSizeModifier(),
                                     at->getIndexTypeCVRQualifiers());
    } else if (const DependentSizedArrayType *at =
               dyn_cast<DependentSizedArrayType>(arrT)) {
      out = ctx.getDependentSizedArrayType(innerType, at->getSizeExpr(),
                                           at->getSizeModifier(),
                                           at->getIndexTypeCVRQualifiers(),
                                           at->getBracketsRange());
    } else if (const IncompleteArrayType *at =
               dyn_cast<IncompleteArrayType>(arrT)) {
      out = ctx.getIncompleteArrayType(innerType,
                                       at->getSizeModifier(),
                                       at->getIndexTypeCVRQualifiers());
    } else if (const VariableArrayType *at =
               dyn_cast<VariableArrayType>(arrT)) {
      out = ctx.getVariableArrayType(innerType, at->getSizeExpr(),
                                     at->getSizeModifier(),
                                     at->getIndexTypeCVRQualifiers(),
                                     at->getBracketsRange());
    } else {
      llvm::errs() << "UNKNOWN ARRAY TYPE KIND\n";
    }
    out = replaceQuals(out, oldT.getQualifiers());
    return out;

  } else {
    // For non-pointers, apply to this type.
    return NodeTyper::withQuals(oldT, type);
  }
}

void EnerCTyper::checkCondition(clang::Expr *cond) {
  // Found a statement with a condition.
  if (typeOf(cond) == ecApprox) {
    typeError(cond, "approximate condition");
  }
}

// Checking types in statements. (No type assignment here.)
void EnerCTyper::checkStmt(clang::Stmt *stmt) {
  switch (stmt->getStmtClass()) {
    // Check for approximate conditions.
    case clang::Stmt::IfStmtClass:
      checkCondition(cast<clang::IfStmt>(stmt)->getCond());
      break;
    case clang::Stmt::ForStmtClass:
      checkCondition(cast<clang::ForStmt>(stmt)->getCond());
      break;
    case clang::Stmt::DoStmtClass:
      checkCondition(cast<clang::DoStmt>(stmt)->getCond());
      break;
    case clang::Stmt::WhileStmtClass:
      checkCondition(cast<clang::WhileStmt>(stmt)->getCond());
      break;
    case clang::Stmt::SwitchStmtClass:
      checkCondition(cast<clang::SwitchStmt>(stmt)->getCond());
      break;

    // Check function return type.
    case clang::Stmt::ReturnStmtClass:
      {
        clang::Expr *expr = cast<clang::ReturnStmt>(stmt)->getRetValue();
        if (expr && curFunction) {
          assertFlow(curFunction->getResultType(), expr);
        }
      }
      break;

    // Check variable initialization.
    case clang::Stmt::DeclStmtClass:
      {
        clang::DeclStmt *dstmt = cast<clang::DeclStmt>(stmt);
        for (DeclStmt::const_decl_iterator i = dstmt->decl_begin();
             i != dstmt->decl_end(); ++i) {
          clang::VarDecl *vdecl = dyn_cast<clang::VarDecl>(*i);
          if (vdecl && vdecl->hasInit()) {
            assertFlow(vdecl->getType(), vdecl->getInit());
          }
        }
      }
      break;

    default:
      break;
  }
}
