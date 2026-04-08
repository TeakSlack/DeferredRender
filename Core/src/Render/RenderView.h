#ifndef RENDER_VIEW_H
#define RENDER_VIEW_H

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Math/Frustum.h"
#include "Render/GpuTypes.h"

// -------------------------------------------------------------------------
// RenderView — everything the renderer needs to render from one point of view.
//
// A frame can have multiple RenderViews:
//   • The main camera               (primary colour output)
//   • One per shadow-casting light  (depth-only, feeds shadow maps)
//   • Planar reflections, portals, etc.
//
// The renderer treats each view independently — Submit() / RenderView() can
// be called in any combination without coupling.
// -------------------------------------------------------------------------
struct RenderView
{
    // ---- Matrices -------------------------------------------------------
    Matrix4x4 View;            // world → camera space
    Matrix4x4 Projection;      // camera → clip space
    Matrix4x4 ViewProjection;  // precomputed View * Projection

    // ---- Camera state ---------------------------------------------------
    Vector3 Position;          // camera world-space position
                               // — used for sort-key depth and lighting calcs
    float   NearZ = 0.1f;      // stored for depth linearisation in shaders
    float   FarZ  = 1000.f;    // and for future shadow cascade splits

    // ---- Culling --------------------------------------------------------
    Frustum ViewFrustum;       // extracted from ViewProjection

    // ---- Output ---------------------------------------------------------
    Viewport       Viewport;   // region of the render target to write into
    GpuFramebuffer Target;     // framebuffer to render into
                               // — invalid handle means render to the swapchain
};

// -------------------------------------------------------------------------
// MakeRenderView
//
// Convenience constructor.  Pass in the camera matrices; fills in
// ViewProjection and extracts the frustum automatically.
//
// Usage:
//   auto view = MakeRenderView(
//       eye, Matrix4x4::LookAt(eye, target, up),
//       Matrix4x4::Perspective(60.f, aspect, 0.1f, 1000.f),
//       vp, fb, 0.1f, 1000.f);
// -------------------------------------------------------------------------
inline RenderView MakeRenderView(
    const Vector3&    position,
    const Matrix4x4&  view,
    const Matrix4x4&  projection,
    const Viewport&   viewport,
    GpuFramebuffer    target,
    float             nearZ,
    float             farZ)
{
    RenderView rv;
    rv.Position       = position;
    rv.View           = view;
    rv.Projection     = projection;
    rv.ViewProjection = view * projection;
    rv.NearZ          = nearZ;
    rv.FarZ           = farZ;
    rv.Viewport       = viewport;
    rv.Target         = target;
    rv.ViewFrustum    = ExtractFrustum(rv.ViewProjection);
    return rv;
}

#endif // RENDER_VIEW_H
