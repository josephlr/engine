// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/transform_layer.h"

namespace flow {

TransformLayer::TransformLayer() {
  transform_.setIdentity();
}

TransformLayer::~TransformLayer() = default;

void TransformLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  // Checks (in some degree) that SkMatrix transform_ is valid and initialized.
  //
  // We need this even if is_set_ is true since one can call set_transform with
  // an uninitialized SkMatrix.
  //
  // If transform_ is uninitialized, this assert may look flaky as it doesn't
  // fail all the time, and some rerun may make it pass. But don't ignore it and
  // just rerun the test if this is triggered, since even a flaky failure here
  // may signify a potentially big problem in the code.
  //
  // We have to write this flaky test because there is no reliable way to test
  // whether a variable is initialized or not in C++.
  FML_CHECK(transform_.isFinite());

  SkMatrix child_matrix;
  child_matrix.setConcat(matrix, transform_);

  SkRect previous_cull_rect = context->cull_rect;
  SkMatrix inverse_transform_;
  // Perspective projections don't produce rectangles that are useful for
  // culling for some reason.
  if (!transform_.hasPerspective() && transform_.invert(&inverse_transform_)) {
    inverse_transform_.mapRect(&context->cull_rect);
  } else {
    context->cull_rect = kGiantRect;
  }

  SkRect child_paint_bounds = SkRect::MakeEmpty();
  PrerollChildren(context, child_matrix, &child_paint_bounds);

  transform_.mapRect(&child_paint_bounds);
  set_paint_bounds(child_paint_bounds);

  context->cull_rect = previous_cull_rect;
}

#if defined(OS_FUCHSIA)

void TransformLayer::UpdateScene(SceneUpdateContext& context) {
  FML_DCHECK(needs_system_composite());

  SceneUpdateContext::Transform transform(context, transform_);
  UpdateSceneChildren(context);
}

#endif  // defined(OS_FUCHSIA)

void TransformLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "TransformLayer::Paint");
  FML_DCHECK(needs_painting());

  SkAutoCanvasRestore save(context.internal_nodes_canvas, true);
  context.internal_nodes_canvas->concat(transform_);
  PaintChildren(context);
}

}  // namespace flow
