#pragma once
#include <map>
#include <string>
#include "../Math/Vector.h"

namespace RyoEngine {

    /// <summary>
    /// コード側の変数(座標・フラグ)へのポインタを、キー名を付けて預かるだけのクラス。
    /// ParticleManager・LightManager などが、ここに登録されたキーの中から好きなものを選んで参照する
    /// (どれを参照するかは ImGui で選んで保存する。コード側は登録するだけ)。
    ///
    /// ・登録したポインタは、毎フレーム自動で読まれる (対象が動いても再登録は不要)
    /// ・同じキーで登録し直すと上書きされる
    /// ・シーンが切り替わると全部解除される (SceneManager が Clear を呼ぶ)
    /// ・途中で消えるもの (GameObject など) が登録した場合は、消える前 (Finalize など) に必ず Unregister すること
    ///   (忘れると、消えた後のメモリを読みにいってしまう)
    ///
    /// 使い方:
    ///   BindingRegistry::RegisterPosition("player pos", &GetTransform().GetLocal().translate);
    ///   BindingRegistry::RegisterFlag("player light", &isIllumination_);
    ///   BindingRegistry::Unregister("player pos");
    /// </summary>
    class BindingRegistry {
    public:
        /// <summary>座標をキー名で登録する。position が nullptr なら登録解除と同じ</summary>
        static void RegisterPosition(const std::string& key, const Vector3* position);
        /// <summary>フラグをキー名で登録する。flag が nullptr なら登録解除と同じ</summary>
        static void RegisterFlag(const std::string& key, const bool* flag);

        static void UnregisterPosition(const std::string& key);
        static void UnregisterFlag(const std::string& key);
        /// <summary>そのキーの座標とフラグを両方解除する</summary>
        static void Unregister(const std::string& key);

        /// <summary>全部解除する (シーン切り替え時に SceneManager から呼ばれる)</summary>
        static void Clear();

        /// <summary>キーから登録済みのポインタを引く。空のキー・未登録なら nullptr</summary>
        static const Vector3* FindPosition(const std::string& key);
        static const bool* FindFlag(const std::string& key);

        /// <summary>登録済みの一覧 (キーの名前順)</summary>
        static const std::map<std::string, const Vector3*>& GetPositions() { return GetInstance()->positions_; }
        static const std::map<std::string, const bool*>& GetFlags() { return GetInstance()->flags_; }

        /// <summary>
        /// ImGui 用：登録済みの座標のキーから1つ選ぶコンボ (「(なし)」も選べる)。選択が変わったら true。
        /// 選んでいるキーが今は登録されていなければ「(未登録)」と表示する (DEBUGのみ。RELEASEでは何もしない)
        /// </summary>
        static bool DrawPositionCombo(const char* label, std::string& selectedKey);
        /// <summary>ImGui 用：登録済みのフラグのキーから1つ選ぶコンボ (DrawPositionCombo のフラグ版)</summary>
        static bool DrawFlagCombo(const char* label, std::string& selectedKey);

    private:
        static BindingRegistry* GetInstance();

        BindingRegistry() = default;
        ~BindingRegistry() = default;
        BindingRegistry(const BindingRegistry&) = delete;
        BindingRegistry& operator=(const BindingRegistry&) = delete;

        // キー名 → コード側の変数へのポインタ (ImGuiのコンボで名前順に並べたいので std::map)
        std::map<std::string, const Vector3*> positions_;
        std::map<std::string, const bool*> flags_;
    };
}
