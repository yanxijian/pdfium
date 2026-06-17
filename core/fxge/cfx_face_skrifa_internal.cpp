// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_face_skrifa_internal.h"

#if defined(PDF_ENABLE_FONTATIONS)
#include <memory>

#include "core/fxge/cfx_path.h"

std::unique_ptr<CFX_Path> ConvertOutline(const skrifa::Outline& outline) {
  auto skrifa_path = std::make_unique<CFX_Path>();
  auto point_idx = 0;
  CFX_PointF current_point(0, 0);
  for (auto verb : outline.verbs) {
    switch (verb) {
      case skrifa::PathVerb::MoveTo: {
        auto p = outline.points[point_idx++];
        current_point = CFX_PointF(p.x, p.y);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kMove);
        break;
      }
      case skrifa::PathVerb::LineTo: {
        auto p = outline.points[point_idx++];
        current_point = CFX_PointF(p.x, p.y);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kLine);
        break;
      }
      case skrifa::PathVerb::QuadTo: {
        auto c0 = outline.points[point_idx++];
        auto p = outline.points[point_idx++];
        // Convert quadratic to cubic bezier to match FreeType
        // decomposition.
        skrifa_path->AppendPoint(
            CFX_PointF(current_point.x + (c0.x - current_point.x) * 2 / 3,
                       current_point.y + (c0.y - current_point.y) * 2 / 3),
            CFX_Path::Point::Type::kBezier);
        skrifa_path->AppendPoint(
            CFX_PointF(c0.x + (p.x - c0.x) / 3, c0.y + (p.y - c0.y) / 3),
            CFX_Path::Point::Type::kBezier);
        current_point = CFX_PointF(p.x, p.y);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kBezier);
        break;
      }
      case skrifa::PathVerb::CurveTo: {
        auto c0 = outline.points[point_idx++];
        auto c1 = outline.points[point_idx++];
        auto p = outline.points[point_idx++];
        skrifa_path->AppendPoint(CFX_PointF(c0.x, c0.y),
                                 CFX_Path::Point::Type::kBezier);
        skrifa_path->AppendPoint(CFX_PointF(c1.x, c1.y),
                                 CFX_Path::Point::Type::kBezier);
        current_point = CFX_PointF(p.x, p.y);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kBezier);
        break;
      }
      case skrifa::PathVerb::Close:
        skrifa_path->ClosePath();
        break;
    }
  }
  return skrifa_path;
}
#endif
