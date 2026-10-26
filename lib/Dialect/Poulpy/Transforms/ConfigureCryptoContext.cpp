#include "lib/Dialect/Poulpy/Transforms/ConfigureCryptoContext.h"

#include <cmath>
#include <cstdint>
#include <set>

#include "lib/Dialect/CKKS/IR/CKKSAttributes.h"
#include "lib/Dialect/CKKS/IR/CKKSDialect.h"
#include "lib/Dialect/Poulpy/IR/PoulpyOps.h"
#include "lib/Dialect/Poulpy/IR/PoulpyTypes.h"
#include "lib/Parameters/RLWESecurityParams.h"
#include "lib/Utils/TransformUtils.h"
#include "lib/Utils/Utils.h"
#include "llvm/include/llvm/ADT/STLExtras.h"            // from @llvm-project
#include "llvm/include/llvm/ADT/SmallVector.h"          // from @llvm-project
#include "llvm/include/llvm/Support/MathExtras.h"       // from @llvm-project
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"  // from @llvm-project
#include "mlir/include/mlir/IR/BuiltinOps.h"            // from @llvm-project
#include "mlir/include/mlir/IR/ImplicitLocOpBuilder.h"  // from @llvm-project
#include "mlir/include/mlir/IR/Visitors.h"              // from @llvm-project
#include "mlir/include/mlir/Support/LLVM.h"             // from @llvm-project

namespace mlir::heir::poulpy {

#define GEN_PASS_DEF_CONFIGURECRYPTOCONTEXT
#include "lib/Dialect/Poulpy/Transforms/ConfigureCryptoContext.h.inc"

struct ConfigureCryptoContext
    : impl::ConfigureCryptoContextBase<ConfigureCryptoContext> {
  using ConfigureCryptoContextBase::ConfigureCryptoContextBase;

  void runOnOperation() override {
    ModuleOp module = getOperation();
    auto params = module->getAttrOfType<ckks::SchemeParamAttr>(
        ckks::CKKSDialect::kSchemeParamAttrName);
    if (!params) {
      module.emitError(
          "poulpy-configure-crypto-context needs CKKS scheme "
          "parameters (")
          << ckks::CKKSDialect::kSchemeParamAttrName << ")";
      return signalPassFailure();
    }
    func::FuncOp func = detectEntryFunction(module, entryFunction);
    if (!func) return;

    MLIRContext* ctx = &getContext();
    ArrayRef<Type> argTypes = func.getArgumentTypes();
    bool multiplies = llvm::is_contained(argTypes, TensorKeyType::get(ctx));
    bool rotates =
        llvm::is_contained(argTypes, AutomorphismKeyMapType::get(ctx));
    std::set<int64_t> rotations;
    walkFuncAndCallees(func, [&](Operation* op) {
      if (auto rotate = dyn_cast<RotateOp>(op)) rotations.insert(rotate.getK());
      return WalkResult::advance();
    });

    // The width lwe-to-poulpy gives a ciphertext at the top level.
    int64_t k = 0;
    for (int64_t q : params.getQ().asArrayRef())
      k += std::lround(std::log2(static_cast<double>(q)));
    int64_t logN = params.getLogN();
    int64_t dsize = 1;
    int64_t digit = dsize * base2k;
    int64_t kAux = digit + logN;
    int64_t widest =
        (multiplies || rotates) ? llvm::divideCeil(k, digit) * digit + kAux : k;
    if (computeRingDim(widest, 1 << logN) != 1 << logN) {
      module.emitError() << "a " << widest
                         << "-bit Poulpy modulus is not 128-bit secure at "
                            "ring degree 2^"
                         << logN;
      return signalPassFailure();
    }

    auto b =
        ImplicitLocOpBuilder::atBlockEnd(module.getLoc(), module.getBody());
    SmallVector<Type> resultTypes = {ModuleType::get(ctx),
                                     ScratchType::get(ctx)};
    if (multiplies) resultTypes.push_back(TensorKeyType::get(ctx));
    if (rotates) resultTypes.push_back(AutomorphismKeyMapType::get(ctx));
    resultTypes.push_back(SecretKeyType::get(ctx));
    auto configure =
        func::FuncOp::create(b, (func.getSymName() + "__configure").str(),
                             b.getFunctionType({}, resultTypes));
    b.setInsertionPointToEnd(configure.addEntryBlock());

    Value mod = ModuleCreateOp::create(b, ModuleType::get(ctx), 1 << logN);
    Value scratch = ScratchAllocForAllOpsOp::create(
        b, ScratchType::get(ctx), mod, base2k, k, kAux, dsize,
        params.getLogDefaultScale());
    Value secret =
        SecretGenerateOp::create(b, SecretType::get(ctx), mod, nullptr);
    SmallVector<Value> results = {mod, scratch};
    if (multiplies) {
      results.push_back(TensorKeyGenerateOp::create(b, TensorKeyType::get(ctx),
                                                    mod, secret, scratch,
                                                    base2k, k, kAux, dsize));
    }
    if (rotates) {
      results.push_back(AutomorphismKeysGenerateOp::create(
          b, AutomorphismKeyMapType::get(ctx), mod, secret, scratch, base2k, k,
          kAux, dsize, llvm::to_vector(rotations)));
    }
    results.push_back(
        SecretKeyPrepareOp::create(b, SecretKeyType::get(ctx), mod, secret));
    func::ReturnOp::create(b, results);
  }
};

}  // namespace mlir::heir::poulpy
