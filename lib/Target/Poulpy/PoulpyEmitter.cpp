#include "lib/Target/Poulpy/PoulpyEmitter.h"

#include "lib/Dialect/Poulpy/IR/PoulpyDialect.h"
#include "lib/Dialect/Poulpy/IR/PoulpyOps.h"
#include "lib/Dialect/Poulpy/IR/PoulpyTypes.h"
#include "lib/Target/Poulpy/PoulpyTemplates.h"
#include "lib/Utils/TargetUtils.h"
#include "llvm/include/llvm/ADT/STLExtras.h"            // from @llvm-project
#include "llvm/include/llvm/ADT/TypeSwitch.h"           // from @llvm-project
#include "llvm/include/llvm/Support/Debug.h"            // from @llvm-project
#include "llvm/include/llvm/Support/FormatVariadic.h"   // from @llvm-project
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"  // from @llvm-project
#include "mlir/include/mlir/Tools/mlir-translate/Translation.h"  // from @llvm-project
#define DEBUG_TYPE "poulpy-emitter"

namespace mlir {
namespace heir {
namespace poulpy {

namespace {
// The emitted code is generic over the backend: a function whose signature
// carries a poulpy value takes `<BE: PoulpyBackend>` and requires
// `Module<BE>: PoulpyModule<BE>`, and calls infer `BE` from their arguments.
bool isGenericOverBackend(func::FuncOp funcOp) {
  auto isPoulpyType = [](Type type) {
    if (auto memRefType = dyn_cast<MemRefType>(type))
      type = memRefType.getElementType();
    return isa<PoulpyDialect>(&type.getDialect());
  };
  return llvm::any_of(funcOp.getArgumentTypes(), isPoulpyType) ||
         llvm::any_of(funcOp.getResultTypes(), isPoulpyType);
}

std::string valueOrClonedValue(Value value,
                               SelectVariableNames* variableNames) {
  auto expression = variableNames->getNameForValue(value);
  if (isa<BlockArgument>(value)) {
    // f64 arguments are borrowed slices, the others borrowed poulpy values.
    auto memRefType = dyn_cast<MemRefType>(value.getType());
    bool isSlice = memRefType && memRefType.getElementType().isF64();
    expression += isSlice ? ".to_vec()" : ".clone()";
  }
  return expression;
}

std::string ref(Value value, SelectVariableNames* variableNames) {
  auto expression = variableNames->getNameForValue(value);
  return isa<BlockArgument>(value) ? "&*" + expression : "&" + expression;
}

std::string refMut(Value value, SelectVariableNames* variableNames) {
  auto expression = variableNames->getNameForValue(value);
  return isa<BlockArgument>(value) ? "&mut *" + expression
                                   : "&mut " + expression;
}

// A rank-1 gadget key layout (tensor or automorphism key) for ciphertexts of
// width `k`, with `dsize` limbs per digit and `kAux` auxiliary bits.
std::string keyLayout(StringRef layoutType, StringRef moduleName,
                      int64_t base2k, int64_t k, int64_t kAux, int64_t dsize) {
  return llvm::formatv(
             "{0} {{ n: {1}.n().into(), base2k: Base2K({2}u32), dnum: "
             "GGLWELayout::dnum_for_input(Base2K({2}u32), "
             "TorusPrecision({3}u32), "
             "Dsize({5}u32)), k_aux: TorusPrecision({4}u32), rank: Rank(1), "
             "dsize: "
             "Dsize({5}u32) }",
             layoutType, moduleName, base2k, k, kAux, dsize)
      .str();
}
}  // namespace

void registerToPoulpyTranslation() {
  TranslateFromMLIRRegistration reg(
      "emit-poulpy", "translate the poulpy dialect to Rust code for poulpy",
      [](Operation* op, llvm::raw_ostream& output) {
        return translateToPoulpy(op, output);
      },
      [](DialectRegistry& registry) {
        registry.insert<func::FuncDialect, memref::MemRefDialect,
                        poulpy::PoulpyDialect>();
      });
}

LogicalResult translateToPoulpy(Operation* op, llvm::raw_ostream& os) {
  SelectVariableNames variableNames(op);
  PoulpyEmitter emitter(os, &variableNames);
  LogicalResult result = emitter.translate(*op);
  return result;
}

LogicalResult PoulpyEmitter::translateBlock(Block& block) {
  for (Operation& op : block.getOperations()) {
    if (failed(translate(op))) {
      return failure();
    }
  }
  return success();
}

LogicalResult PoulpyEmitter::translate(Operation& op) {
  LogicalResult status =
      llvm::TypeSwitch<Operation&, LogicalResult>(op)
          .Case<ModuleOp, func::FuncOp, func::ReturnOp, func::CallOp, AddOp,
                AddAssignOp, SubOp, SubAssignOp, MulOp, MulAssignOp, AddPtVecOp,
                AddPtVecAssignOp, SubPtVecOp, SubPtVecAssignOp, MulPtVecOp,
                MulPtVecAssignOp, NegOp, NegAssignOp, RotateOp, RotateAssignOp,
                MulPow2Op, MulPow2AssignOp, DivPow2Op, DivPow2AssignOp,
                CompactLimbsOp, EncodeOp, DecodeOp, EncryptOp, DecryptOp,
                ModuleCreateOp, ScratchAllocOp, ScratchAllocForAllOpsOp,
                SecretGenerateOp, SecretKeyPrepareOp, TensorKeyGenerateOp,
                AutomorphismKeysGenerateOp, memref::AllocOp, memref::CopyOp>(
              [&](auto op) { return printOperation(op); })
          .Default([&](Operation& op) {
            return op.emitOpError("unable to find printer for op");
          });

  if (failed(status)) {
    op.emitOpError(llvm::formatv("Failed to translate op {0}", op.getName()));
    return failure();
  }

  return success();
}

void PoulpyEmitter::computeMutatedValues(func::FuncOp funcOp) {
  mutatedValues.clear();

  funcOp.walk([&](Operation* op) {
    llvm::TypeSwitch<Operation&, void>(*op)
        .Case<AddOp, AddAssignOp, SubOp, SubAssignOp, MulOp, MulAssignOp,
              AddPtVecOp, AddPtVecAssignOp, SubPtVecOp, SubPtVecAssignOp,
              MulPtVecOp, MulPtVecAssignOp, NegOp, NegAssignOp, RotateOp,
              RotateAssignOp, MulPow2Op, MulPow2AssignOp, DivPow2Op,
              DivPow2AssignOp, CompactLimbsOp>(
            [&](auto op) { mutatedValues.insert(op.getDst()); })
        .Case<DecodeOp>([&](DecodeOp op) {
          mutatedValues.insert(op.getReal());
          mutatedValues.insert(op.getImag());
        })
        .Case<EncryptOp>(
            [&](EncryptOp op) { mutatedValues.insert(op.getCiphertext()); })
        .Case<DecryptOp>(
            [&](DecryptOp op) { mutatedValues.insert(op.getPlaintext()); })
        .Default([&](Operation& op) {});
  });
}

void PoulpyEmitter::materializeIfPending(Value dst, Value module,
                                         Value layoutSource,
                                         bool useSemanticWidth) {
  if (pendingAllocs.erase(dst)) {
    auto layoutName = variableNames->getNameForValue(layoutSource);
    os << "let mut " << variableNames->getNameForValue(dst) << " = "
       << variableNames->getNameForValue(module) << ".ckks_ciphertext_alloc("
       << layoutName << ".base2k(), " << layoutName
       << (useSemanticWidth ? ".k()" : ".max_k()") << ");\n";
  }
}

// The _assign ops read-before-write dst, so it must already be materialized.
// encode, encrypt and decrypt allocate their result themselves, so it must NOT
// be materialized yet.
LogicalResult PoulpyEmitter::checkPendingState(Value dst, Operation* op,
                                               bool shouldBePending) {
  if (pendingAllocs.contains(dst) == shouldBePending) return success();

  if (shouldBePending) {
    return op->emitError(
        "result must come from an unmaterialized memref.alloc");
  }
  InFlightDiagnostic diag =
      op->emitError("dst has not been initialized before this use");
  diag.attachNote(dst.getLoc()) << "allocated here but never written to first";
  return diag;
}

LogicalResult PoulpyEmitter::printOperation(ModuleOp moduleOp) {
  os << kModulePrelude << kTypeAliases << "\n";
  for (Operation& op : moduleOp) {
    if (failed(translate(op))) {
      return failure();
    }
  }
  return success();
}

LogicalResult PoulpyEmitter::printOperation(func::FuncOp funcOp) {
  computeMutatedValues(funcOp);
  sourceCounter = 0;
  bool isGeneric = isGenericOverBackend(funcOp);
  os << "pub fn " << funcOp.getName();
  if (isGeneric) os << "<BE: PoulpyBackend>";
  os << "(\n";
  os.indent();
  for (Value arg : funcOp.getArguments()) {
    auto argName = variableNames->getNameForValue(arg);
    os << argName << ": ";
    bool isMutated = mutatedValues.contains(arg);
    if (failed(emitType(arg.getType(), /*isArg=*/true, isMutated))) {
      return funcOp.emitOpError()
             << "Failed to emit poulpy type " << arg.getType();
    }
    os << ",\n";
  }
  os.unindent();
  os << ") -> Result<";

  auto numResults = funcOp.getNumResults();
  if (numResults == 0) {
    os << "()";
  } else if (numResults == 1) {
    Type result = funcOp.getResultTypes()[0];
    if (failed(emitType(result, /*isArg=*/false, /*isMutated=*/false))) {
      return funcOp.emitOpError() << "Failed to emit poulpy type " << result;
    }
  } else {
    auto types = commaSeparatedTypes(funcOp.getResultTypes(), [&](Type t) {
      return convertType(t, /*isArg=*/false, /*isMutated=*/false);
    });
    if (failed(types))
      return funcOp.emitOpError() << "Failed to emit poulpy result types";
    os << "(" << *types << ")";
  }

  os << ">";
  if (isGeneric) os << " where Module<BE>: PoulpyModule<BE>";
  os << " {\n";
  os.indent();
  for (Block& block : funcOp.getBlocks()) {
    if (failed(translateBlock(block))) {
      return funcOp.emitOpError()
             << "Failed to translate block of func " << funcOp.getName();
    }
  }

  os.unindent();
  os << "}\n\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(func::ReturnOp op) {
  if (op.getNumOperands() == 0) {
    os << "Ok(())\n";
  } else if (op.getNumOperands() == 1) {
    auto returnOperand = op.getOperands()[0];
    auto expression = valueOrClonedValue(returnOperand, variableNames);
    os << "Ok(" << expression << ")\n";
  } else {
    auto values = commaSeparatedValues(op.getOperands(), [&](Value v) {
      return valueOrClonedValue(v, variableNames);
    });
    os << "Ok((" << values << "))\n";
  }
  return success();
}

LogicalResult PoulpyEmitter::printOperation(func::CallOp op) {
  auto enclosingModuleOp = op->getParentOfType<ModuleOp>();
  auto calleeOp = enclosingModuleOp.lookupSymbol<func::FuncOp>(op.getCallee());

  computeMutatedValues(calleeOp);

  SmallVector<std::string> args;
  for (auto [operand, calleeArg] :
       llvm::zip(op.getOperands(), calleeOp.getArguments())) {
    if (isa<ScratchType>(calleeArg.getType()) ||
        mutatedValues.contains(calleeArg)) {
      args.push_back(refMut(operand, variableNames));
    } else {
      args.push_back(ref(operand, variableNames));
    }
  }

  auto argList = llvm::join(args, ", ");

  auto numResults = op.getNumResults();

  if (numResults == 0) {
    os << calleeOp.getName() << "(" << argList << ")?;\n";
  } else if (numResults == 1) {
    os << "let " << variableNames->getNameForValue(op.getResult(0)) << " = "
       << calleeOp.getName() << "(" << argList << ")?;\n";
  } else {
    auto names = commaSeparatedValues(op.getResults(), [&](Value v) {
      return variableNames->getNameForValue(v);
    });
    os << "let (" << names << ") = " << calleeOp.getName() << "(" << argList
       << ")?;\n";
  }

  return success();
}

void PoulpyEmitter::emitCall(Value module, StringRef rustFn, Value dst,
                             ArrayRef<std::string> args, Value scratch) {
  os << variableNames->getNameForValue(module) << "." << rustFn << "("
     << refMut(dst, variableNames);
  for (const std::string& arg : args) os << ", " << arg;
  if (scratch)
    os << ", &mut " << variableNames->getNameForValue(scratch) << ".borrow()";
  os << ")?;\n";
}

template <typename OpTy>
LogicalResult PoulpyEmitter::emitBinaryOp(OpTy op, StringRef rustFn,
                                          ArrayRef<std::string> extraArgs) {
  materializeIfPending(op.getDst(), op.getModule(), op.getA(),
                       /*useSemanticWidth=*/false);

  SmallVector<std::string> args = {ref(op.getA(), variableNames),
                                   ref(op.getB(), variableNames)};
  args.append(extraArgs.begin(), extraArgs.end());
  emitCall(op.getModule(), rustFn, op.getDst(), args, op.getScratch());
  return success();
}

template <typename OpTy>
LogicalResult PoulpyEmitter::emitBinaryAssignOp(
    OpTy op, StringRef rustFn, ArrayRef<std::string> extraArgs) {
  if (failed(checkPendingState(op.getDst(), op, /*shouldBePending=*/false)))
    return failure();

  SmallVector<std::string> args = {ref(op.getA(), variableNames)};
  args.append(extraArgs.begin(), extraArgs.end());
  emitCall(op.getModule(), rustFn, op.getDst(), args, op.getScratch());
  return success();
}

LogicalResult PoulpyEmitter::printOperation(AddOp addOp) {
  return emitBinaryOp(addOp, "ckks_add_into");
}

LogicalResult PoulpyEmitter::printOperation(AddAssignOp addAssignOp) {
  return emitBinaryAssignOp(addAssignOp, "ckks_add_assign");
}

LogicalResult PoulpyEmitter::printOperation(SubOp subOp) {
  return emitBinaryOp(subOp, "ckks_sub_into");
}

LogicalResult PoulpyEmitter::printOperation(SubAssignOp subAssignOp) {
  return emitBinaryAssignOp(subAssignOp, "ckks_sub_assign");
}

LogicalResult PoulpyEmitter::printOperation(MulOp mulOp) {
  return emitBinaryOp(mulOp, "ckks_mul_into",
                      {ref(mulOp.getTsk(), variableNames)});
}

LogicalResult PoulpyEmitter::printOperation(MulAssignOp mulAssignOp) {
  return emitBinaryAssignOp(mulAssignOp, "ckks_mul_assign",
                            {ref(mulAssignOp.getTsk(), variableNames)});
}

LogicalResult PoulpyEmitter::printOperation(AddPtVecOp op) {
  return emitBinaryOp(op, "ckks_add_pt_vec_into");
}

LogicalResult PoulpyEmitter::printOperation(AddPtVecAssignOp op) {
  return emitBinaryAssignOp(op, "ckks_add_pt_vec_assign");
}

LogicalResult PoulpyEmitter::printOperation(SubPtVecOp op) {
  return emitBinaryOp(op, "ckks_sub_pt_vec_into");
}

LogicalResult PoulpyEmitter::printOperation(SubPtVecAssignOp op) {
  return emitBinaryAssignOp(op, "ckks_sub_pt_vec_assign");
}

LogicalResult PoulpyEmitter::printOperation(MulPtVecOp op) {
  return emitBinaryOp(op, "ckks_mul_pt_vec_into");
}

LogicalResult PoulpyEmitter::printOperation(MulPtVecAssignOp op) {
  return emitBinaryAssignOp(op, "ckks_mul_pt_vec_assign");
}

LogicalResult PoulpyEmitter::printOperation(NegOp negOp) {
  auto dst = negOp.getDst();
  auto src = negOp.getSrc();

  materializeIfPending(dst, negOp.getModule(), src,
                       /*useSemanticWidth=*/false);

  emitCall(negOp.getModule(), "ckks_neg_into", dst, {ref(src, variableNames)},
           negOp.getScratch());

  return success();
}

// NOTE: like ckks_div_pow2_assign, ckks_neg_assign takes no scratch argument.
LogicalResult PoulpyEmitter::printOperation(NegAssignOp negAssignOp) {
  auto dst = negAssignOp.getDst();

  if (failed(checkPendingState(dst, negAssignOp, /*shouldBePending=*/false)))
    return failure();

  emitCall(negAssignOp.getModule(), "ckks_neg_assign", dst, {});

  return success();
}

LogicalResult PoulpyEmitter::printOperation(RotateOp rotateOp) {
  auto dst = rotateOp.getDst();
  auto src = rotateOp.getSrc();

  materializeIfPending(dst, rotateOp.getModule(), src,
                       /*useSemanticWidth=*/false);

  emitCall(rotateOp.getModule(), "ckks_rotate_into", dst,
           {ref(src, variableNames), std::to_string(rotateOp.getK()) + "i64",
            ref(rotateOp.getKeys(), variableNames)},
           rotateOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(RotateAssignOp rotateAssignOp) {
  auto dst = rotateAssignOp.getDst();

  if (failed(checkPendingState(dst, rotateAssignOp, /*shouldBePending=*/false)))
    return failure();

  emitCall(rotateAssignOp.getModule(), "ckks_rotate_assign", dst,
           {std::to_string(rotateAssignOp.getK()) + "i64",
            ref(rotateAssignOp.getKeys(), variableNames)},
           rotateAssignOp.getScratch());

  return success();
}

template <typename OpTy>
LogicalResult PoulpyEmitter::emitPow2Op(OpTy op, StringRef rustFn) {
  materializeIfPending(op.getDst(), op.getModule(), op.getSrc(),
                       /*useSemanticWidth=*/false);

  emitCall(
      op.getModule(), rustFn, op.getDst(),
      {ref(op.getSrc(), variableNames), std::to_string(op.getBits()) + "usize"},
      op.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(MulPow2Op mulPow2Op) {
  return emitPow2Op(mulPow2Op, "ckks_mul_pow2_into");
}

LogicalResult PoulpyEmitter::printOperation(MulPow2AssignOp mulPow2AssignOp) {
  auto dst = mulPow2AssignOp.getDst();

  if (failed(
          checkPendingState(dst, mulPow2AssignOp, /*shouldBePending=*/false)))
    return failure();

  emitCall(mulPow2AssignOp.getModule(), "ckks_mul_pow2_assign", dst,
           {std::to_string(mulPow2AssignOp.getBits()) + "usize"},
           mulPow2AssignOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(DivPow2Op divPow2Op) {
  return emitPow2Op(divPow2Op, "ckks_div_pow2_into");
}

// NOTE: ckks_div_pow2_assign takes no scratch argument, unlike every other
// _assign op. The dialect op still carries a scratch operand for structural
// uniformity, it's just not part of the Rust call.
LogicalResult PoulpyEmitter::printOperation(DivPow2AssignOp divPow2AssignOp) {
  auto dst = divPow2AssignOp.getDst();

  if (failed(
          checkPendingState(dst, divPow2AssignOp, /*shouldBePending=*/false)))
    return failure();

  emitCall(divPow2AssignOp.getModule(), "ckks_div_pow2_assign", dst,
           {std::to_string(divPow2AssignOp.getBits()) + "usize"});

  return success();
}

LogicalResult PoulpyEmitter::printOperation(CompactLimbsOp compactLimbsOp) {
  auto dst = compactLimbsOp.getDst();
  auto src = compactLimbsOp.getSrc();

  // Unlike every other _into op, dst is allocated with the semantic width
  // (.k()), not the allocated capacity (.max_k()).
  materializeIfPending(dst, compactLimbsOp.getModule(), src,
                       /*useSemanticWidth=*/true);

  emitCall(compactLimbsOp.getModule(), "ckks_copy", dst,
           {ref(src, variableNames)}, compactLimbsOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(EncodeOp encodeOp) {
  auto module = encodeOp.getModule();
  auto plaintext = encodeOp.getPlaintext();
  int64_t logDelta = encodeOp.getLogDelta();
  int64_t logBudget = encodeOp.getLogBudget();
  int64_t base2k = encodeOp.getBase2k();

  if (failed(checkPendingState(plaintext, encodeOp, /*shouldBePending=*/true)))
    return failure();
  pendingAllocs.erase(plaintext);

  auto ptName = variableNames->getNameForValue(plaintext);
  os << "let mut " << ptName << " = " << variableNames->getNameForValue(module)
     << ".ckks_pt_vec_alloc(Base2K(" << base2k << "u32), TorusPrecision("
     << (logDelta + logBudget) << "u32));\n";
  os << ptName << ".set_meta(CKKSMeta { log_delta: " << logDelta
     << "usize, log_sparsity: 0usize, slots: SlotsKind::Complex });\n";
  emitCall(module, "ckks_encode_reim_into", plaintext,
           {ref(encodeOp.getReal(), variableNames),
            ref(encodeOp.getImag(), variableNames)},
           encodeOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(DecodeOp decodeOp) {
  os << variableNames->getNameForValue(decodeOp.getModule())
     << ".ckks_decode_reim_into(" << ref(decodeOp.getPlaintext(), variableNames)
     << ", " << refMut(decodeOp.getReal(), variableNames) << ", "
     << refMut(decodeOp.getImag(), variableNames) << ", &mut "
     << variableNames->getNameForValue(decodeOp.getScratch())
     << ".borrow())?;\n";

  return success();
}

LogicalResult PoulpyEmitter::printOperation(EncryptOp encryptOp) {
  auto module = encryptOp.getModule();
  auto ciphertext = encryptOp.getCiphertext();
  auto secretKey = encryptOp.getSecretKey();
  uint64_t base2k = encryptOp.getBase2k();
  uint64_t ctK = encryptOp.getCtk();

  if (failed(
          checkPendingState(ciphertext, encryptOp, /*shouldBePending=*/true)))
    return failure();
  pendingAllocs.erase(ciphertext);

  os << "let mut " << variableNames->getNameForValue(ciphertext) << " = "
     << variableNames->getNameForValue(module)
     << ".ckks_ciphertext_alloc(Base2K(" << base2k << "u32), TorusPrecision("
     << ctK << "u32));\n";

  std::string sourceXe = emitSource();
  std::string sourceXa = emitSource();
  emitCall(
      module, "ckks_encrypt_sk", ciphertext,
      {ref(encryptOp.getPlaintext(), variableNames),
       ref(secretKey, variableNames), "&mut " + sourceXe, "&mut " + sourceXa},
      encryptOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(DecryptOp decryptOp) {
  auto module = decryptOp.getModule();
  auto plaintext = decryptOp.getPlaintext();
  auto ciphertext = decryptOp.getCiphertext();

  if (failed(checkPendingState(plaintext, decryptOp, /*shouldBePending=*/true)))
    return failure();
  pendingAllocs.erase(plaintext);

  os << "let mut " << variableNames->getNameForValue(plaintext) << " = "
     << variableNames->getNameForValue(module)
     << ".ckks_plaintext_alloc_from_infos(" << ref(ciphertext, variableNames)
     << ");\n";

  emitCall(module, "ckks_decrypt", plaintext,
           {ref(ciphertext, variableNames),
            ref(decryptOp.getSecretKey(), variableNames)},
           decryptOp.getScratch());

  return success();
}

LogicalResult PoulpyEmitter::printOperation(ModuleCreateOp moduleCreateOp) {
  os << "let " << variableNames->getNameForValue(moduleCreateOp.getModule())
     << " = Module::<BE>::new(" << moduleCreateOp.getN() << "u64);\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(ScratchAllocOp scratchAllocOp) {
  os << "let mut "
     << variableNames->getNameForValue(scratchAllocOp.getScratch())
     << " = ScratchOwned::<BE>::alloc(" << scratchAllocOp.getSize()
     << "usize);\n";
  return success();
}

// TODO(mmoro): These seeds are deterministic placeholders, not real
// randomness. Thread a real CSPRNG seed once a client-interface pass
// exists to supply one.
std::string PoulpyEmitter::emitSource() {
  std::string name = "source" + std::to_string(sourceCounter);
  os << "let mut " << name << " = Source::new([" << sourceCounter
     << "u8; 32]);\n";
  ++sourceCounter;
  return name;
}

LogicalResult PoulpyEmitter::printOperation(ScratchAllocForAllOpsOp op) {
  auto moduleName = variableNames->getNameForValue(op.getModule());
  auto scratchName = variableNames->getNameForValue(op.getScratch());
  int64_t base2k = op.getBase2k();
  int64_t k = op.getK();

  os << "let " << scratchName << "_tsk_layout = "
     << keyLayout("GLWETensorKeyLayout", moduleName, base2k, k, op.getKAux(),
                  op.getDsize())
     << ";\n";
  os << "let " << scratchName << "_atk_layout = "
     << keyLayout("GLWEAutomorphismKeyLayout", moduleName, base2k, k,
                  op.getKAux(), op.getDsize())
     << ";\n";
  os << "let " << scratchName
     << "_ct_layout = CKKSLayout { glwe_layout: GLWELayout { n: " << moduleName
     << ".n().into(), base2k: Base2K(" << base2k << "u32), k: TorusPrecision("
     << k << "u32), rank: Rank(1) }, meta: CKKSMeta { log_delta: "
     << op.getLogDelta()
     << "usize, log_sparsity: 0usize, slots: SlotsKind::Complex } };\n";
  os << "let mut " << scratchName << " = ScratchOwned::<BE>::alloc("
     << moduleName << ".ckks_all_ops_with_atk_tmp_bytes(&" << scratchName
     << "_ct_layout, &" << scratchName << "_tsk_layout, &" << scratchName
     << "_atk_layout, &" << scratchName << "_ct_layout));\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(SecretGenerateOp op) {
  auto moduleName = variableNames->getNameForValue(op.getModule());
  auto secretName = variableNames->getNameForValue(op.getSecret());
  std::string source = emitSource();
  os << "let mut " << secretName << " = " << moduleName
     << ".glwe_secret_alloc(Rank(1));\n";
  if (std::optional<uint64_t> hw = op.getHw()) {
    os << moduleName << ".glwe_secret_fill_ternary_hw(&mut " << secretName
       << ", " << *hw << "usize, &mut " << source << ");\n";
  } else {
    os << moduleName << ".glwe_secret_fill_ternary_prob(&mut " << secretName
       << ", 2.0 / 3.0, &mut " << source << ");\n";
  }
  return success();
}

LogicalResult PoulpyEmitter::printOperation(SecretKeyPrepareOp op) {
  auto moduleName = variableNames->getNameForValue(op.getModule());
  auto secretKeyName = variableNames->getNameForValue(op.getSecretKey());
  os << "let mut " << secretKeyName << " = " << moduleName
     << ".glwe_secret_prepared_alloc(Rank(1));\n";
  os << moduleName << ".glwe_secret_prepare(&mut " << secretKeyName << ", "
     << ref(op.getSecret(), variableNames) << ");\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(TensorKeyGenerateOp op) {
  auto moduleName = variableNames->getNameForValue(op.getModule());
  auto keyName = variableNames->getNameForValue(op.getTensorKey());
  auto scratchName = variableNames->getNameForValue(op.getScratch());
  os << "let " << keyName << "_layout = "
     << keyLayout("GLWETensorKeyLayout", moduleName, op.getBase2k(), op.getK(),
                  op.getKAux(), op.getDsize())
     << ";\n";
  std::string sourceXe = emitSource();
  std::string sourceXa = emitSource();
  os << "let mut " << keyName << "_raw = " << moduleName
     << ".glwe_tensor_key_alloc_from_infos(&" << keyName << "_layout);\n";
  os << moduleName << ".glwe_tensor_key_encrypt_sk(&mut " << keyName << "_raw, "
     << ref(op.getSecret(), variableNames) << ", &mut " << sourceXe << ", &mut "
     << sourceXa << ", &mut " << scratchName << ".borrow());\n";
  os << "let mut " << keyName << " = " << moduleName
     << ".alloc_tensor_key_prepared_from_infos(&" << keyName << "_layout);\n";
  os << moduleName << ".prepare_tensor_key(&mut " << keyName << ", &" << keyName
     << "_raw, &mut " << scratchName << ".borrow());\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(AutomorphismKeysGenerateOp op) {
  auto moduleName = variableNames->getNameForValue(op.getModule());
  auto keysName = variableNames->getNameForValue(op.getKeys());
  auto scratchName = variableNames->getNameForValue(op.getScratch());
  os << "let mut " << keysName << ": Akm<BE> = HashMap::new();\n";
  ArrayRef<int64_t> rotations = op.getRotations();
  if (rotations.empty()) return success();

  os << "let " << keysName << "_layout = "
     << keyLayout("GLWEAutomorphismKeyLayout", moduleName, op.getBase2k(),
                  op.getK(), op.getKAux(), op.getDsize())
     << ";\n";
  std::string sourceXe = emitSource();
  std::string sourceXa = emitSource();
  os << "for rotation in ["
     << llvm::join(
            llvm::map_range(
                rotations, [](int64_t r) { return std::to_string(r) + "i64"; }),
            ", ")
     << "] {\n";
  os.indent();
  os << "let p = " << moduleName << ".ckks_galois_element(rotation);\n";
  os << "let mut raw = " << moduleName
     << ".glwe_automorphism_key_alloc_from_infos(&" << keysName
     << "_layout);\n";
  os << moduleName << ".glwe_automorphism_key_encrypt_sk(&mut raw, p, "
     << ref(op.getSecret(), variableNames) << ", &mut " << sourceXe << ", &mut "
     << sourceXa << ", &mut " << scratchName << ".borrow());\n";
  os << "let mut prepared = " << moduleName
     << ".glwe_automorphism_key_prepared_alloc_from_infos(&" << keysName
     << "_layout);\n";
  os << moduleName << ".glwe_automorphism_key_prepare(&mut prepared, &raw, "
     << "&mut " << scratchName << ".borrow());\n";
  os << keysName << ".insert(p, prepared);\n";
  os.unindent();
  os << "}\n";
  return success();
}

LogicalResult PoulpyEmitter::printOperation(memref::AllocOp allocOp) {
  MemRefType resultType = allocOp.getType();
  if (resultType.getElementType().isF64()) {
    // Unlike ciphertext buffers, an f64 vector's size is already known
    // statically from the type so we materialize immediately
    if (resultType.getRank() != 1 || resultType.isDynamicDim(0)) {
      return allocOp.emitOpError(
          "unsupported memref shape for f64 alloc (expected static rank-1)");
    }
    os << "let mut " << variableNames->getNameForValue(allocOp.getResult())
       << " = vec![0f64; " << resultType.getDimSize(0) << "];\n";
    return success();
  }
  pendingAllocs.insert(allocOp.getResult());
  return success();
}

// Bufferization copies a buffer into a fresh allocation before updating the
// copy in place, e.g. for an _assign op on an argument that is still live.
LogicalResult PoulpyEmitter::printOperation(memref::CopyOp copyOp) {
  Value target = copyOp.getTarget();
  if (!pendingAllocs.erase(target)) {
    return copyOp.emitOpError(
        "target must come from an unmaterialized memref.alloc");
  }
  os << "let mut " << variableNames->getNameForValue(target) << " = "
     << variableNames->getNameForValue(copyOp.getSource()) << ".clone();\n";
  return success();
}

FailureOr<std::string> PoulpyEmitter::convertType(Type type, bool isArg,
                                                  bool isMutated) {
  return llvm::TypeSwitch<Type&, FailureOr<std::string>>(type)
      .Case<ModuleType>([&](ModuleType) -> FailureOr<std::string> {
        return std::string(isArg ? "&Module<BE>" : "Module<BE>");
      })
      .Case<ScratchType>([&](ScratchType) -> FailureOr<std::string> {
        return std::string(isArg ? "&mut ScratchOwned<BE>"
                                 : "ScratchOwned<BE>");
      })
      .Case<MemRefType>([&](MemRefType memRefType) -> FailureOr<std::string> {
        Type elementType = memRefType.getElementType();
        if (elementType.isF64()) {
          if (memRefType.getRank() != 1 || memRefType.isDynamicDim(0)) {
            return failure();
          }
          if (!isArg) return std::string("Vec<f64>");
          return std::string(isMutated ? "&mut [f64]" : "&[f64]");
        }
        if (memRefType.getRank() != 0) return failure();
        if (isa<CiphertextType>(elementType)) {
          return std::string(isArg ? (isMutated ? "&mut Ct<BE>" : "&Ct<BE>")
                                   : "Ct<BE>");
        }
        if (isa<PlaintextType>(elementType)) {
          return std::string(isArg ? (isMutated ? "&mut Pt<BE>" : "&Pt<BE>")
                                   : "Pt<BE>");
        }
        return failure();
      })
      .Case<SecretType>([&](SecretType) -> FailureOr<std::string> {
        return std::string(isArg ? "&Secret<BE>" : "Secret<BE>");
      })
      .Case<SecretKeyType>([&](SecretKeyType) -> FailureOr<std::string> {
        return std::string(isArg ? "&Sk<BE>" : "Sk<BE>");
      })
      .Case<TensorKeyType>([&](TensorKeyType) -> FailureOr<std::string> {
        return std::string(isArg ? "&Tsk<BE>" : "Tsk<BE>");
      })
      .Case<AutomorphismKeyMapType>(
          [&](AutomorphismKeyMapType) -> FailureOr<std::string> {
            return std::string(isArg ? "&Akm<BE>" : "Akm<BE>");
          })
      .Default([&](Type&) { return failure(); });
}

LogicalResult PoulpyEmitter::emitType(Type type, bool isArg, bool isMutated) {
  auto result = convertType(type, isArg, isMutated);
  if (failed(result)) {
    return failure();
  }
  os << result;
  return success();
}
PoulpyEmitter::PoulpyEmitter(raw_ostream& os,
                             SelectVariableNames* variableNames)
    : os(os), variableNames(variableNames) {}
}  // namespace poulpy
}  // namespace heir
}  // namespace mlir
