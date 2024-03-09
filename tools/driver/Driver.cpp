#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Basic/Version.h"
#include "tinylang/CodeGen/CodeGenerator.h"
#include "tinylang/Parser/Parser.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

using namespace tinylang;

namespace {
static llvm::codegen::RegisterCodeGenFlags CGF;

static llvm::cl::opt<std::string>
    InputFile(llvm::cl::Positional,
              llvm::cl::desc("<input-files>"),
              llvm::cl::init("-"));

static llvm::cl::opt<std::string>
    OutputFilename("o", llvm::cl::desc("Output filename"),
                   llvm::cl::value_desc("filename"));

static llvm::cl::opt<std::string> MTriple(
    "mtriple",
    llvm::cl::desc("Override target triple for module"));

static llvm::cl::opt<bool> EmitLLVM(
    "emit-llvm",
    llvm::cl::desc("Emit IR code instead of assembler"),
    llvm::cl::init(false));

static const char *Head = "tinylang - Tinylang compiler";

void printVersion(llvm::raw_ostream &OS) {
  OS << Head << " " << getTinylangVersion() << "\n";
  OS << "  Default target: "
     << llvm::sys::getDefaultTargetTriple() << "\n";
  std::string CPU(llvm::sys::getHostCPUName());
  OS << "  Host CPU: " << CPU << "\n";
  OS << "\n";
  OS.flush();
  llvm::TargetRegistry::printRegisteredTargetsForVersion(
      OS);
  exit(EXIT_SUCCESS);
}

llvm::TargetMachine *
createTargetMachine(const char *Argv0) {
  llvm::Triple Triple = llvm::Triple(
      !MTriple.empty()
          ? llvm::Triple::normalize(MTriple)
          : llvm::sys::getDefaultTargetTriple());

  llvm::TargetOptions TargetOptions =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(
          Triple);
  std::string CPUStr = llvm::codegen::getCPUStr();
  std::string FeatureStr = llvm::codegen::getFeaturesStr();

  std::string Error;
  const llvm::Target *Target =
      llvm::TargetRegistry::lookupTarget(
          llvm::codegen::getMArch(), Triple, Error);

  if (!Target) {
    llvm::WithColor::error(llvm::errs(), Argv0) << Error;
    return nullptr;
  }

  llvm::TargetMachine *TM = Target->createTargetMachine(
      Triple.getTriple(), CPUStr, FeatureStr, TargetOptions,
      std::optional<llvm::Reloc::Model>(
          llvm::codegen::getRelocModel()));
  return TM;
}

bool emit(llvm::StringRef Argv0, llvm::Module *M,
          llvm::TargetMachine *TM,
          llvm::StringRef InputFilename) {
  llvm::CodeGenFileType FileType =
      llvm::codegen::getFileType();
  if (OutputFilename.empty()) {
    if (InputFilename == "-") {
      OutputFilename = "-";
    } else {
      if (InputFilename.endswith(".mod"))
        OutputFilename = InputFilename.drop_back(4).str();
      else
        OutputFilename = InputFilename.str();
      switch (FileType) {
      case llvm::CGFT_AssemblyFile:
        OutputFilename.append(EmitLLVM ? ".ll" : ".s");
        break;
      case llvm::CGFT_ObjectFile:
        OutputFilename.append(".o");
        break;
      case llvm::CGFT_Null:
        OutputFilename.append(".null");
        break;
      }
    }
  }

  // Open the file.
  std::error_code EC;
  llvm::sys::fs::OpenFlags OpenFlags =
      llvm::sys::fs::OF_None;
  if (FileType == llvm::CGFT_AssemblyFile)
    OpenFlags |= llvm::sys::fs::OF_TextWithCRLF;
  auto Out = std::make_unique<llvm::ToolOutputFile>(
      OutputFilename, EC, OpenFlags);
  if (EC) {
    llvm::WithColor::error(llvm::errs(), Argv0)
        << EC.message() << '\n';
    return false;
  }

  llvm::legacy::PassManager PM;
  if (FileType == llvm::CGFT_AssemblyFile && EmitLLVM) {
    PM.add(createPrintModulePass(Out->os()));
  } else {
    if (TM->addPassesToEmitFile(PM, Out->os(), nullptr,
                                FileType)) {
      llvm::WithColor::error(llvm::errs(), Argv0)
          << "No support for file type\n";
      return false;
    }
  }
  PM.run(*M);
  Out->keep();
  return true;
}
} // namespace

int main(int Argc, const char **Argv) {
  llvm::InitLLVM X(Argc, Argv);

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  llvm::cl::SetVersionPrinter(&printVersion);
  llvm::cl::ParseCommandLineOptions(Argc, Argv, Head);

  if (llvm::codegen::getMCPU() == "help" ||
      std::any_of(llvm::codegen::getMAttrs().begin(),
                  llvm::codegen::getMAttrs().end(),
                  [](const std::string &a) {
                    return a == "help";
                  })) {
    auto Triple = llvm::Triple(LLVM_DEFAULT_TARGET_TRIPLE);
    std::string ErrMsg;
    if (auto *Target = llvm::TargetRegistry::lookupTarget(
            Triple.getTriple(), ErrMsg)) {
      llvm::errs() << "Targeting " << Target->getName()
                   << ". ";
      // This prints the available CPUs and features of the
      // target to stderr...
      Target->createMCSubtargetInfo(
          Triple.getTriple(), llvm::codegen::getCPUStr(),
          llvm::codegen::getFeaturesStr());
    } else {
      llvm::errs() << ErrMsg << "\n";
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  llvm::TargetMachine *TM = createTargetMachine(Argv[0]);
  if (!TM)
    exit(EXIT_FAILURE);

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
      FileOrErr = llvm::MemoryBuffer::getFile(InputFile);
  if (std::error_code BufferError = FileOrErr.getError()) {
    llvm::WithColor::error(llvm::errs(), Argv[0])
        << "Error reading " << InputFile << ": "
        << BufferError.message() << "\n";
  }

  llvm::SourceMgr SrcMgr;
  DiagnosticsEngine Diags(SrcMgr);

  // Tell SrcMgr about this buffer, which is what the
  // parser will pick up.
  SrcMgr.AddNewSourceBuffer(std::move(*FileOrErr),
                            llvm::SMLoc());

  auto TheLexer = Lexer(SrcMgr, Diags);
  auto TheSema = Sema(Diags);
  auto TheParser = Parser(TheLexer, TheSema);
  auto *Mod = TheParser.parse();
  if (Mod && !Diags.numErrors()) {
    llvm::LLVMContext Ctx;
    if (CodeGenerator *CG =
            CodeGenerator::create(Ctx, TM)) {
      std::unique_ptr<llvm::Module> M =
          CG->run(Mod, InputFile);
      if (!emit(Argv[0], M.get(), TM, InputFile)) {
        llvm::WithColor::error(llvm::errs(), Argv[0])
            << "Error writing output\n";
      }
      delete CG;
    }
  }
}
