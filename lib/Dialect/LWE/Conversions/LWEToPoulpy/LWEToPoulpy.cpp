#include "lib/Dialect/LWE/Conversions/LWEToPoulpy/LWEToPoulpy.h"

#include <cmath>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>

#include "lib/Dialect/CKKS/IR/CKKSAttributes.h"
#include "lib/Dialect/CKKS/IR/CKKSDialect.h"
#include "lib/Dialect/CKKS/IR/CKKSOps.h"
#include "lib/Dialect/LWE/IR/LWEAttributes.h"
#include "lib/Dialect/LWE/IR/LWEDialect.h"
#include "lib/Dialect/LWE/IR/LWEOps.h"
#include "lib/Dialect/LWE/IR/LWETypes.h"
#include "lib/Dialect/Poulpy/IR/PoulpyDialect.h"
#include "lib/Dialect/Poulpy/IR/PoulpyOps.h"
#include "lib/Dialect/Poulpy/IR/PoulpyTypes.h"
#include "lib/Utils/ConversionUtils.h"
#include "lib/Utils/Utils.h"
#include "llvm/include/llvm/ADT/DenseMap.h"              // from @llvm-project
#include "llvm/include/llvm/ADT/STLExtras.h"             // from @llvm-project
#include "llvm/include/llvm/ADT/SmallVector.h"           // from @llvm-project
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"    // from @llvm-project
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"   // from @llvm-project
#include "mlir/include/mlir/Dialect/Tensor/IR/Tensor.h"  // from @llvm-project
#include "mlir/include/mlir/IR/BuiltinAttributes.h"      // from @llvm-project
#include "mlir/include/mlir/IR/BuiltinOps.h"             // from @llvm-project
#include "mlir/include/mlir/IR/BuiltinTypes.h"           // from @llvm-project
#include "mlir/include/mlir/IR/Matchers.h"               // from @llvm-project
#include "mlir/include/mlir/IR/PatternMatch.h"           // from @llvm-project
#include "mlir/include/mlir/IR/TypeUtilities.h"          // from @llvm-project
#include "mlir/include/mlir/IR/Value.h"                  // from @llvm-project
#include "mlir/include/mlir/IR/Visitors.h"               // from @llvm-project
#include "mlir/include/mlir/Support/LLVM.h"              // from @llvm-project
#include "mlir/include/mlir/Transforms/DialectConversion.h"  // from @llvm-project

namespace mlir::heir::lwe {

#define GEN_PASS_DEF_LWETOPOULPY
#include "lib/Dialect/LWE/Conversions/LWEToPoulpy/LWEToPoulpy.h.inc"

namespace {

bool isLwePayload(Type type) {
  return isa<LWECiphertextType, LWEPlaintextType>(getElementTypeOrSelf(type));
}

bool isLweType(Type type) {
  return isa<LWEDialect>(&getElementTypeOrSelf(type).getDialect());
}

// Ciphertexts and plaintexts become rank-0 tensors of the poulpy payload types,
// the tensor form of the poulpy ops. A unit tensor of them, as the client
// interface passes ciphertexts, becomes the single value it holds.
class ToPoulpyTypeConverter : public TypeConverter {
 public:
  explicit ToPoulpyTypeConverter(MLIRContext* ctx) {
    addConversion([](Type type) { return type; });
    addConversion([ctx](LWECiphertextType) -> Type {
      return RankedTensorType::get({}, poulpy::CiphertextType::get(ctx));
    });
    addConversion([ctx](LWEPlaintextType) -> Type {
      return RankedTensorType::get({}, poulpy::PlaintextType::get(ctx));
    });
    addConversion([ctx](LWESecretKeyType) -> Type {
      return poulpy::SecretKeyType::get(ctx);
    });
    addConversion([this](RankedTensorType type) -> Type {
      if (!isLwePayload(type)) return type;
      if (type.getNumElements() != 1) return Type();
      return convertType(type.getElementType());
    });
  }
};

// The width in bits of a ciphertext at `level`: the bit sizes of the first
// `level + 1` moduli.
int64_t widthAtLevel(ckks::SchemeParamAttr params, int64_t level) {
  ArrayRef<int64_t> moduli = params.getQ().asArrayRef();
  return std::accumulate(
      moduli.begin(), moduli.begin() + level + 1, int64_t{0},
      [](int64_t sum, int64_t q) {
        return sum + std::lround(std::log2(static_cast<double>(q)));
      });
}

int64_t ciphertextLevel(Type type) {
  return cast<LWECiphertextType>(getElementTypeOrSelf(type))
      .getModulusChain()
      .getCurrent();
}

bool isPowerOfTwoFloatTensor(Type type) {
  auto tensorType = dyn_cast<RankedTensorType>(type);
  return tensorType && tensorType.hasStaticShape() &&
         isa<FloatType>(tensorType.getElementType()) &&
         llvm::isPowerOf2_64(tensorType.getNumElements());
}

// Rejects what the lowering does not support, with a reason, before any IR is
// changed.
LogicalResult validate(ModuleOp module) {
  auto checkTypes = [](Operation* op, TypeRange types) -> LogicalResult {
    for (Type type : types) {
      if (isa<LWEPublicKeyType>(type)) {
        return op->emitError(
            "public-key encryption is not supported: poulpy-ckks only has "
            "secret-key encryption; run --mlir-to-ckks with "
            "use-public-key=false");
      }
      auto shaped = dyn_cast<ShapedType>(type);
      if (shaped && isLwePayload(shaped) &&
          (!shaped.hasStaticShape() || shaped.getNumElements() != 1)) {
        return op->emitError(
            "tensors of more than one ciphertext or plaintext are not "
            "supported yet");
      }
    }
    return success();
  };

  WalkResult result = module.walk<WalkOrder::PreOrder>([&](Operation* op)
                                                           -> WalkResult {
    if (auto func = dyn_cast<func::FuncOp>(op)) {
      if (failed(checkTypes(op, func.getArgumentTypes())) ||
          failed(checkTypes(op, func.getResultTypes())))
        return WalkResult::interrupt();
    }
    if (failed(checkTypes(op, op->getOperandTypes())) ||
        failed(checkTypes(op, op->getResultTypes())))
      return WalkResult::interrupt();

    StringRef dialect =
        op->getDialect() ? op->getDialect()->getNamespace() : "";
    if (dialect == "preprocessing") {
      op->emitError(
          "split preprocessing is not supported yet; run --mlir-to-ckks with "
          "enable-split-preprocessing=false");
      return WalkResult::interrupt();
    }
    if (dialect == "kernel") {
      op->emitError("kernel ops are not supported yet");
      return WalkResult::interrupt();
    }

    // Poulpy's multiplication relinearizes, so each multiplication must be
    // relinearized right away, and a rescale is a no-op that may only follow
    // a multiplication.
    if (auto mul = dyn_cast<RMulOp>(op)) {
      if (!mul->hasOneUse() || !isa<ckks::RelinearizeOp>(*mul->user_begin())) {
        op->emitError(
            "a ciphertext multiplication must be relinearized right away: "
            "Poulpy's multiplication always relinearizes");
        return WalkResult::interrupt();
      }
    }
    if (auto relin = dyn_cast<ckks::RelinearizeOp>(op)) {
      if (!relin.getInput().getDefiningOp<RMulOp>()) {
        op->emitError("relinearization must directly follow a multiplication");
        return WalkResult::interrupt();
      }
    }
    if (auto rescale = dyn_cast<ckks::RescaleOp>(op)) {
      Operation* producer = rescale.getInput().getDefiningOp();
      if (!isa_and_nonnull<ckks::RelinearizeOp, RMulPlainOp>(producer)) {
        op->emitError(
            "rescale is a no-op in Poulpy and must directly follow a "
            "multiplication");
        return WalkResult::interrupt();
      }
    }
    if (auto rotate = dyn_cast<ckks::RotateOp>(op)) {
      if (!rotate.getStaticShift() &&
          !matchPattern(rotate.getDynamicShift(), m_Constant())) {
        op->emitError("rotations by a non-constant shift are not supported");
        return WalkResult::interrupt();
      }
    }
    if (isa<ckks::BootstrapOp>(op)) {
      op->emitError("bootstrapping is not supported yet");
      return WalkResult::interrupt();
    }
    if (auto encode = dyn_cast<RLWEEncodeOp>(op)) {
      if (!encode.getLevel() || !encode.getScale()) {
        op->emitError("encoding needs a level and a scale");
        return WalkResult::interrupt();
      }
      if (!isPowerOfTwoFloatTensor(encode.getInput().getType())) {
        op->emitError(
            "only static float tensors of a power-of-two size can be "
            "encoded");
        return WalkResult::interrupt();
      }
    }
    if (auto decode = dyn_cast<RLWEDecodeOp>(op)) {
      if (!isPowerOfTwoFloatTensor(decode.getResult().getType())) {
        op->emitError(
            "only static float tensors of a power-of-two size can be "
            "decoded");
        return WalkResult::interrupt();
      }
    }
    return WalkResult::advance();
  });
  return failure(result.wasInterrupted());
}

// The poulpy context values a function needs, in argument order: the module
// and scratch arena for any ciphertext or plaintext work, the tensor key to
// multiply and the automorphism keys to rotate, including in its callees.
SmallVector<Type> contextTypes(func::FuncOp func) {
  bool usesPoulpy = llvm::any_of(func.getArgumentTypes(), isLweType) ||
                    llvm::any_of(func.getResultTypes(), isLweType);
  bool multiplies = false;
  bool rotates = false;
  walkFuncAndCallees(func, [&](Operation* op) {
    usesPoulpy |=
        isa_and_nonnull<LWEDialect, ckks::CKKSDialect>(op->getDialect());
    multiplies |= isa<RMulOp>(op);
    rotates |= isa<ckks::RotateOp>(op);
    return WalkResult::advance();
  });

  MLIRContext* ctx = func.getContext();
  SmallVector<Type> types;
  if (!usesPoulpy) return types;
  types.push_back(poulpy::ModuleType::get(ctx));
  types.push_back(poulpy::ScratchType::get(ctx));
  if (multiplies) types.push_back(poulpy::TensorKeyType::get(ctx));
  if (rotates) types.push_back(poulpy::AutomorphismKeyMapType::get(ctx));
  return types;
}

// Prepends the context values each function needs and passes them at every
// call. A caller needs everything its callees need, so it has them to pass.
void addContextArgs(ModuleOp module) {
  llvm::DenseMap<Operation*, SmallVector<Type>> needed;
  module.walk([&](func::FuncOp func) { needed[func] = contextTypes(func); });

  for (auto& [op, types] : needed) {
    if (types.empty()) continue;
    auto func = cast<func::FuncOp>(op);
    SmallVector<unsigned> indices(types.size(), 0);
    SmallVector<DictionaryAttr> attrs(types.size(), nullptr);
    SmallVector<Location> locs(types.size(), func.getLoc());
    (void)func.insertArguments(indices, types, attrs, locs);
  }

  module.walk([&](func::CallOp call) {
    FailureOr<func::FuncOp> callee = getCalledFunction(call);
    if (failed(callee)) return;
    SmallVector<Value> args;
    for (Type type : needed[*callee])
      args.push_back(*getContextualArgFromFunc(call, type));
    call->insertOperands(0, args);
  });
}

// Shared state of the op patterns: the scheme parameters that fix ciphertext
// widths, the limb size, and access to the enclosing function's context.
template <typename OpTy>
struct PoulpyPattern : public OpConversionPattern<OpTy> {
  PoulpyPattern(const TypeConverter& typeConverter, MLIRContext* ctx,
                ckks::SchemeParamAttr params, int64_t base2k)
      : OpConversionPattern<OpTy>(typeConverter, ctx),
        params(params),
        base2k(base2k) {}

  Value context(Operation* op, Type type) const {
    return *getContextualArgFromFunc(op, type);
  }
  Value module(Operation* op) const {
    return context(op, poulpy::ModuleType::get(op->getContext()));
  }
  Value scratch(Operation* op) const {
    return context(op, poulpy::ScratchType::get(op->getContext()));
  }
  Value emptyCiphertext(OpBuilder& b, Location loc) const {
    return tensor::EmptyOp::create(b, loc, ArrayRef<int64_t>{},
                                   poulpy::CiphertextType::get(b.getContext()));
  }
  Value emptyPlaintext(OpBuilder& b, Location loc) const {
    return tensor::EmptyOp::create(b, loc, ArrayRef<int64_t>{},
                                   poulpy::PlaintextType::get(b.getContext()));
  }

  ckks::SchemeParamAttr params;
  int64_t base2k;
};

template <typename LweOp, typename PoulpyOp>
struct ConvertCtCtOp : public PoulpyPattern<LweOp> {
  using PoulpyPattern<LweOp>::PoulpyPattern;

  LogicalResult matchAndRewrite(
      LweOp op, typename LweOp::Adaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Value dst = this->emptyCiphertext(rewriter, op.getLoc());
    rewriter.replaceOp(op,
                       PoulpyOp::create(rewriter, op.getLoc(), dst.getType(),
                                        this->module(op), dst, adaptor.getLhs(),
                                        adaptor.getRhs(), this->scratch(op)));
    return success();
  }
};

// The multiplication relinearizes and rescales: its ckks.relinearize and
// ckks.rescale are erased.
struct ConvertMulOp : public PoulpyPattern<RMulOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RMulOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Value dst = emptyCiphertext(rewriter, op.getLoc());
    Value tensorKey = context(op, poulpy::TensorKeyType::get(op->getContext()));
    rewriter.replaceOp(
        op, poulpy::MulOp::create(rewriter, op.getLoc(), dst.getType(),
                                  module(op), dst, adaptor.getLhs(),
                                  adaptor.getRhs(), tensorKey, scratch(op)));
    return success();
  }
};

template <typename OpTy>
struct EraseOp : public OpConversionPattern<OpTy> {
  using OpConversionPattern<OpTy>::OpConversionPattern;

  LogicalResult matchAndRewrite(
      OpTy op, typename OpTy::Adaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    rewriter.replaceOp(op, adaptor.getInput());
    return success();
  }
};

// The ciphertext and plaintext operands of a ciphertext-plaintext op, which may
// come in either order.
template <typename OpTy>
std::pair<Value, Value> ciphertextAndPlaintext(OpTy op,
                                               typename OpTy::Adaptor adaptor) {
  bool lhsIsPlaintext =
      isa<LWEPlaintextType>(getElementTypeOrSelf(op.getLhs().getType()));
  return lhsIsPlaintext ? std::pair(adaptor.getRhs(), adaptor.getLhs())
                        : std::pair(adaptor.getLhs(), adaptor.getRhs());
}

template <typename LweOp, typename PoulpyOp>
struct ConvertCommutativePlainOp : public PoulpyPattern<LweOp> {
  using PoulpyPattern<LweOp>::PoulpyPattern;

  LogicalResult matchAndRewrite(
      LweOp op, typename LweOp::Adaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    auto [ciphertext, plaintext] = ciphertextAndPlaintext(op, adaptor);
    Value dst = this->emptyCiphertext(rewriter, op.getLoc());
    rewriter.replaceOp(
        op,
        PoulpyOp::create(rewriter, op.getLoc(), dst.getType(), this->module(op),
                         dst, ciphertext, plaintext, this->scratch(op)));
    return success();
  }
};

// `pt - ct` is computed as `-(ct - pt)`.
struct ConvertSubPlainOp : public PoulpyPattern<RSubPlainOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RSubPlainOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    auto [ciphertext, plaintext] = ciphertextAndPlaintext(op, adaptor);
    Location loc = op.getLoc();
    Value dst = emptyCiphertext(rewriter, loc);
    Value result =
        poulpy::SubPtVecOp::create(rewriter, loc, dst.getType(), module(op),
                                   dst, ciphertext, plaintext, scratch(op))
            ->getResult(0);
    if (isa<LWEPlaintextType>(getElementTypeOrSelf(op.getLhs().getType()))) {
      result = poulpy::NegAssignOp::create(rewriter, loc, result.getType(),
                                           module(op), result, scratch(op))
                   ->getResult(0);
    }
    rewriter.replaceOp(op, result);
    return success();
  }
};

struct ConvertNegateOp : public PoulpyPattern<RNegateOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RNegateOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Value dst = emptyCiphertext(rewriter, op.getLoc());
    rewriter.replaceOp(op, poulpy::NegOp::create(
                               rewriter, op.getLoc(), dst.getType(), module(op),
                               dst, adaptor.getInput(), scratch(op)));
    return success();
  }
};

struct ConvertRotateOp : public PoulpyPattern<ckks::RotateOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      ckks::RotateOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    APInt shift;
    if (auto staticShift = op.getStaticShift()) {
      shift = staticShift->getValue();
    } else if (!matchPattern(op.getDynamicShift(), m_ConstantInt(&shift))) {
      return rewriter.notifyMatchFailure(op, "non-constant shift");
    }
    Value dst = emptyCiphertext(rewriter, op.getLoc());
    Value keys =
        context(op, poulpy::AutomorphismKeyMapType::get(op->getContext()));
    rewriter.replaceOp(op, poulpy::RotateOp::create(
                               rewriter, op.getLoc(), dst.getType(), module(op),
                               dst, adaptor.getInput(),
                               rewriter.getI64IntegerAttr(shift.getSExtValue()),
                               keys, scratch(op)));
    return success();
  }
};

// Encodes the cleartext as the real parts of as many slots as it has values.
struct ConvertEncodeOp : public PoulpyPattern<RLWEEncodeOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RLWEEncodeOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Location loc = op.getLoc();
    auto inputType = cast<RankedTensorType>(adaptor.getInput().getType());
    auto realType = RankedTensorType::get({inputType.getNumElements()},
                                          rewriter.getF64Type());
    Value real = adaptor.getInput();
    if (inputType.getRank() != 1) {
      ReassociationIndices all(inputType.getRank());
      std::iota(all.begin(), all.end(), 0);
      real = tensor::CollapseShapeOp::create(
          rewriter, loc, real, ArrayRef<ReassociationIndices>{all});
    }
    if (!inputType.getElementType().isF64())
      real = arith::ExtFOp::create(rewriter, loc, realType, real);
    Value imag = arith::ConstantOp::create(rewriter, loc, realType,
                                           rewriter.getZeroAttr(realType));

    int64_t logDelta = *op.getScale();
    int64_t k = widthAtLevel(params, *op.getLevel());
    Value dst = emptyPlaintext(rewriter, loc);
    rewriter.replaceOp(
        op, poulpy::EncodeOp::create(rewriter, loc, dst.getType(), module(op),
                                     dst, real, imag, scratch(op),
                                     rewriter.getI64IntegerAttr(logDelta),
                                     rewriter.getI64IntegerAttr(k - logDelta),
                                     rewriter.getI64IntegerAttr(base2k)));
    return success();
  }
};

// Decodes as many slots as the cleartext has values and keeps their real
// parts.
struct ConvertDecodeOp : public PoulpyPattern<RLWEDecodeOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RLWEDecodeOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Location loc = op.getLoc();
    auto resultType = cast<RankedTensorType>(op.getResult().getType());
    int64_t size = resultType.getNumElements();
    auto slotsType = RankedTensorType::get({size}, rewriter.getF64Type());
    Value realDst = tensor::EmptyOp::create(
        rewriter, loc, ArrayRef<int64_t>{size}, rewriter.getF64Type());
    Value imagDst = tensor::EmptyOp::create(
        rewriter, loc, ArrayRef<int64_t>{size}, rewriter.getF64Type());
    auto decode = poulpy::DecodeOp::create(
        rewriter, loc, TypeRange{slotsType, slotsType}, module(op), realDst,
        imagDst, adaptor.getInput(), scratch(op));

    Value real = decode->getResult(0);
    if (!resultType.getElementType().isF64()) {
      real = arith::TruncFOp::create(
          rewriter, loc,
          RankedTensorType::get({size}, resultType.getElementType()), real);
    }
    if (resultType.getRank() != 1) {
      ReassociationIndices all(resultType.getRank());
      std::iota(all.begin(), all.end(), 0);
      real = tensor::ExpandShapeOp::create(rewriter, loc, resultType, real,
                                           ArrayRef<ReassociationIndices>{all});
    }
    rewriter.replaceOp(op, real);
    return success();
  }
};

struct ConvertEncryptOp : public PoulpyPattern<RLWEEncryptOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RLWEEncryptOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    int64_t k = widthAtLevel(params, ciphertextLevel(op.getOutput().getType()));
    Value dst = emptyCiphertext(rewriter, op.getLoc());
    rewriter.replaceOp(op, poulpy::EncryptOp::create(
                               rewriter, op.getLoc(), dst.getType(), module(op),
                               dst, adaptor.getInput(), adaptor.getKey(),
                               scratch(op), rewriter.getI64IntegerAttr(base2k),
                               rewriter.getI64IntegerAttr(k)));
    return success();
  }
};

struct ConvertDecryptOp : public PoulpyPattern<RLWEDecryptOp> {
  using PoulpyPattern::PoulpyPattern;

  LogicalResult matchAndRewrite(
      RLWEDecryptOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    Value dst = emptyPlaintext(rewriter, op.getLoc());
    rewriter.replaceOp(
        op, poulpy::DecryptOp::create(rewriter, op.getLoc(), dst.getType(),
                                      module(op), dst, adaptor.getInput(),
                                      adaptor.getSecretKey(), scratch(op)));
    return success();
  }
};

// A unit tensor of ciphertexts or plaintexts is the value it holds.
struct ConvertUnitExtractOp : public OpConversionPattern<tensor::ExtractOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      tensor::ExtractOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    if (!isLwePayload(op.getTensor().getType())) return failure();
    rewriter.replaceOp(op, adaptor.getTensor());
    return success();
  }
};

struct ConvertUnitInsertOp : public OpConversionPattern<tensor::InsertOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      tensor::InsertOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    if (!isLwePayload(op.getDest().getType())) return failure();
    rewriter.replaceOp(op, adaptor.getScalar());
    return success();
  }
};

struct ConvertUnitFromElementsOp
    : public OpConversionPattern<tensor::FromElementsOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      tensor::FromElementsOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    if (!isLwePayload(op.getType())) return failure();
    rewriter.replaceOp(op, adaptor.getElements().front());
    return success();
  }
};

struct ConvertUnitEmptyOp : public OpConversionPattern<tensor::EmptyOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      tensor::EmptyOp op, OpAdaptor adaptor,
      ConversionPatternRewriter& rewriter) const override {
    if (!isLwePayload(op.getType())) return failure();
    auto type =
        cast<RankedTensorType>(getTypeConverter()->convertType(op.getType()));
    rewriter.replaceOp(
        op, tensor::EmptyOp::create(rewriter, op.getLoc(), type.getShape(),
                                    type.getElementType()));
    return success();
  }
};

}  // namespace

struct LWEToPoulpy : impl::LWEToPoulpyBase<LWEToPoulpy> {
  using LWEToPoulpyBase::LWEToPoulpyBase;

  void runOnOperation() override {
    ModuleOp module = getOperation();
    MLIRContext* context = &getContext();

    auto params = module->getAttrOfType<ckks::SchemeParamAttr>(
        ckks::CKKSDialect::kSchemeParamAttrName);
    if (!params) {
      module.emitError("lwe-to-poulpy needs CKKS scheme parameters (")
          << ckks::CKKSDialect::kSchemeParamAttrName << ")";
      return signalPassFailure();
    }
    if (failed(validate(module))) return signalPassFailure();

    addContextArgs(module);
    // See https://github.com/llvm/llvm-project/pull/127772: dialect
    // conversion drops the dialect attributes of func.call ops.
    auto funcCallOpDialectAttrs = saveFuncCallOpDialectAttrs(module);

    ToPoulpyTypeConverter typeConverter(context);
    ConversionTarget target(*context);
    target.addLegalDialect<poulpy::PoulpyDialect, arith::ArithDialect>();
    target.addIllegalDialect<LWEDialect, ckks::CKKSDialect>();

    RewritePatternSet patterns(context);
    addStructuralConversionPatterns(typeConverter, patterns, target);
    // Set after the structural patterns, which install their own rule.
    target.markUnknownOpDynamicallyLegal(
        [&](Operation* op) { return typeConverter.isLegal(op); });
    patterns.add<ConvertCtCtOp<RAddOp, poulpy::AddOp>,
                 ConvertCtCtOp<RSubOp, poulpy::SubOp>, ConvertMulOp,
                 ConvertCommutativePlainOp<RAddPlainOp, poulpy::AddPtVecOp>,
                 ConvertCommutativePlainOp<RMulPlainOp, poulpy::MulPtVecOp>,
                 ConvertSubPlainOp, ConvertNegateOp, ConvertRotateOp,
                 ConvertEncodeOp, ConvertDecodeOp, ConvertEncryptOp,
                 ConvertDecryptOp>(typeConverter, context, params, base2k);
    patterns.add<EraseOp<ckks::RelinearizeOp>, EraseOp<ckks::RescaleOp>,
                 EraseOp<ckks::LevelReduceOp>, ConvertUnitExtractOp,
                 ConvertUnitInsertOp, ConvertUnitFromElementsOp,
                 ConvertUnitEmptyOp>(typeConverter, context);

    ConversionConfig config;
    config.allowPatternRollback = false;
    if (failed(applyPartialConversion(module, target, std::move(patterns),
                                      config))) {
      return signalPassFailure();
    }
    restoreFuncCallOpDialectAttrs(module, funcCallOpDialectAttrs);
  }
};

}  // namespace mlir::heir::lwe
