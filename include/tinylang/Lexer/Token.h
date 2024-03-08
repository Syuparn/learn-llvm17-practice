#ifndef TINYLANG_LEXER_TOKEN_H
#define TINYLANG_LEXER_TOKEN_H

#include "tinylang/Basic/TokenKinds.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"
#include <cassert>

namespace tinylang {

class Token {
  friend class Lexer;

  const char *Ptr;
  size_t Length;
  tok::TokenKind Kind;

public:
  tok::TokenKind getKind() const { return Kind; }

  void setKind(tok::TokenKind K) { Kind = K; }

  size_t getLength() const { return Length; }

  // source position
  llvm::SMLoc getLocation() const {
    return llvm::SMLoc::getFromPointer(Ptr);
  }

  llvm::StringRef getIdentifier() {
    assert(is(tok::identifier) &&
           "Cannot get identifier of non-identifier");
    return llvm::StringRef(Ptr, Length);
  }

  llvm::StringRef getLiteralData() {
    assert(isOneOf(tok::integer_literal) &&
           "Cannot get literal data of non-literal");
    return llvm::StringRef(Ptr, Length);
  }

  bool is(tok::TokenKind K) const { return Kind == K; }

  template <typename... Tokens>
  bool isOneOf(Tokens &&...Toks) const {
    return (... || is(Toks));
  }
};
} // namespace tinylang

#endif
