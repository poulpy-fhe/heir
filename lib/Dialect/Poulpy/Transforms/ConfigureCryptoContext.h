#ifndef LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_H_
#define LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_H_

#include "mlir/include/mlir/Pass/Pass.h"  // from @llvm-project

// IWYU pragma: begin_keep
#include "lib/Dialect/Poulpy/IR/PoulpyDialect.h"
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"  // from @llvm-project
// IWYU pragma: end_keep

namespace mlir::heir::poulpy {

#define GEN_PASS_DECL
#include "lib/Dialect/Poulpy/Transforms/ConfigureCryptoContext.h.inc"

#define GEN_PASS_REGISTRATION
#include "lib/Dialect/Poulpy/Transforms/ConfigureCryptoContext.h.inc"

}  // namespace mlir::heir::poulpy

#endif  // LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_H_
