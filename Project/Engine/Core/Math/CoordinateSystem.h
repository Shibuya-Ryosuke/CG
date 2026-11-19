#pragma once
#include "Math.h"
#include "Quaternion.h"
#include <cmath>

namespace RyoEngine {

    // =====================================================================
    // いろいろな座標系と、直交座標 (x, y, z) との変換。角度はすべて度数法。
    // 上方向は +Y。「水平面」は XZ 平面。
    // =====================================================================

    // ---------------------------------------------------------------------
    // 極座標 (2D): 中心からの距離と角度
    // ---------------------------------------------------------------------
    struct PolarCoordinate {
        float radius;   // 中心からの距離
        float angleDeg; // +X から +Y へ向かって測った角度 (度)
    };

    inline Vector2 PolarToCartesian(const PolarCoordinate& polar) {
        const float a = DegToRad(polar.angleDeg);
        return { polar.radius * std::cos(a), polar.radius * std::sin(a) };
    }
    inline PolarCoordinate CartesianToPolar(const Vector2& p) {
        return { std::sqrt(p.x * p.x + p.y * p.y), RadToDeg(std::atan2(p.y, p.x)) };
    }

    // ---------------------------------------------------------------------
    // 円柱座標 (3D): 水平面での距離と角度 + 高さ
    // ---------------------------------------------------------------------
    struct CylindricalCoordinate {
        float radius;   // Y軸からの水平距離
        float angleDeg; // +X から +Z へ向かって測った角度 (度)
        float height;   // Y座標
    };

    inline Vector3 CylindricalToCartesian(const CylindricalCoordinate& c) {
        const float a = DegToRad(c.angleDeg);
        return { c.radius * std::cos(a), c.height, c.radius * std::sin(a) };
    }
    inline CylindricalCoordinate CartesianToCylindrical(const Vector3& p) {
        return { std::sqrt(p.x * p.x + p.z * p.z), RadToDeg(std::atan2(p.z, p.x)), p.y };
    }

    // ---------------------------------------------------------------------
    // 球面座標 (3D): 原点からの距離と、2つの角度
    // ---------------------------------------------------------------------
    struct SphericalCoordinate {
        float radius;     // 原点からの距離
        float polarDeg;   // 天頂角 θ: +Y (真上) から測った角度 (0 = 真上, 90 = 水平, 180 = 真下)
        float azimuthDeg; // 方位角 φ: +X から +Z へ向かって測った水平の角度
    };

    inline Vector3 SphericalToCartesian(const SphericalCoordinate& s) {
        const float theta = DegToRad(s.polarDeg);
        const float phi = DegToRad(s.azimuthDeg);
        return {
            s.radius * std::sin(theta) * std::cos(phi),
            s.radius * std::cos(theta),
            s.radius * std::sin(theta) * std::sin(phi),
        };
    }
    inline SphericalCoordinate CartesianToSpherical(const Vector3& p) {
        const float radius = Length(p);
        if (radius <= 0.0f) {
            return { 0.0f, 0.0f, 0.0f };
        }
        return {
            radius,
            RadToDeg(std::acos(Clamp(p.y / radius, -1.0f, 1.0f))),
            RadToDeg(std::atan2(p.z, p.x)),
        };
    }

    // ---------------------------------------------------------------------
    // 重心座標: 三角形の3頂点の重み (u, v, w)。u + v + w = 1
    // 点 = 頂点0 * u + 頂点1 * v + 頂点2 * w
    // 3つとも 0 以上なら、その点は三角形の中にある
    // ---------------------------------------------------------------------
    /// 点の重心座標を求める (点は三角形の平面上にあるものとして扱う。平面から外れていたら平面に落とした位置で求める)
    inline Vector3 CartesianToBarycentric(const Vector3& point, const Triangle& triangle) {
        const Vector3 v0 = triangle.vertices[1] - triangle.vertices[0];
        const Vector3 v1 = triangle.vertices[2] - triangle.vertices[0];
        const Vector3 v2 = point - triangle.vertices[0];
        const float d00 = Dot(v0, v0);
        const float d01 = Dot(v0, v1);
        const float d11 = Dot(v1, v1);
        const float d20 = Dot(v2, v0);
        const float d21 = Dot(v2, v1);
        const float denom = d00 * d11 - d01 * d01;
        if (std::abs(denom) < 1e-12f) {
            // つぶれた三角形
            return { 1.0f, 0.0f, 0.0f };
        }
        const float v = (d11 * d20 - d01 * d21) / denom;
        const float w = (d00 * d21 - d01 * d20) / denom;
        return { 1.0f - v - w, v, w };
    }
    /// 重心座標から点を求める
    inline Vector3 BarycentricToCartesian(const Vector3& weights, const Triangle& triangle) {
        return triangle.vertices[0] * weights.x + triangle.vertices[1] * weights.y + triangle.vertices[2] * weights.z;
    }
    /// 重心座標が三角形の中を指しているか
    inline bool IsInsideTriangle(const Vector3& weights) {
        return weights.x >= 0.0f && weights.y >= 0.0f && weights.z >= 0.0f;
    }

    // ---------------------------------------------------------------------
    // フレネ標構: 曲線に沿って動くものの「進む向き・曲がる向き・それに垂直な向き」
    // (レールに沿って進むカメラ・ジェットコースター・ミサイルの向きなど)
    // ---------------------------------------------------------------------
    struct FrenetFrame {
        Vector3 tangent;  // 接線 T: 進む向き
        Vector3 normal;   // 主法線 N: 曲がっていく向き (カーブの内側)
        Vector3 binormal; // 従法線 B = T × N
    };

    /// <summary>
    /// 速度と加速度からフレネ標構を求める。
    /// まっすぐ進んでいて曲がる向きが決まらないときは、fallbackUp を使って向きを決める
    /// </summary>
    inline FrenetFrame ComputeFrenetFrame(const Vector3& velocity, const Vector3& acceleration,
        const Vector3& fallbackUp = { 0.0f, 1.0f, 0.0f }) {
        FrenetFrame frame{};
        frame.tangent = Normalize(velocity);
        if (Dot(frame.tangent, frame.tangent) <= 0.0f) {
            frame.tangent = { 0.0f, 0.0f, 1.0f };
        }
        // 加速度のうち、進む向きに垂直な成分が曲がる向き
        Vector3 n = acceleration - frame.tangent * Dot(acceleration, frame.tangent);
        if (Dot(n, n) < 1e-10f) {
            // まっすぐ: fallbackUp と直角になる向きにしておく
            n = Cross(frame.tangent, Cross(fallbackUp, frame.tangent));
            if (Dot(n, n) < 1e-10f) {
                n = Cross(frame.tangent, Cross(Vector3{ 1.0f, 0.0f, 0.0f }, frame.tangent));
            }
        }
        frame.normal = Normalize(n);
        frame.binormal = Cross(frame.tangent, frame.normal);
        return frame;
    }

    /// <summary>
    /// 曲線上の連続した3点 (1つ前・今・1つ後) からフレネ標構を求める (曲線の式が無いとき用)
    /// </summary>
    inline FrenetFrame ComputeFrenetFrameFromPoints(const Vector3& previous, const Vector3& current, const Vector3& next,
        const Vector3& fallbackUp = { 0.0f, 1.0f, 0.0f }) {
        // 差分で 速度 ≒ (next - previous) / 2、加速度 ≒ next - 2*current + previous とみなす
        const Vector3 velocity = (next - previous) * 0.5f;
        const Vector3 acceleration = next - current * 2.0f + previous;
        return ComputeFrenetFrame(velocity, acceleration, fallbackUp);
    }

    /// <summary>
    /// フレネ標構をワールド行列にする (+Z が進む向き、+Y が曲がる向き(主法線) を向く)。
    /// position を入れると、その位置に置く行列になる
    /// </summary>
    inline Matrix4x4 MakeFrenetMatrix(const FrenetFrame& frame, const Vector3& position = { 0.0f, 0.0f, 0.0f }) {
        const Vector3 xAxis = Cross(frame.normal, frame.tangent); // = -binormal (右手系のまま回転行列にするため)
        Matrix4x4 m = MakeIdentity4x4();
        m.m[0][0] = xAxis.x;          m.m[0][1] = xAxis.y;          m.m[0][2] = xAxis.z;
        m.m[1][0] = frame.normal.x;   m.m[1][1] = frame.normal.y;   m.m[1][2] = frame.normal.z;
        m.m[2][0] = frame.tangent.x;  m.m[2][1] = frame.tangent.y;  m.m[2][2] = frame.tangent.z;
        m.m[3][0] = position.x;       m.m[3][1] = position.y;       m.m[3][2] = position.z;
        return m;
    }

    // ---------------------------------------------------------------------
    // 振り子: 支点からひもでぶら下がって、1つの平面 (支点を通る XY 平面) で揺れる
    // ---------------------------------------------------------------------
    struct Pendulum {
        Vector3 anchor = { 0.0f, 0.0f, 0.0f }; // 支点
        float length = 1.0f;                   // ひもの長さ
        float angleDeg = 30.0f;                // 真下からの角度 (+ で +X 側へ振れる)
        float angularVelocityDeg = 0.0f;       // 角速度 (度/秒)
    };

    /// 振り子を deltaTime 秒進める (gravity: 重力加速度の大きさ)
    inline void UpdatePendulum(Pendulum& pendulum, float deltaTime, float gravity = 9.8f) {
        if (pendulum.length <= 0.0f) {
            return;
        }
        // 角加速度 = -(g / L) * sinθ (弧度法で計算して、度に戻す)
        const float angularAcceleration = -(gravity / pendulum.length) * std::sin(DegToRad(pendulum.angleDeg));
        pendulum.angularVelocityDeg += RadToDeg(angularAcceleration) * deltaTime;
        pendulum.angleDeg += pendulum.angularVelocityDeg * deltaTime;
    }

    /// 振り子のおもりの位置
    inline Vector3 GetPendulumPosition(const Pendulum& pendulum) {
        const float a = DegToRad(pendulum.angleDeg);
        return pendulum.anchor + Vector3{ std::sin(a) * pendulum.length, -std::cos(a) * pendulum.length, 0.0f };
    }

    // ---------------------------------------------------------------------
    // 円錐振り子: 支点からひもでぶら下がって、水平に円を描いて回る (ひもが円錐の側面をなぞる)
    // ---------------------------------------------------------------------
    struct ConicalPendulum {
        Vector3 anchor = { 0.0f, 0.0f, 0.0f }; // 支点
        float length = 1.0f;                   // ひもの長さ
        float halfApexAngleDeg = 30.0f;        // ひもと真下の間の角度 (円錐の半頂角。0〜90未満)
        float angleDeg = 0.0f;                 // 今の回転角 (+X から +Z へ)
    };

    /// 円錐振り子を deltaTime 秒進める (回る速さは、ひもの長さと角度から重力で決まる)
    inline void UpdateConicalPendulum(ConicalPendulum& pendulum, float deltaTime, float gravity = 9.8f) {
        const float cosAlpha = std::cos(DegToRad(pendulum.halfApexAngleDeg));
        if (pendulum.length <= 0.0f || cosAlpha <= 0.0f) {
            return;
        }
        // 角速度 ω = √(g / (L cosα))
        const float angularVelocity = std::sqrt(gravity / (pendulum.length * cosAlpha));
        pendulum.angleDeg += RadToDeg(angularVelocity) * deltaTime;
        // 角度が大きくなりすぎないように 0〜360 に収める
        pendulum.angleDeg = std::fmod(pendulum.angleDeg, 360.0f);
    }

    /// 円錐振り子のおもりの位置
    inline Vector3 GetConicalPendulumPosition(const ConicalPendulum& pendulum) {
        const float alpha = DegToRad(pendulum.halfApexAngleDeg);
        const float phi = DegToRad(pendulum.angleDeg);
        const float radius = std::sin(alpha) * pendulum.length;
        const float height = std::cos(alpha) * pendulum.length;
        return pendulum.anchor + Vector3{ radius * std::cos(phi), -height, radius * std::sin(phi) };
    }
}
