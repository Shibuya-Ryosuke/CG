#pragma once
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../Math/Math.h"
#include "../Math/Quaternion.h"

namespace RyoEngine {

    // =====================================================================
    // アニメーション・骨格のデータ (読み込んだ後は変わらないもの)
    // ModelLoader が作り、ModelManager が持ち、Animator が読む。
    // 座標系はエンジンと同じ (読み込み時に X を反転済み)。回転はクォータニオン。
    // =====================================================================

    // ---------------------------------------------------------------------
    // キーフレーム
    // ---------------------------------------------------------------------
    template <typename T>
    struct Keyframe {
        float time; // 秒
        T value;
    };

    template <typename T>
    struct AnimationCurve {
        std::vector<Keyframe<T>> keyframes; // time の小さい順
    };

    /// <summary>1つのノード(ボーン)の動き。キーが無い要素は、元の姿勢の値のまま</summary>
    struct NodeAnimation {
        AnimationCurve<Vector3> translate;
        AnimationCurve<Quaternion> rotate;
        AnimationCurve<Vector3> scale;
    };

    /// <summary>1つのアニメーション (歩き・走りなど)</summary>
    struct AnimationData {
        std::string name;
        float duration = 0.0f; // 秒
        // ノード(ボーン)の名前 → 動き
        std::unordered_map<std::string, NodeAnimation> nodeAnimations;
    };

    // ---------------------------------------------------------------------
    // 骨格
    // ---------------------------------------------------------------------
    /// <summary>ボーン1本 (glTF のノード1つ)。元の姿勢 (アニメーションしていないとき) の、親から見た SRT を持つ</summary>
    struct JointData {
        std::string name;
        int32_t parent = -1; // 親の番号 (-1 = 一番上)。親は必ず自分より前の番号にある
        Vector3 translate = { 0.0f, 0.0f, 0.0f };
        Quaternion rotate = { 0.0f, 0.0f, 0.0f, 1.0f };
        Vector3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct SkeletonData {
        std::vector<JointData> joints;
        std::unordered_map<std::string, int32_t> jointMap; // 名前 → 番号

        /// <summary>名前からボーンの番号を探す (無ければ -1)</summary>
        int32_t FindJoint(const std::string& name) const {
            auto it = jointMap.find(name);
            return it != jointMap.end() ? it->second : -1;
        }
    };

    /// <summary>メッシュがついていくボーン1本分 (スキニングの行列パレット1要素の元)</summary>
    struct SkinBone {
        int32_t jointIndex = 0;                         // 骨格のボーンの番号
        Matrix4x4 inverseBindMatrix = MakeIdentity4x4(); // 元の姿勢でのボーンの行列の逆 (メッシュの座標 → ボーンから見た座標)
    };

    /// <summary>頂点1つ分の、ついていくボーンと割合 (最大4本。合計が1になるようにしてある)</summary>
    struct VertexInfluence {
        float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        uint32_t indices[4] = { 0, 0, 0, 0 }; // メッシュの SkinBone 配列の番号
    };

    // ---------------------------------------------------------------------
    // モデルに結びつけたアニメーション
    // ---------------------------------------------------------------------
    /// <summary>アニメーションの1本の動きを、モデルのどのボーンに当てるか</summary>
    struct AnimationChannelBinding {
        int32_t jointIndex = 0;
        const NodeAnimation* animation = nullptr;
        bool useTranslate = true;   // false: 位置はモデル自身の元の姿勢のまま (ほかの骨格のアニメーションで、骨の長さを変えないため)
        bool useScale = true;
        float translateScale = 1.0f; // 位置にかける倍率 (ほかの骨格のアニメーションで、腰の移動量を体格に合わせるため)
    };

    /// <summary>モデルに登録されたアニメーション (名前で再生する単位)</summary>
    struct SkeletalAnimation {
        // unique_ptr を持つのでコピーは不可、ムーブだけ可 (明示しないと /Wall で C4625 / C4626 が出る)
        SkeletalAnimation() = default;
        SkeletalAnimation(const SkeletalAnimation&) = delete;
        SkeletalAnimation& operator=(const SkeletalAnimation&) = delete;
        SkeletalAnimation(SkeletalAnimation&&) = default;
        SkeletalAnimation& operator=(SkeletalAnimation&&) = default;

        std::string name;
        float duration = 0.0f;
        std::unique_ptr<AnimationData> data; // channels が中を指すので、アドレスが変わらないよう unique_ptr で持つ
        std::vector<AnimationChannelBinding> channels;
    };

    // ---------------------------------------------------------------------
    // 計算
    // ---------------------------------------------------------------------
    /// <summary>キーフレームの間を線形補間して値を求める (キーが無ければ fallback)</summary>
    inline Vector3 SampleCurve(const AnimationCurve<Vector3>& curve, float time, const Vector3& fallback) {
        const auto& keys = curve.keyframes;
        if (keys.empty()) return fallback;
        if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
        if (time >= keys.back().time) return keys.back().value;
        // time より後ろにある最初のキー
        auto next = std::upper_bound(keys.begin(), keys.end(), time,
            [](float t, const Keyframe<Vector3>& key) { return t < key.time; });
        auto prev = next - 1;
        const float span = next->time - prev->time;
        const float t = span > 0.0f ? (time - prev->time) / span : 0.0f;
        return Lerp(prev->value, next->value, t);
    }

    /// <summary>キーフレームの間を球面線形補間して回転を求める (キーが無ければ fallback)</summary>
    inline Quaternion SampleCurve(const AnimationCurve<Quaternion>& curve, float time, const Quaternion& fallback) {
        const auto& keys = curve.keyframes;
        if (keys.empty()) return fallback;
        if (keys.size() == 1 || time <= keys.front().time) return keys.front().value;
        if (time >= keys.back().time) return keys.back().value;
        auto next = std::upper_bound(keys.begin(), keys.end(), time,
            [](float t, const Keyframe<Quaternion>& key) { return t < key.time; });
        auto prev = next - 1;
        const float span = next->time - prev->time;
        const float t = span > 0.0f ? (time - prev->time) / span : 0.0f;
        return Slerp(prev->value, next->value, t);
    }

    /// <summary>元の姿勢での、各ボーンの行列 (モデルの原点から見た座標。親の行列を全部かけたもの)</summary>
    inline std::vector<Matrix4x4> ComputeBindPoseMatrices(const SkeletonData& skeleton) {
        std::vector<Matrix4x4> matrices(skeleton.joints.size());
        for (size_t i = 0; i < skeleton.joints.size(); ++i) {
            const JointData& joint = skeleton.joints[i];
            const Matrix4x4 local = MakeAffineMatrix(joint.scale, joint.rotate, joint.translate);
            // 親は必ず自分より前にあるので、前から順に計算すればよい
            matrices[i] = joint.parent >= 0 ? local * matrices[joint.parent] : local;
        }
        return matrices;
    }
}
