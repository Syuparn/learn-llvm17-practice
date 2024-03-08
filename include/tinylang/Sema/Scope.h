#ifndef TINYLANG_SEMA_SCOPE_H
#define TINYLANG_SEMA_SCOPE_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

namespace tinylang {

class Decl;

class Scope {
  Scope *Parent;
  llvm::StringMap<Decl *> Symbols;

public:
  Scope(Scope *Parent = nullptr) : Parent(Parent) {}

  bool insert(Decl *Declaration);
  Decl *lookup(llvm::StringRef Name);

  Scope *getParent() { return Parent; }
};
} // namespace tinylang
#endif
