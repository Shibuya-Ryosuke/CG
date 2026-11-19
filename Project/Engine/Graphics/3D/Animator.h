#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include "../../Core/Animation/AnimationData.h"

namespace RyoEngine {

    /// <summary>
    /// 1体分のアニメーションの再生状態と、今の姿勢 (各ボーンの行列) を持つクラス。
    /// Model が骨格を持つモデルのときだけ1つ持つ (ゲーム側は Model の PlayAnimation などを使う)。
    ///
    /// ・Play     : 名前で再生する。同じ名前を再生中にもう一度呼んでも、最初からにはしない (restart で最初から)
    ///              blendSeconds を渡すと、今の姿勢から新しいアニメーションへ、その秒数かけて混ぜて切り替える
    /// ・Stop     : その場の姿勢で止める。次の Play は最初から
    /// ・ResetPose: アニメーションの無い、元の姿勢に戻す
    /// ・Pause / Resume: 一時停止と、止めた所からの再開
    /// </summary>
    class Animator {
    public:
        /// <summary>モデルに結びつける (骨格を持つモデルの番号)</summary>
        void Initialize(uint32_t modelIndex);

        /// <summary>
        /// 時間を進めて、今の姿勢を計算する (1フレームに1回)。
        /// 姿勢が変わったら true (スキニングをやり直す必要がある)
        /// </summary>
        bool Update(float deltaTime);

        void Play(const std::string& name, bool loop, float blendSeconds, bool restart);
        void Stop();
        void ResetPose();
        void Pause();
        void Resume();

        /// <summary>再生中か (一時停止中・止めた後・ループしないアニメーションが終わった後は false)</summary>
        bool IsPlaying() const { return playing_ && !paused_; }
        /// <summary>ループしないアニメーションが最後まで行ったか (ループ中は常に false)</summary>
        bool IsFinished() const { return finished_; }
        bool IsPaused() const { return paused_; }

        void SetSpeed(float speed) { speed_ = speed; }
        float GetSpeed() const { return speed_; }
        /// <summary>今の再生位置 (秒)</summary>
        float GetTime() const { return time_; }
        /// <summary>再生位置を変える (秒)。姿勢もすぐに変わる</summary>
        void SetTime(float time);
        /// <summary>今のアニメーションの長さ (秒。無ければ0)</summary>
        float GetDuration() const;
        /// <summary>今のアニメーションの名前 (無ければ空)</summary>
        const std::string& GetCurrentName() const;

        /// <summary>各ボーンの今の行列 (モデルの原点から見た座標。骨格の順番)</summary>
        const std::vector<Matrix4x4>& GetJointMatrices() const { return jointMatrices_; }
        /// <summary>元の姿勢から一番大きく動いたボーンの移動距離 (カリングの球を広げるため)</summary>
        float GetMaxJointOffset() const { return maxJointOffset_; }

    private:
        // ボーン1本の姿勢 (親から見た SRT)
        struct JointPose {
            Vector3 translate;
            Quaternion rotate;
            Vector3 scale;
        };

        const SkeletalAnimation* GetAnimation(int32_t index) const;
        // 元の姿勢 + アニメーション(index, time) の姿勢を out に入れる (index < 0 なら元の姿勢)
        void SamplePose(int32_t index, float time, std::vector<JointPose>& out) const;
        // pose_ から各ボーンの行列を計算する
        void BuildMatrices();

        uint32_t modelIndex_ = 0;

        int32_t current_ = -1; // 再生中のアニメーションの番号 (-1 = なし)
        float time_ = 0.0f;
        float speed_ = 1.0f;
        bool loop_ = true;
        bool playing_ = false;
        bool paused_ = false;
        bool finished_ = false;
        bool dirty_ = true; // 次の Update で姿勢を計算し直すか

        // 切り替えのブレンド (切り替えた瞬間の姿勢から混ぜる)
        std::vector<JointPose> blendFrom_;
        float blendDuration_ = 0.0f;
        float blendElapsed_ = 0.0f;

        std::vector<JointPose> pose_;           // 今の姿勢
        std::vector<JointPose> sampled_;        // 計算用 (毎回確保しないため)
        std::vector<Matrix4x4> jointMatrices_;  // 今の各ボーンの行列
        float maxJointOffset_ = 0.0f;

        // 見つからなかったアニメーションの名前 (警告を1回だけ出すため)
        std::unordered_set<std::string> warnedNames_;
    };
}
