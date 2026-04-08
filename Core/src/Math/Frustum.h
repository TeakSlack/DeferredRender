#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <cmath>
#include "Vector3.h"
#include "Matrix4x4.h"

// -------------------------------------------------------------------------
// Plane — a half-space defined by a unit normal and a signed distance.
//
// The plane equation is:  dot(Normal, P) + Distance >= 0
// Points satisfying this are on the "positive" (inside) side.
// -------------------------------------------------------------------------
struct Plane
{
    Vector3 Normal;
    float   Distance = 0.f;

    // Signed distance from the plane to point P.
    // Positive = inside (positive half-space), negative = outside.
    float SignedDistance(const Vector3& p) const
    {
        return Vector3::Dot(Normal, p) + Distance;
    }
};

// -------------------------------------------------------------------------
// Frustum — six half-space planes enclosing the view volume.
//
// Planes are in world space, extracted from the view-projection matrix.
// Normals point inward so positive signed distance == inside the frustum.
//
// Plane order: Left, Right, Bottom, Top, Near, Far.
// -------------------------------------------------------------------------
struct Frustum
{
    Plane Planes[6];

    enum { Left = 0, Right, Bottom, Top, Near, Far };

    // Returns true if the AABB [min, max] intersects or is inside the frustum.
    // Uses the p-vertex test (conservative — no false negatives, rare false positives).
    bool IntersectsAABB(const Vector3& min, const Vector3& max) const
    {
        for (const Plane& plane : Planes)
        {
            // P-vertex: the AABB corner most in the direction of the plane normal.
            // If this corner is outside (negative signed distance), the whole box is.
            Vector3 pVertex(
                plane.Normal.x >= 0.f ? max.x : min.x,
                plane.Normal.y >= 0.f ? max.y : min.y,
                plane.Normal.z >= 0.f ? max.z : min.z);

            if (plane.SignedDistance(pVertex) < 0.f)
                return false;
        }
        return true;
    }
};

// -------------------------------------------------------------------------
// ExtractFrustum
//
// Derives the six world-space frustum planes from a combined VP matrix.
// Planes are normalised so SignedDistance returns true world-space metres.
//
// Math — row-vector convention (clip = worldPos * VP):
//
//   clip[j] = worldPos · Col(j)
//
// The six NDC visibility conditions (RH, [0,1] depth for D3D12/Vulkan):
//
//   Left:   clip.w + clip.x >= 0   →  worldPos · (Col3 + Col0) >= 0
//   Right:  clip.w - clip.x >= 0   →  worldPos · (Col3 - Col0) >= 0
//   Bottom: clip.w + clip.y >= 0   →  worldPos · (Col3 + Col1) >= 0
//   Top:    clip.w - clip.y >= 0   →  worldPos · (Col3 - Col2) >= 0
//   Near:   clip.z         >= 0   →  worldPos · Col2            >= 0
//   Far:    clip.w - clip.z >= 0   →  worldPos · (Col3 - Col2) >= 0
//
// Each condition maps to a plane (a,b,c,d) where:
//   a = VP[0][colA] (± VP[0][colB])
//   b = VP[1][colA] (± VP[1][colB])
//   c = VP[2][colA] (± VP[2][colB])
//   d = VP[3][colA] (± VP[3][colB])
// -------------------------------------------------------------------------
inline Frustum ExtractFrustum(const Matrix4x4& vp)
{
    // Helper: build and normalise a plane from two columns of VP.
    // plane = Col(a) + sign * Col(b)
    auto MakePlane = [&](int colA, int colB, float sign) -> Plane
    {
        Plane p;
        p.Normal.x = vp[0][colA] + sign * vp[0][colB];
        p.Normal.y = vp[1][colA] + sign * vp[1][colB];
        p.Normal.z = vp[2][colA] + sign * vp[2][colB];
        p.Distance = vp[3][colA] + sign * vp[3][colB];

        float len = p.Normal.Magnitude();
        if (len > 1e-6f)
        {
            p.Normal.x /= len;
            p.Normal.y /= len;
            p.Normal.z /= len;
            p.Distance  /= len;
        }
        return p;
    };

    // Single-column variant for the Near plane (Col2 only)
    auto MakePlaneSingle = [&](int col) -> Plane
    {
        Plane p;
        p.Normal.x = vp[0][col];
        p.Normal.y = vp[1][col];
        p.Normal.z = vp[2][col];
        p.Distance = vp[3][col];

        float len = p.Normal.Magnitude();
        if (len > 1e-6f)
        {
            p.Normal.x /= len;
            p.Normal.y /= len;
            p.Normal.z /= len;
            p.Distance  /= len;
        }
        return p;
    };

    Frustum f;
    f.Planes[Frustum::Left]   = MakePlane(3, 0, +1.f);  // Col3 + Col0
    f.Planes[Frustum::Right]  = MakePlane(3, 0, -1.f);  // Col3 - Col0
    f.Planes[Frustum::Bottom] = MakePlane(3, 1, +1.f);  // Col3 + Col1
    f.Planes[Frustum::Top]    = MakePlane(3, 1, -1.f);  // Col3 - Col1
    f.Planes[Frustum::Near]   = MakePlaneSingle(2);      // Col2
    f.Planes[Frustum::Far]    = MakePlane(3, 2, -1.f);  // Col3 - Col2
    return f;
}

#endif // FRUSTUM_H
