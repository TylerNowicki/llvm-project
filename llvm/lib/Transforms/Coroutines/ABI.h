//===- ABI.h - Coroutine ABI Transformers ---------------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This file declares the pass that analyzes a function for coroutine intrs and
// a transformer class that contains methods for handling different steps of
// coroutine lowering.
//===----------------------------------------------------------------------===//

#ifndef LIB_TRANSFORMS_COROUTINES_ABI_H
#define LIB_TRANSFORMS_COROUTINES_ABI_H

#include "CoroShape.h"
#include "MaterializationUtils.h"
#include "SuspendCrossingInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"

namespace llvm {

class Function;

namespace coro {

// This interface/API is to provide an object oriented way to implement ABI
// functionality. This is intended to replace use of switch(ABI) to perform
// ABI specific behavior. The ABIs (e.g. Switch, Async, Retcon{Once}) are the
// common ABIs. However, specific users often need to modify the behavior of
// these, such as for C++20 or Swift. This can be accomplished by inheriting
// one of the common ABIs and overriding one or more of the methods to create
// a custom ABI. The custom ABI is specified with the coro.begin.custom.abi
// intrinsic instead of the coro.begin intrinsic by providing an i32 in the
// last argument. This is used to lookup a generator for the custom ABI from
// a set of generators provided to the CoroSplitPass constructor.
//
class LLVM_LIBRARY_VISIBILITY BaseABI {
public:
  BaseABI(Function &F, Shape &S)
      : F(F), Shape(S), IsMaterializable(coro::isTriviallyMaterializable) {}

  BaseABI(Function &F, coro::Shape &S,
          std::function<bool(Instruction &)> IsMaterializable)
      : F(F), Shape(S), IsMaterializable(IsMaterializable) {}

  // Initialize the coroutine ABI
  virtual void init() = 0;

  // Allocate the coroutine frame and do spill/reload as needed.
  virtual void buildCoroutineFrame(bool OptimizeFrame);

  // Perform the function splitting according to the ABI.
  virtual void splitCoroutine(Function &F, coro::Shape &Shape,
                              SmallVectorImpl<Function *> &Clones,
                              TargetTransformInfo &TTI) = 0;

  Function &F;
  coro::Shape &Shape;

  // Callback used by coro::BaseABI::buildCoroutineFrame for rematerialization.
  // It is provided to coro::doMaterializations(..).
  std::function<bool(Instruction &I)> IsMaterializable;
};

class LLVM_LIBRARY_VISIBILITY SwitchABI : public BaseABI {
public:
  SwitchABI(Function &F, coro::Shape &S) : BaseABI(F, S) {}

  SwitchABI(Function &F, coro::Shape &S,
            std::function<bool(Instruction &)> IsMaterializable)
      : BaseABI(F, S, IsMaterializable) {}

  void init() override;

  void splitCoroutine(Function &F, coro::Shape &Shape,
                      SmallVectorImpl<Function *> &Clones,
                      TargetTransformInfo &TTI) override;
};

class LLVM_LIBRARY_VISIBILITY AsyncABI : public BaseABI {
public:
  AsyncABI(Function &F, coro::Shape &S) : BaseABI(F, S) {}

  AsyncABI(Function &F, coro::Shape &S,
           std::function<bool(Instruction &)> IsMaterializable)
      : BaseABI(F, S, IsMaterializable) {}

  void init() override;

  void splitCoroutine(Function &F, coro::Shape &Shape,
                      SmallVectorImpl<Function *> &Clones,
                      TargetTransformInfo &TTI) override;
};

class LLVM_LIBRARY_VISIBILITY AnyRetconABI : public BaseABI {
public:
  AnyRetconABI(Function &F, coro::Shape &S) : BaseABI(F, S) {}

  AnyRetconABI(Function &F, coro::Shape &S,
               std::function<bool(Instruction &)> IsMaterializable)
      : BaseABI(F, S, IsMaterializable) {}

  void init() override;

  void splitCoroutine(Function &F, coro::Shape &Shape,
                      SmallVectorImpl<Function *> &Clones,
                      TargetTransformInfo &TTI) override;
};

} // end namespace coro

} // end namespace llvm

#endif // LLVM_TRANSFORMS_COROUTINES_ABI_H
