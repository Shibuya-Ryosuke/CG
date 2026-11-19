#pragma once
#include "Math.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace RyoEngine {

    /// <summary>
    /// クォータニオン (回転を表す4つの数)。
    /// ・回転の角度はすべて度数法で受け取る・返す
    /// ・回転の向きは MakeRotateXMatrix などと同じ (行列に変換すると、行ベクトル規約 v * M でそのまま使える)
    /// ・q1 * q2 は「q2 で回してから q1 で回す」(右から順に効く)
    /// </summary>
    struct Quaternion {
        float x;
        float y;
        float z;
        float w;

        // 4つの値を必ず渡して作る。{ x, y, z } のような3つの値の波かっこが Vector3 と Quaternion の
        // どちらにもなれてしまい、Normalize({...}) などの呼び出しがあいまいになるのを防ぐため
        Quaternion() = default;
        constexpr Quaternion(float qx, float qy, float qz, float qw) : x(qx), y(qy), z(qz), w(qw) {}
    };

    // ================================================================
    // 角度の変換 (度数法 ⇔ 弧度法)
    // ================================================================
    inline float DegToRad(float degree) { return degree * (std::numbers::pi_v<float> / 180.0f); }
    inline float RadToDeg(float radian) { return radian * (180.0f / std::numbers::pi_v<float>); }

    // ================================================================
    // 基本
    // ================================================================
    /// 単位クォータニオン (回転なし)
    inline Quaternion IdentityQuaternion() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

    /// 掛け算 (lhs * rhs = 「rhs で回してから lhs で回す」)
    inline Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
        return {
            lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
            lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
        };
    }
    inline Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs) { return Multiply(lhs, rhs); }
    inline Quaternion operator*(const Quaternion& q, float s) { return { q.x * s, q.y * s, q.z * s, q.w * s }; }
    inline Quaternion operator*(float s, const Quaternion& q) { return q * s; }
    inline Quaternion operator+(const Quaternion& a, const Quaternion& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
    inline Quaternion operator-(const Quaternion& a, const Quaternion& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
    inline Quaternion operator-(const Quaternion& q) { return { -q.x, -q.y, -q.z, -q.w }; }

    /// 共役 (回転の向きを逆にしたもの。長さ1なら Inverse と同じ)
    inline Quaternion Conjugate(const Quaternion& q) { return { -q.x, -q.y, -q.z, q.w }; }

    /// 内積
    inline float Dot(const Quaternion& a, const Quaternion& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

    /// ノルム (長さ)
    inline float Norm(const Quaternion& q) { return std::sqrt(Dot(q, q)); }

    /// 正規化 (長さを1にする。長さ0のときは単位クォータニオンを返す)
    inline Quaternion Normalize(const Quaternion& q) {
        const float norm = Norm(q);
        if (norm <= 0.0f) {
            return IdentityQuaternion();
        }
        return q * (1.0f / norm);
    }

    /// 逆クォータニオン (長さ0のときは単位クォータニオンを返す)
    inline Quaternion Inverse(const Quaternion& q) {
        const float normSq = Dot(q, q);
        if (normSq <= 0.0f) {
            return IdentityQuaternion();
        }
        return Conjugate(q) * (1.0f / normSq);
    }

    // ================================================================
    // 作る
    // ================================================================
    /// 軸と角度(度)から作る (軸は正規化しなくてよい。長さ0なら回転なし)
    inline Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angleDeg) {
        const Vector3 n = Normalize(axis);
        if (Dot(n, n) <= 0.0f) {
            return IdentityQuaternion();
        }
        const float half = DegToRad(angleDeg) * 0.5f;
        const float s = std::sin(half);
        return { n.x * s, n.y * s, n.z * s, std::cos(half) };
    }

    /// オイラー角(度)から作る。MakeRotateMatrix / MakeAffineMatrix と同じ順番 (X → Y → Z の順に回す)
    inline Quaternion MakeQuaternionFromEuler(const Vector3& rotateDeg) {
        const Quaternion qx = MakeRotateAxisAngleQuaternion({ 1.0f, 0.0f, 0.0f }, rotateDeg.x);
        const Quaternion qy = MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, rotateDeg.y);
        const Quaternion qz = MakeRotateAxisAngleQuaternion({ 0.0f, 0.0f, 1.0f }, rotateDeg.z);
        // 右から順に効くので、X で回して → Y → Z
        return qz * qy * qx;
    }

    /// from の向きを to の向きへ回すクォータニオン (どちらも正規化しなくてよい)
    inline Quaternion MakeRotateFromToQuaternion(const Vector3& from, const Vector3& to) {
        const Vector3 f = Normalize(from);
        const Vector3 t = Normalize(to);
        const float cosTheta = Dot(f, t);
        if (cosTheta >= 1.0f - 1e-6f) {
            // 同じ向き
            return IdentityQuaternion();
        }
        if (cosTheta <= -1.0f + 1e-6f) {
            // 真逆の向き: from に垂直な軸なら、どれで180度回してもよい
            Vector3 axis = Cross({ 1.0f, 0.0f, 0.0f }, f);
            if (Dot(axis, axis) < 1e-6f) {
                axis = Cross({ 0.0f, 1.0f, 0.0f }, f);
            }
            return MakeRotateAxisAngleQuaternion(axis, 180.0f);
        }
        const Vector3 axis = Cross(f, t);
        return Normalize(Quaternion{ axis.x, axis.y, axis.z, 1.0f + cosTheta });
    }

    /// 前方向(forward)と上方向(up)から、その向きを向くクォータニオンを作る (+Z を forward に、+Y をおおよそ up に向ける)
    inline Quaternion MakeLookRotation(const Vector3& forward, const Vector3& up = { 0.0f, 1.0f, 0.0f });

    // ================================================================
    // 回す・変換する
    // ================================================================
    /// ベクトルを回す
    inline Vector3 RotateVector(const Vector3& vector, const Quaternion& q) {
        const Quaternion v = { vector.x, vector.y, vector.z, 0.0f };
        const Quaternion r = q * v * Conjugate(q);
        return { r.x, r.y, r.z };
    }

    /// 回転行列にする (行ベクトル規約。MakeRotateMatrix と同じように使える)
    inline Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
        const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z, ww = q.w * q.w;
        const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
        Matrix4x4 m{};
        m.m[0][0] = ww + xx - yy - zz;
        m.m[0][1] = 2.0f * (xy + wz);
        m.m[0][2] = 2.0f * (xz - wy);
        m.m[1][0] = 2.0f * (xy - wz);
        m.m[1][1] = ww - xx + yy - zz;
        m.m[1][2] = 2.0f * (yz + wx);
        m.m[2][0] = 2.0f * (xz + wy);
        m.m[2][1] = 2.0f * (yz - wx);
        m.m[2][2] = ww - xx - yy + zz;
        m.m[3][3] = 1.0f;
        return m;
    }

    /// 回転行列 (スケールが入っていないもの) からクォータニオンを作る
    inline Quaternion MakeQuaternionFromRotateMatrix(const Matrix4x4& m) {
        // MakeRotateMatrix(Quaternion) の逆。一番大きい成分から求めて、誤差を小さくする
        const float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
        Quaternion q{};
        if (trace > 0.0f) {
            const float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4w
            q.w = 0.25f * s;
            q.x = (m.m[1][2] - m.m[2][1]) / s;
            q.y = (m.m[2][0] - m.m[0][2]) / s;
            q.z = (m.m[0][1] - m.m[1][0]) / s;
        } else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
            const float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f; // s = 4x
            q.w = (m.m[1][2] - m.m[2][1]) / s;
            q.x = 0.25f * s;
            q.y = (m.m[1][0] + m.m[0][1]) / s;
            q.z = (m.m[2][0] + m.m[0][2]) / s;
        } else if (m.m[1][1] > m.m[2][2]) {
            const float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f; // s = 4y
            q.w = (m.m[2][0] - m.m[0][2]) / s;
            q.x = (m.m[1][0] + m.m[0][1]) / s;
            q.y = 0.25f * s;
            q.z = (m.m[2][1] + m.m[1][2]) / s;
        } else {
            const float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f; // s = 4z
            q.w = (m.m[0][1] - m.m[1][0]) / s;
            q.x = (m.m[2][0] + m.m[0][2]) / s;
            q.y = (m.m[2][1] + m.m[1][2]) / s;
            q.z = 0.25f * s;
        }
        return Normalize(q);
    }

    /// オイラー角(度)にする。MakeQuaternionFromEuler の逆 (X → Y → Z の順)。
    /// Y が ±90度付近のときは X と Z が区別できないので、Z を0にして X に寄せる
    inline Vector3 QuaternionToEuler(const Quaternion& q) {
        const Matrix4x4 m = MakeRotateMatrix(Normalize(q));
        const float sinY = Clamp(-m.m[0][2], -1.0f, 1.0f);
        Vector3 result{};
        result.y = std::asin(sinY);
        if (std::abs(sinY) < 0.9999f) {
            result.x = std::atan2(m.m[1][2], m.m[2][2]);
            result.z = std::atan2(m.m[0][1], m.m[0][0]);
        } else {
            result.x = std::atan2(-m.m[2][1], m.m[1][1]);
            result.z = 0.0f;
        }
        return { RadToDeg(result.x), RadToDeg(result.y), RadToDeg(result.z) };
    }

    /// 回転軸と角度(度)を取り出す (回転なしのときは軸 {0,1,0}・角度0)
    inline void QuaternionToAxisAngle(const Quaternion& q, Vector3& outAxis, float& outAngleDeg) {
        Quaternion n = Normalize(q);
        if (n.w < 0.0f) {
            // 同じ回転を、角度が0〜180度になる方で表す
            n = -n;
        }
        const float sinHalf = std::sqrt((std::max)(1.0f - n.w * n.w, 0.0f));
        outAngleDeg = RadToDeg(2.0f * std::acos(Clamp(n.w, -1.0f, 1.0f)));
        if (sinHalf < 1e-6f) {
            outAxis = { 0.0f, 1.0f, 0.0f };
            outAngleDeg = 0.0f;
            return;
        }
        outAxis = { n.x / sinHalf, n.y / sinHalf, n.z / sinHalf };
    }

    /// 2つの回転の間の角度(度。0〜180)
    inline float AngleBetween(const Quaternion& a, const Quaternion& b) {
        // a から b への差の回転を求めて、その角度を測る (acos より 0度付近の誤差が小さい)
        const Quaternion diff = Conjugate(Normalize(a)) * Normalize(b);
        const float sinHalf = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        return RadToDeg(2.0f * std::atan2(sinHalf, std::abs(diff.w)));
    }

    // ================================================================
    // 補間
    // ================================================================
    /// 球面線形補間 (t=0 で q0、t=1 で q1。いつも近い方の回り方で補間する)
    inline Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
        Quaternion a = Normalize(q0);
        Quaternion b = Normalize(q1);
        float dot = Dot(a, b);
        if (dot < 0.0f) {
            // 遠回りしないように、片方を反転する (同じ回転を表す)
            b = -b;
            dot = -dot;
        }
        if (dot >= 1.0f - 1e-5f) {
            // ほぼ同じ向きのときは、0で割らないように線形補間で済ませる
            return Normalize(a * (1.0f - t) + b * t);
        }
        const float theta = std::acos(dot);
        const float sinTheta = std::sin(theta);
        const float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
        const float scale1 = std::sin(t * theta) / sinTheta;
        return a * scale0 + b * scale1;
    }

    /// 正規化線形補間 (Slerp より軽い。補間の速さが少しだけ一定でない)
    inline Quaternion Nlerp(const Quaternion& q0, const Quaternion& q1, float t) {
        Quaternion b = q1;
        if (Dot(q0, q1) < 0.0f) {
            b = -b;
        }
        return Normalize(q0 * (1.0f - t) + b * t);
    }

    /// current から target へ、最大 maxDeltaDeg 度だけ近づける (毎フレーム一定の速さで振り向くとき用)
    inline Quaternion RotateTowards(const Quaternion& current, const Quaternion& target, float maxDeltaDeg) {
        const float angle = AngleBetween(current, target);
        if (angle <= maxDeltaDeg || angle <= 0.0f) {
            return Normalize(target);
        }
        return Slerp(current, target, maxDeltaDeg / angle);
    }

    // ================================================================
    // 向き
    // ================================================================
    inline Quaternion MakeLookRotation(const Vector3& forward, const Vector3& up) {
        Vector3 zAxis = Normalize(forward);
        if (Dot(zAxis, zAxis) <= 0.0f) {
            return IdentityQuaternion();
        }
        Vector3 xAxis = Normalize(Cross(up, zAxis));
        if (Dot(xAxis, xAxis) <= 0.0f) {
            // forward と up が平行なときは、別の上方向を使う
            xAxis = Normalize(Cross(std::abs(zAxis.y) < 0.99f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f }, zAxis));
        }
        const Vector3 yAxis = Cross(zAxis, xAxis);
        // 行ベクトル規約の回転行列は、各行がその軸の向き
        Matrix4x4 m = MakeIdentity4x4();
        m.m[0][0] = xAxis.x; m.m[0][1] = xAxis.y; m.m[0][2] = xAxis.z;
        m.m[1][0] = yAxis.x; m.m[1][1] = yAxis.y; m.m[1][2] = yAxis.z;
        m.m[2][0] = zAxis.x; m.m[2][1] = zAxis.y; m.m[2][2] = zAxis.z;
        return MakeQuaternionFromRotateMatrix(m);
    }

    /// 回転後の前方向 (+Z)・上方向 (+Y)・右方向 (+X)
    inline Vector3 GetForward(const Quaternion& q) { return RotateVector({ 0.0f, 0.0f, 1.0f }, q); }
    inline Vector3 GetUp(const Quaternion& q) { return RotateVector({ 0.0f, 1.0f, 0.0f }, q); }
    inline Vector3 GetRight(const Quaternion& q) { return RotateVector({ 1.0f, 0.0f, 0.0f }, q); }

    /// SRT からアフィン行列を作る (回転がクォータニオン版の MakeAffineMatrix)
    inline Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
        return MakeScaleMatrix(scale) * MakeRotateMatrix(rotate) * MakeTranslateMatrix(translate);
    }
}
