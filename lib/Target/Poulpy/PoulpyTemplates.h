#ifndef LIB_TARGET_POULPY_POULPYTEMPLATES_H_
#define LIB_TARGET_POULPY_POULPYTEMPLATES_H_

#include <string_view>

namespace mlir {
namespace heir {
namespace poulpy {
constexpr std::string_view kModulePrelude =
    R"poulpy(#![allow(unused_imports, unused_variables, unused_mut, non_snake_case)]

use anyhow::Result;
use std::collections::HashMap;
use poulpy_ckks::prelude::*;
use poulpy_ckks::api::CKKSAllOpsTmpBytes;
use poulpy_ckks::layouts::{CKKSCiphertextOwned, CKKSPlaintextOwned};
use poulpy_core::{GLWEAutomorphismKeyEncryptSk, GLWETensorKeyEncryptSk};
use poulpy_core::layouts::prepared::{GLWESecretPrepared, GLWETensorKeyPrepared};
use poulpy_core::layouts::{
    BackendGLWESecret, Base2K, Dsize, GGLWELayout, GLWEAutomorphismKeyLayout,
    GLWEAutomorphismKeyPrepared, GLWEAutomorphismKeyPreparedFactory, GLWEInfos,
    GLWELayout, GLWESecretPreparedFactory, GLWESecretSampling,
    GLWETensorKeyLayout, GLWETensorKeyPreparedFactory, LWEInfos,
    ModuleCoreAlloc, Rank, TorusPrecision,
};
use poulpy_hal::{
    api::{ModuleNew, ScratchOwnedAlloc, ScratchOwnedBorrow},
    layouts::{Backend, Module, ScratchOwned},
    source::Source,
};

/// The backend the generated code runs on. The caller picks it, e.g.
/// `f::<NTT4x30Portable>(...)`.
pub trait PoulpyBackend: Backend<OwnedBuf: Clone> {}
impl<T: Backend<OwnedBuf: Clone>> PoulpyBackend for T {}

/// Everything the generated code calls on `Module<BE>`.
pub trait PoulpyModule<BE: Backend>:
    ModuleNew<BE>
    + ModuleCoreAlloc<OwnedBuf = BE::OwnedBuf, ZnxWord = BE::ZnxWord>
    + CKKSModuleAlloc<BE>
    + CKKSModuleInfos
    + CKKSAllOpsTmpBytes<BE>
    + CKKSEncodingOps<BE, f64>
    + CKKSEncryptOps<BE>
    + CKKSDecryptOps<BE>
    + CKKSAddOps<BE>
    + CKKSSubOps<BE>
    + CKKSMulOps<BE>
    + CKKSNegOps<BE>
    + CKKSCopyOps<BE>
    + CKKSRotateOps<BE>
    + CKKSPow2Ops<BE>
    + GLWESecretSampling<BE>
    + GLWESecretPreparedFactory<BE>
    + GLWETensorKeyEncryptSk<BE>
    + GLWETensorKeyPreparedFactory<BE>
    + GLWEAutomorphismKeyEncryptSk<BE>
    + GLWEAutomorphismKeyPreparedFactory<BE>
{
}
impl<BE: Backend, M> PoulpyModule<BE> for M where
    M: ModuleNew<BE>
        + ModuleCoreAlloc<OwnedBuf = BE::OwnedBuf, ZnxWord = BE::ZnxWord>
        + CKKSModuleAlloc<BE>
        + CKKSModuleInfos
        + CKKSAllOpsTmpBytes<BE>
        + CKKSEncodingOps<BE, f64>
        + CKKSEncryptOps<BE>
        + CKKSDecryptOps<BE>
        + CKKSAddOps<BE>
        + CKKSSubOps<BE>
        + CKKSMulOps<BE>
        + CKKSNegOps<BE>
        + CKKSCopyOps<BE>
        + CKKSRotateOps<BE>
        + CKKSPow2Ops<BE>
        + GLWESecretSampling<BE>
        + GLWESecretPreparedFactory<BE>
        + GLWETensorKeyEncryptSk<BE>
        + GLWETensorKeyPreparedFactory<BE>
        + GLWEAutomorphismKeyEncryptSk<BE>
        + GLWEAutomorphismKeyPreparedFactory<BE>
{
}
)poulpy";

constexpr std::string_view kTypeAliases = R"poulpy(
type Ct<BE> = CKKSCiphertextOwned<BE>;
type Pt<BE> = CKKSPlaintextOwned<BE>;
type Secret<BE> = BackendGLWESecret<BE>;
type Sk<BE> = GLWESecretPrepared<<BE as Backend>::OwnedBuf, BE>;
type Tsk<BE> = GLWETensorKeyPrepared<<BE as Backend>::OwnedBuf, BE>;
type Akm<BE> = HashMap<i64, GLWEAutomorphismKeyPrepared<<BE as Backend>::OwnedBuf, BE>>;
)poulpy";
}  // namespace poulpy
}  // namespace heir
}  // namespace mlir

#endif  // LIB_TARGET_POULPY_POULPYTEMPLATES_H_
