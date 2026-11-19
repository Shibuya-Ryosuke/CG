#include "Animator.h"
#include "ModelManager.h"
#include "../../Core/Base/Logger.h"
#include <cmath>

namespace RyoEngine {

    void Animator::Initialize(uint32_t modelIndex) {
        modelIndex_ = modelIndex;
        current_ = -1;
        time_ = 0.0f;
        playing_ = false;
        paused_ = false;
        finished_ = false;
        blendDuration_ = 0.0f;
        blendElapsed_ = 0.0f;
        dirty_ = true;
        pose_.clear();
        jointMatrices_.clear();
        maxJointOffset_ = 0.0f;
        warnedNames_.clear();

        // 最初は元の姿勢
        SamplePose(-1, 0.0f, pose_);
        BuildMatrices();
    }

    const SkeletalAnimation* Animator::GetAnimation(int32_t index) const {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset || index < 0 || static_cast<size_t>(index) >= asset->animations.size()) {
            return nullptr;
        }
        return &asset->animations[static_cast<size_t>(index)];
    }

    void Animator::SamplePose(int32_t index, float time, std::vector<JointPose>& out) const {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset) {
            out.clear();
            return;
        }
        const std::vector<JointData>& joints = asset->skeleton.joints;
        out.resize(joints.size());
        // まず元の姿勢を入れる (アニメーションで動かないボーンはこのまま)
        for (size_t i = 0; i < joints.size(); ++i) {
            out[i] = { joints[i].translate, joints[i].rotate, joints[i].scale };
        }

        const SkeletalAnimation* animation = GetAnimation(index);
        if (!animation) {
            return;
        }
        for (const AnimationChannelBinding& channel : animation->channels) {
            JointPose& pose = out[static_cast<size_t>(channel.jointIndex)];
            const NodeAnimation& nodeAnimation = *channel.animation;
            if (channel.useTranslate) {
                const Vector3 translate = SampleCurve(nodeAnimation.translate, time, pose.translate);
                // 移動量の倍率は、キーがあるときだけかける (キーが無ければ元の姿勢のまま)
                pose.translate = nodeAnimation.translate.keyframes.empty() ? translate : translate * channel.translateScale;
            }
            pose.rotate = SampleCurve(nodeAnimation.rotate, time, pose.rotate);
            if (channel.useScale) {
                pose.scale = SampleCurve(nodeAnimation.scale, time, pose.scale);
            }
        }
    }

    void Animator::BuildMatrices() {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset) {
            jointMatrices_.clear();
            return;
        }
        const std::vector<JointData>& joints = asset->skeleton.joints;
        jointMatrices_.resize(joints.size());
        maxJointOffset_ = 0.0f;
        for (size_t i = 0; i < joints.size() && i < pose_.size(); ++i) {
            const JointPose& pose = pose_[i];
            const Matrix4x4 local = MakeAffineMatrix(pose.scale, pose.rotate, pose.translate);
            // 親は必ず自分より前にあるので、前から順に計算すればよい
            jointMatrices_[i] = joints[i].parent >= 0 ? local * jointMatrices_[static_cast<size_t>(joints[i].parent)] : local;

            // 元の姿勢からどれだけ動いたか (カリングの球を広げるため)
            if (i < asset->bindPoseMatrices.size()) {
                const Matrix4x4& now = jointMatrices_[i];
                const Matrix4x4& bind = asset->bindPoseMatrices[i];
                const Vector3 offset = { now.m[3][0] - bind.m[3][0], now.m[3][1] - bind.m[3][1], now.m[3][2] - bind.m[3][2] };
                const float distance = Length(offset);
                if (distance > maxJointOffset_) {
                    maxJointOffset_ = distance;
                }
            }
        }
    }

    bool Animator::Update(float deltaTime) {
        bool changed = dirty_;
        dirty_ = false;

        const SkeletalAnimation* animation = GetAnimation(current_);

        // 時間を進める
        if (animation && playing_ && !paused_) {
            const float step = deltaTime * speed_;
            if (step != 0.0f) {
                time_ += step;
                const float duration = animation->duration;
                if (duration <= 0.0f) {
                    time_ = 0.0f;
                } else if (loop_) {
                    time_ = std::fmod(time_, duration);
                    if (time_ < 0.0f) {
                        time_ += duration; // 逆再生
                    }
                } else if (time_ >= duration || time_ <= 0.0f) {
                    // ループしない: 端で止めて、最後の姿勢のままにする
                    time_ = time_ >= duration ? duration : 0.0f;
                    playing_ = false;
                    finished_ = true;
                }
                changed = true;
            }
        }

        // 切り替えのブレンドを進める
        if (blendElapsed_ < blendDuration_ && !paused_) {
            blendElapsed_ += deltaTime;
            changed = true;
        }

        if (!changed) {
            return false;
        }

        // 今の姿勢を計算する
        SamplePose(animation ? current_ : -1, time_, sampled_);
        if (blendDuration_ > 0.0f && blendElapsed_ < blendDuration_ && blendFrom_.size() == sampled_.size()) {
            // 切り替えた瞬間の姿勢から、新しいアニメーションの姿勢へ混ぜる
            const float t = blendElapsed_ / blendDuration_;
            pose_.resize(sampled_.size());
            for (size_t i = 0; i < sampled_.size(); ++i) {
                pose_[i].translate = Lerp(blendFrom_[i].translate, sampled_[i].translate, t);
                pose_[i].rotate = Slerp(blendFrom_[i].rotate, sampled_[i].rotate, t);
                pose_[i].scale = Lerp(blendFrom_[i].scale, sampled_[i].scale, t);
            }
        } else {
            pose_ = sampled_;
            blendDuration_ = 0.0f;
        }
        BuildMatrices();
        return true;
    }

    void Animator::Play(const std::string& name, bool loop, float blendSeconds, bool restart) {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        const int32_t index = asset ? asset->FindAnimation(name) : -1;
        if (index < 0) {
            // 毎フレーム呼ばれてもログが溢れないように、同じ名前は1回だけ
            if (warnedNames_.insert(name).second) {
                Logger::LogWarning("[Animator] アニメーションが見つかりません: \"{}\" ({})\n", name, asset ? asset->filePath : std::string());
            }
            return;
        }

        // 同じアニメーションを再生中 (一時停止中も含む) なら、最初からにはせずそのまま続ける
        if (index == current_ && !restart && (playing_ || paused_)) {
            loop_ = loop;
            paused_ = false;
            return;
        }

        // 切り替え: 今の姿勢から混ぜる
        if (blendSeconds > 0.0f && !pose_.empty()) {
            blendFrom_ = pose_;
            blendDuration_ = blendSeconds;
            blendElapsed_ = 0.0f;
        } else {
            blendDuration_ = 0.0f;
            blendElapsed_ = 0.0f;
        }

        current_ = index;
        loop_ = loop;
        // 逆再生のときは最後から
        time_ = speed_ < 0.0f ? asset->animations[static_cast<size_t>(index)].duration : 0.0f;
        playing_ = true;
        paused_ = false;
        finished_ = false;
        dirty_ = true;
    }

    void Animator::Stop() {
        // その場の姿勢で止める (姿勢は計算し直さない)。次の Play は最初から
        playing_ = false;
        paused_ = false;
        finished_ = false;
        blendDuration_ = 0.0f;
        blendElapsed_ = 0.0f;
    }

    void Animator::ResetPose() {
        Stop();
        current_ = -1;
        time_ = 0.0f;
        dirty_ = true;
    }

    void Animator::Pause() {
        if (playing_) {
            paused_ = true;
        }
    }

    void Animator::Resume() {
        paused_ = false;
    }

    void Animator::SetTime(float time) {
        const SkeletalAnimation* animation = GetAnimation(current_);
        if (!animation) {
            return;
        }
        time_ = time;
        if (animation->duration > 0.0f) {
            if (loop_) {
                time_ = std::fmod(time_, animation->duration);
                if (time_ < 0.0f) time_ += animation->duration;
            } else {
                time_ = time_ < 0.0f ? 0.0f : (time_ > animation->duration ? animation->duration : time_);
            }
        } else {
            time_ = 0.0f;
        }
        dirty_ = true;
    }

    float Animator::GetDuration() const {
        const SkeletalAnimation* animation = GetAnimation(current_);
        return animation ? animation->duration : 0.0f;
    }

    const std::string& Animator::GetCurrentName() const {
        static const std::string empty;
        const SkeletalAnimation* animation = GetAnimation(current_);
        return animation ? animation->name : empty;
    }
}
