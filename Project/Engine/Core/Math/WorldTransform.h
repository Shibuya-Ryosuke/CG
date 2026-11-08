#pragma once
#include <cstdint>
#include <vector>
#include "Math.h"
#include "Transform.h"

namespace RyoEngine {

    /// <summary>
    /// 「親から見たSRT(ローカル)」と、親子関係を考慮した「ワールド行列」を持つクラス。
    /// Model が1つずつ持っている。単体で使って、見た目と関係ない基準点にすることもできる。
    ///
    /// ・world = local × 親のworld (行ベクトル規約)
    /// ・UpdateMatrix() のとき、親がまだこのフレームで計算されていなければ、先に親を計算する
    ///   (呼ぶ順番が子→親になっても、古い親の行列を使わない)
    /// ・親が先に破棄された場合、子は自動で親子解除される (子は local がそのままワールドになる)
    /// ・親のスケールは常に子にも反映される
    /// </summary>
    class WorldTransform {
    public:
        WorldTransform() = default;
        ~WorldTransform();
        // 親子のつながりを持つので、コピー/ムーブは不可
        WorldTransform(const WorldTransform&) = delete;
        WorldTransform& operator=(const WorldTransform&) = delete;

        /// <summary>フレーム番号を進める。RyoEngine::NewFrame から1回だけ呼ばれる (ゲーム側は呼ばない)</summary>
        static void NewFrame();

        /// <summary>
        /// 親を設定する。nullptr で親子解除。
        /// 自分自身や自分の子孫を親にしようとした場合は、警告を出して何もしない。
        /// </summary>
        void SetParent(WorldTransform* parent);
        WorldTransform* GetParent() const { return parent_; }
        const std::vector<WorldTransform*>& GetChildren() const { return children_; }

        /// <summary>親から見たSRT (回転は度数法)。親がいなければワールドでの値と同じ</summary>
        Transform& GetLocal() { return local_; }
        const Transform& GetLocal() const { return local_; }
        void SetLocal(const Transform& local) { local_ = local; }

        /// <summary>
        /// ワールド行列を計算する。親がいる場合、親がこのフレームでまだ計算されていなければ先に計算する。
        /// 同じフレームで何回呼んでもよい (呼ぶたびに自分は計算し直す)。
        /// </summary>
        void UpdateMatrix();

        const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
        Vector3 GetWorldPosition() const { return { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] }; }
        /// <summary>ワールド行列から取り出したスケール (親のスケールも含む)</summary>
        Vector3 GetWorldScale() const;

    private:
        static uint64_t& FrameCounter();

        // 自分の子孫に target が含まれるか (循環した親子を作らないため)
        bool IsDescendant(const WorldTransform* target) const;
        void RemoveChild(WorldTransform* child);

        Transform local_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        WorldTransform* parent_ = nullptr;
        std::vector<WorldTransform*> children_;

        Matrix4x4 worldMatrix_ = MakeIdentity4x4();
        uint64_t updatedFrame_ = 0; // 最後に UpdateMatrix した フレーム番号 (0 = まだ一度も計算していない)
    };
}
