// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "esp/gfx/Sampler.h"

int main(int argc, char** argv) {
  esp::gfx::Sampler app({argc, argv});
  return app.exec();
}
