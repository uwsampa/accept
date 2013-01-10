#include "TyperConsumer.h"

void TyperConsumer::reportError(clang::Stmt* stmt,
                                llvm::StringRef message) {
  clang::DiagnosticsEngine &deng = ci.getDiagnostics();
  unsigned did = deng.getCustomDiagID(clang::DiagnosticsEngine::Error, message);
  clang::DiagnosticBuilder db = deng.Report(stmt->getLocStart(), did);
  db.AddSourceRange(clang::CharSourceRange(stmt->getSourceRange(), false));
}

void TyperConsumer::HandleTranslationUnit(
    clang::ASTContext &Ctx
) {
  llvm::errs() << "consumer finished\n";
}

