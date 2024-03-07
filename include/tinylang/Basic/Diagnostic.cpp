#include "./Diagnostic.h"
#include <llvm/Support/SourceMgr.h>

using namespace tinylang;
namespace {
    const char *DiagnosticText[] = {
        #define DIAG(ID, Level, Msg) Msg,
        #include "./Diagnostic.def"
    };

    llvm::SourceMgr::DiagKind DiagnosticKind[] = {
        #define DIAG(ID, Level, Msg) llvm::SourceMgr::DK_##Level,
        #include "./Diagnostic.def"
    };
}

const char *DiagnosticsEngine::getDiagnosticText(unsigned DiagID) {
    return DiagnosticText[DiagID];
}

llvm::SourceMgr::DiagKind DiagnosticsEngine::getDiagnosticKind(unsigned DiagID) {
    return DiagnosticKind[DiagID];
}
