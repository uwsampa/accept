#include "NodeTyper.h"

using namespace clang;
using namespace llvm;

uint32_t NodeTyper::typeForDecl(Decl *decl) {
  if (decl == NULL) {
    return defaultType(decl);
  }

  TypeQualifierAttr *tqa = decl->getAttr<TypeQualifierAttr>();
  if (tqa) {
    StringRef name = tqa->getName();
    return typeForQualString(name);
  } else {
    return defaultType(decl);
  }
}

void NodeTyper::checkStmt(Stmt *stmt) {
  // Default implementation accepts everything.
}


// Convenient shortcuts to get the *current* custom quals of a declaration or
// expression.

uint32_t NodeTyper::typeOf(Decl *decl) {
  if (!decl)
    return 0;
  ValueDecl *vdecl = dyn_cast<ValueDecl>(decl);
  if (!vdecl)
    return 0;
  return vdecl->getType().getQualifiers().getCustomQuals();
}

uint32_t NodeTyper::typeOf(Expr *expr) {
  if (!expr)
    return 0;
  return expr->getType().getQualifiers().getCustomQuals();
}


// Compatibility checking.

bool NodeTyper::compatible(uint32_t lhs, uint32_t rhs) {
  // Default implementation allows everything.
  return true;
}
bool NodeTyper::compatible(QualType lhs, QualType rhs) {
  if (lhs.isNull() || rhs.isNull())
    return true;

  // Current type level.
  if (!compatible(lhs.getQualifiers().getCustomQuals(),
                  rhs.getQualifiers().getCustomQuals())) {
    return false;
  }

  // Possibly recuse into referent types.
  if (controller->ci.getASTContext().UnwrapSimilarPointerTypes(lhs, rhs)) {
    return compatible(lhs, rhs);
  } else {
    return true;
  }
}


// Error messages.

void NodeTyper::typeError(Stmt* stmt, StringRef message) {
  controller->reportError(stmt, message);
}
void NodeTyper::typeError(Decl* decl, StringRef message) {
  controller->reportError(decl, message);
}
void NodeTyper::assertCompatible(QualType type, Expr *expr,
                                 StringRef error) {
  if (!compatible(type, expr->getType())) {
    typeError(expr, error);
  }
}


// Type assignment.

QualType NodeTyper::withQuals(QualType oldT, uint32_t type) {
  // Don't qualify the null type. (Leads to a segfault.)
  if (oldT.isNull())
    return oldT;

  Qualifiers quals = oldT.getQualifiers();
  quals.setCustomQuals(type);
  return replaceQuals(oldT, quals);
}

void NodeTyper::setQuals(Expr *expr, uint32_t type) {
  if (!expr) return;

  QualType newType = withQuals(expr->getType(), type);
  expr->setType(newType);
}

void NodeTyper::setQuals(Decl *decl, uint32_t type) {
  if (!decl) return;

  // Value (variable, field, function, etc.) declarations.
  ValueDecl *vdecl = dyn_cast<ValueDecl>(decl);
  if (vdecl) {
    // For function declarations, qualify the return type. Otherwise, qualify
    // the type itself.
    if (FunctionDecl *fundecl = dyn_cast<FunctionDecl>(decl)) {
      setReturnQuals(fundecl, type);
    } else {
      vdecl->setType(withQuals(vdecl->getType(), type));
    }
  } else {
    // TODO: typedefs, etc.?
  }
}

// Stolen from Joe.
void NodeTyper::rebuildFuncType(FunctionDecl *fd,
                                QualType newRetType) {
  // Get parameter types (to preserve them in the new type). This has the
  // *additional side effect* of updating the function's type to incorporate
  // any modifications to the types of the arguments.
  std::vector<QualType> args =
      std::vector<QualType>(fd->getNumParams());
  {
    int i = 0;
    FunctionDecl::param_const_iterator pi = fd->param_begin(); 
    for ( ; pi != fd->param_end(); ++pi, ++i ) {
      args.at(i) = (*pi)->getType();
    }
  }
  
  FunctionProtoType::ExtProtoInfo epi; // TODO: is it ok to use the default ctor here?
  const FunctionProtoType* funT;
  if ( (funT = fd->getType()->getAs<FunctionProtoType>()) ) {
    epi = funT->getExtProtoInfo();
  }
  
  // create a new function type
  QualType newType = fd->getASTContext().getFunctionType(
                                newRetType, &(args.front()), 
                                fd->getNumParams(), epi);

  fd->setType(newType);
}
void NodeTyper::rebuildFuncType(FunctionDecl *fd) {
  rebuildFuncType(fd, fd->getResultType());
}
void NodeTyper::setReturnQuals(FunctionDecl *fd, uint32_t type) {
  // Skip void functions.
  if ( fd->getResultType().getTypePtr()->isVoidType() )
    return;
  
  // Add qualifiers to return type.
  QualType newRetType = withQuals(fd->getResultType(), type);

  return rebuildFuncType(fd, newRetType);
}


// Interface to visitor.

// Attempt, to the extent possible, to copy custom qualifiers from one type
// tree to another.
QualType NodeTyper::copyQuals(QualType fromType,
                              QualType toType) {
  ASTContext &ctx = controller->ci.getASTContext();
  
  // Copy fromType's customQuals to toType's customQuals.
  Qualifiers newQuals = toType.getQualifiers();
  newQuals.setCustomQuals(fromType.getQualifiers().getCustomQuals());
  QualType out = replaceQuals(toType, newQuals);

  // Possibly recurse into referent type.
  if (ctx.UnwrapSimilarPointerTypes(fromType, toType)) {
    QualType innerType = copyQuals(fromType, toType);

    // Rewrap innerType into out.
    if (out->isPointerType()) {
      out = ctx.getQualifiedType(ctx.getPointerType(innerType),
                                 out.getQualifiers());
    } else {
      errs() << "UNIMPLEMENTED QUAL COPY\n";
    }
  } else if (fromType->isArrayType() && toType->isPointerType()) {
    // Special case for array-to-pointer lowering.
    QualType innerType = copyQuals(
        fromType->getAsArrayTypeUnsafe()->getElementType(),
        toType->getPointeeType()
    );

    // Copy qualifiers from the array type to the pointer type, but *exclude*
    // custom qualifiers. This works around a frustrating limitation in Clang
    // that the element type qualifiers are always shared by the array (via
    // the array's canonical QualType). Calling
    // ASTContext::getConstantArrayType() on a qualified element type, for
    // example, will give you an array type with the same qualifiers (not as
    // local qualifiers but as qualifiers on the canonical type). There's no
    // easy way to get an array type without this behavior. So we simply use
    // the local qualifiers here (i.e., excluding the qualifiers on the
    // canonical type). This might break eventually in the face of typedefs.
    Qualifiers newQuals = fromType.getLocalQualifiers();
    out = ctx.getQualifiedType(ctx.getPointerType(innerType), newQuals);
  } else if (fromType->isArrayType() && toType->isArrayType()) {
    // Ensure we have a real array type (not a typedef, for example).
    toType = toType.getDesugaredType(ctx);

    // Recurse to inner type.
    QualType innerType = copyQuals(
        fromType->getAsArrayTypeUnsafe()->getElementType(),
        toType->getAsArrayTypeUnsafe()->getElementType()
    );

    // Reconstruct the correct kind of ArrayType.
    // Mostly copy 'n pasted from: ASTContext::getUnqualifiedArrayType.
    if (const ConstantArrayType *CAT = dyn_cast<ConstantArrayType>(toType)) {
      out = ctx.getConstantArrayType(innerType, CAT->getSize(),
                                     CAT->getSizeModifier(), 0);
    } else if (const IncompleteArrayType *IAT = dyn_cast<IncompleteArrayType>(toType)) {
      out = ctx.getIncompleteArrayType(innerType, IAT->getSizeModifier(), 0);
    } else if (const VariableArrayType *VAT = dyn_cast<VariableArrayType>(toType)) {
      out = ctx.getVariableArrayType(innerType,
                                     VAT->getSizeExpr(),
                                     VAT->getSizeModifier(),
                                     VAT->getIndexTypeCVRQualifiers(),
                                     VAT->getBracketsRange());
    } else {
      const DependentSizedArrayType *DSAT = cast<DependentSizedArrayType>(toType);
      out = ctx.getDependentSizedArrayType(innerType, DSAT->getSizeExpr(),
                                           DSAT->getSizeModifier(), 0,
                                           SourceRange());
    }
  }

  return out;
}

// Return the referent type of a pointer or array type. If this is not a
// deference-able type, then returns a null type.
QualType NodeTyper::innerType(QualType type) {
  if (type->isAnyPointerType()) {
    return type->getPointeeType();
  } else if (type->isArrayType()) {
    return controller->ci.getASTContext().getArrayDecayedType(
      type
    )->getPointeeType();
  } else {
    return QualType();
  }
}

// Return a QualType whose qualifiers are removed and replaced with quals.
QualType NodeTyper::replaceQuals(QualType type,
                                 Qualifiers quals) {
  return controller->ci.getASTContext().getQualifiedType(
      type.getTypePtr(), quals
  );
}

void NodeTyper::process(Expr *expr) {
  // For some expression types, we recalculate the type here. This lets us
  // propagate types modified earlier in the process to other points in the
  // AST. This would not be necessary if we ran before ordinary type checking,
  // but that's not possible in Clang since type checking occurs simultaneously
  // with parsing.
  if (expr) {
    QualType newType = expr->getType();

    switch ( expr->getStmtClass() ) {

    // Symbols. Get the type of the declaration.
    case Stmt::DeclRefExprClass: {
      DeclRefExpr* tex = cast<DeclRefExpr>(expr);
      newType = tex->getDecl()->getType().getNonReferenceType();
      break;
    }
    case Stmt::MemberExprClass: {
      MemberExpr* mex = cast<MemberExpr>(expr);
      newType = mex->getMemberDecl()->getType().getNonReferenceType();
      break;
    }

    // Unary operators. For address-of and dereference, use inner expression
    // type. Otherwise, propagate the type of the subexpression.
    case Stmt::UnaryOperatorClass: {
      UnaryOperator *uop = cast<UnaryOperator>(expr);
      QualType seType = uop->getSubExpr()->getType();
      if (uop->getOpcode() == UO_AddrOf) {
        newType = controller->ci.getASTContext().getPointerType(seType);
      } else if (uop->getOpcode() == UO_Deref) {
        newType = innerType(seType);
      } else {
        // For unary pointer arithmetic, etc., it is necessary to propagate
        // the qualifier.
        newType = copyQuals(seType, newType);
      }
      break;
    }

    // Array subscript. Also use inner type.
    case Stmt::ArraySubscriptExprClass: {
      ArraySubscriptExpr *aex =
          cast<ArraySubscriptExpr>(expr);
      newType = innerType(aex->getLHS()->getType());
      break;
    }

    // Propagate types through casts. Implicit casts include lvalue-to-rvalue
    // casts, float-to-double conversions, etc. Lots of asserts will fail if we
    // don't do this (and unsoundness will occur). We also propagate through
    // explicit casts because it's impossible to annotate casts with the
    // current attribute syntax.
    case Stmt::ImplicitCastExprClass:
    case Stmt::CStyleCastExprClass:
    case Stmt::CXXFunctionalCastExprClass:
    case Stmt::CXXConstCastExprClass:
    case Stmt::CXXDynamicCastExprClass:
    case Stmt::CXXReinterpretCastExprClass:
    case Stmt::CXXStaticCastExprClass:
    {
      CastExpr *cex = cast<CastExpr>(expr);
      newType = copyQuals(cex->getSubExpr()->getType(), newType);
      break;
    }

    // Function calls. Set the return type.
    case Stmt::CallExprClass: {
      CallExpr* call = cast<CallExpr>(expr);
      FunctionDecl *callee = call->getDirectCallee();
      if (callee) {
        newType = callee->getCallResultType();
        // Since getCallResultType() (via QualType::getNonLValueExprType())
        // removes qualifiers, we restore them now.
        newType = copyQuals(callee->getResultType(), newType);
      }
      break;
    }

    // Pointer arithmetic and comma expressions.
    case Stmt::BinaryOperatorClass: {
      BinaryOperator *bop = cast<BinaryOperator>(expr);
      BinaryOperatorKind op = bop->getOpcode();
      QualType ltype = bop->getLHS()->getType();
      QualType rtype = bop->getRHS()->getType();

      if (op == BO_Add || op == BO_Sub) {
        // If either side is a pointer, then this is pointer arithmetic.
        // Propagate the qualifiers from that pointer type to the result
        // pointer type.
        if (ltype->isPointerType()) {
          newType = copyQuals(ltype, newType);
        } else if (rtype->isPointerType()) {
          newType = copyQuals(rtype, newType);
        }

      } else if (op == BO_Comma) {
        // Take the type of the RHS.
        newType = rtype;

      }
      break;
    }

    // Parentheses.
    case Stmt::ParenExprClass: {
      ParenExpr* paren = cast<ParenExpr>(expr);
      newType = copyQuals(paren->getSubExpr()->getType(), newType);
    }

    default:
      // Do nothing.
      break;
    }

    if (newType != expr->getType() && !newType.isNull())
      expr->setType(newType);
  }

  uint32_t type = typeForExpr(expr);
  if (type != CL_LEAVE_UNCHANGED)
    setQuals(expr, type);
}
void NodeTyper::process(Decl *decl) {
  uint32_t type = typeForDecl(decl);
  if (type != CL_LEAVE_UNCHANGED)
    setQuals(decl, type);
}
