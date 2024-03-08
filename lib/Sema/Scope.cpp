#include "tinylang/Sema/Scope.h"
#include "tinylang/AST/AST.h"

using namespace tinylang;

bool Scope::insert(Decl *Declaration) {
  return Symbols
      .insert(std::pair<llvm::StringRef, Decl *>(
          Declaration->getName(), Declaration))
      .second;
}

Decl *Scope::lookup(llvm::StringRef Name) {
  Scope *S = this;
  while (S) {
    llvm::StringMap<Decl *>::const_iterator I =
        S->Symbols.find(Name);
    if (I != S->Symbols.end())
      return I->second;
    S = S->getParent();
  }
  return nullptr;
}
