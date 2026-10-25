#ifndef LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_H_
#define LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_H_

#include "mlir/include/mlir/Pass/Pass.h"  // from @llvm-project

// IWYU pragma: begin_keep
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"    // from @llvm-project
#include "mlir/include/mlir/Dialect/Tensor/IR/Tensor.h"  // from @llvm-project
// IWYU pragma: end_keep

namespace mlir::heir::lwe {

#define GEN_PASS_DECL
#include "lib/Dialect/LWE/Conversions/LWEToPoulpy/LWEToPoulpy.h.inc"

#define GEN_PASS_REGISTRATION
#include "lib/Dialect/LWE/Conversions/LWEToPoulpy/LWEToPoulpy.h.inc"

}  // namespace mlir::heir::lwe

#endif  // LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_H_
