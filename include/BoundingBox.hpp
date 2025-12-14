#pragma once
#include <DirectXCollision.h>
#include <cmath>

namespace BoundingBox {
    using RE::NiPoint3;

    // You already have this:
    inline RE::bhkRigidBody* GetRigidBody(const RE::TESObjectREFR* refr) {
        const auto object3D = refr->GetCurrent3D();
        if (!object3D) {
            return nullptr;
        }
        if (const auto body = object3D->GetCollisionObject()) {
            return body->GetRigidBody();
        }
        return nullptr;
    }

    // ---- helper: NiMatrix3 -> DirectX quaternion (x,y,z,w) ----
    inline DirectX::XMFLOAT4 MatrixToDXQuaternion(const RE::NiMatrix3& R) {
        // Assuming NiMatrix3::entry[row][col] and R * v uses the usual row-major form:
        // x' = m00*x + m01*y + m02*z, etc.
        const float m00 = R.entry[0][0], m01 = R.entry[0][1], m02 = R.entry[0][2];
        const float m10 = R.entry[1][0], m11 = R.entry[1][1], m12 = R.entry[1][2];
        const float m20 = R.entry[2][0], m21 = R.entry[2][1], m22 = R.entry[2][2];

        float qw, qx, qy, qz;

        const float trace = m00 + m11 + m22;
        if (trace > 0.0f) {
            const float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4*qw
            qw = 0.25f * s;
            qx = (m21 - m12) / s;
            qy = (m02 - m20) / s;
            qz = (m10 - m01) / s;
        } else if (m00 > m11 && m00 > m22) {
            const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f; // s = 4*qx
            qw = (m21 - m12) / s;
            qx = 0.25f * s;
            qy = (m01 + m10) / s;
            qz = (m02 + m20) / s;
        } else if (m11 > m22) {
            const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f; // s = 4*qy
            qw = (m02 - m20) / s;
            qx = (m01 + m10) / s;
            qy = 0.25f * s;
            qz = (m12 + m21) / s;
        } else {
            const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f; // s = 4*qz
            qw = (m10 - m01) / s;
            qx = (m02 + m20) / s;
            qy = (m12 + m21) / s;
            qz = 0.25f * s;
        }

        // Optional: normalize to be safe (should already be very close)
        const float lenSq = qx * qx + qy * qy + qz * qz + qw * qw;
        if (lenSq > 0.0f) {
            const float invLen = 1.0f / std::sqrt(lenSq);
            qx *= invLen;
            qy *= invLen;
            qz *= invLen;
            qw *= invLen;
        }

        // DirectX uses (x,y,z,w)
        return DirectX::XMFLOAT4(qx, qy, qz, qw);
    }

    // ---- helper: build OBB from Havok world AABB (fast path) ----
    inline bool GetOBBFromHavok(const RE::TESObjectREFR* obj, DirectX::BoundingOrientedBox& out) {
        if (const auto body = GetRigidBody(obj)) {
            RE::hkAabb aabb;
            body->GetAabbWorldspace(aabb);

            float minComp[4]{};
            float maxComp[4]{};
            _mm_store_ps(minComp, aabb.min.quad);
            _mm_store_ps(maxComp, aabb.max.quad);

            // Same factor you used before
            constexpr float havokToSkyrim = 69.9915f;

            const NiPoint3 minWorld{minComp[0] * havokToSkyrim, minComp[1] * havokToSkyrim, minComp[2] * havokToSkyrim};
            const NiPoint3 maxWorld{maxComp[0] * havokToSkyrim, maxComp[1] * havokToSkyrim, maxComp[2] * havokToSkyrim};

            const NiPoint3 center = (minWorld + maxWorld) * 0.5f;
            const NiPoint3 half = (maxWorld - minWorld) * 0.5f;

            out.Center = DirectX::XMFLOAT3(center.x, center.y, center.z);
            out.Extents = DirectX::XMFLOAT3(half.x, half.y, half.z);
            // Havok gave us a world AABB, so orientation is identity
            out.Orientation = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);

            return true;
        }
        return false;
    }

    // ---- main: build a world-space OBB for a TESObjectREFR ----
    inline void GetOBB(const RE::TESObjectREFR* obj, DirectX::BoundingOrientedBox& out, const bool allowAABB) {
        // 1) Prefer Havok: cheap, already in world space.
        if (allowAABB && GetOBBFromHavok(obj, out)) {
            return;
        }

        // 2) Fallback: use gameplay bounds + world transform (tight OBB)
        NiPoint3 minLocal = obj->GetBoundMin();
        NiPoint3 maxLocal = obj->GetBoundMax();

        const float scale = obj->GetScale();
        minLocal *= scale;
        maxLocal *= scale;

        RE::NiMatrix3 R;
        NiPoint3 T;

        if (const auto node = obj->GetCurrent3D()) {
            R = node->world.rotate;
            T = node->world.translate;
        } else {
            // If no 3D, approximate from ref's angle/pos
            R.SetEulerAnglesXYZ(obj->GetAngle());
            T = obj->GetPosition();
        }

        const NiPoint3 localCenter = (minLocal + maxLocal) * 0.5f;
        const NiPoint3 halfLocal = (maxLocal - minLocal) * 0.5f;

        const NiPoint3 worldCenter = T + R * localCenter;

        out.Center = DirectX::XMFLOAT3(worldCenter.x, worldCenter.y, worldCenter.z);
        out.Extents = DirectX::XMFLOAT3(halfLocal.x, halfLocal.y, halfLocal.z);
        out.Orientation = MatrixToDXQuaternion(R);
    }
}