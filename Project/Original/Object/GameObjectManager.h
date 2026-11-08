#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "GameObject.h"

namespace RyoEngine {

    /// <summary>
    /// GameObject の持ち主。作成・更新・描画・削除をまとめて行う。
    ///
    /// ・作るときは Create&lt;T&gt;(コンストラクタの引数...)。作った直後に Initialize が呼ばれ、ポインタが返る
    ///   (ポインタは、その GameObject が消えるまで使える)
    /// ・Update / Draw はエンジン(SceneManager)が自動で呼ぶ。順番は「GameObject → シーン」
    ///   (シーンの Update では、全員が動いて当たり判定も済んだ後の状態を見られる)
    /// ・Destroy された GameObject は、Update の最後にまとめて消える
    /// ・シーンが切り替わると全部消える (SceneManager が Clear を呼ぶ)
    /// ・SetPaused(true) の間は、Update と当たり判定を止める (Draw は続ける)
    ///
    /// 使い方:
    ///   player_ = GameObjectManager::Create&lt;Player&gt;();
    ///   GameObjectManager::Create&lt;Bullet&gt;(position, velocity);   // 引数はコンストラクタへ渡される
    /// </summary>
    class GameObjectManager {
    public:
        static void Initialize();
        static void Finalize();

        /// <summary>
        /// GameObject を作る。引数は T のコンストラクタへ渡される。作った直後に Initialize が呼ばれる。
        /// Update の途中で作ったものは、次のフレームから Update される
        /// </summary>
        template <class T, class... Args>
        static T* Create(Args&&... args) {
            static_assert(std::is_base_of_v<GameObject, T>, "GameObjectManager::Create には GameObject の派生クラスを指定すること");
            std::unique_ptr<T> object = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = object.get();
            Add(std::move(object));
            return raw;
        }

        // ---------------- 毎フレーム (エンジンから呼ばれる) ----------------
        /// <summary>全員の Update → 当たり判定 → 作ったものを仲間に入れる → Destroy されたものを消す</summary>
        static void Update();
        /// <summary>全員の Draw (と、当たり判定の表示)</summary>
        static void Draw();
        /// <summary>全部消す (シーン切り替え時に SceneManager から呼ばれる)</summary>
        static void Clear();
        /// <summary>
        /// ParamEditor に「当たり判定を表示」を登録する。
        /// シーン切り替えで ParamEditor の登録が消えるので、そのたびに SceneManager から呼ばれる
        /// </summary>
        static void RegisterDebugParams();

        // ---------------- 一時停止 ----------------
        /// <summary>true の間、Update と当たり判定を止める (ポーズ画面など)。Draw は続ける</summary>
        static void SetPaused(bool paused) { GetInstance()->isPaused_ = paused; }
        static bool IsPaused() { return GetInstance()->isPaused_; }

        // ---------------- 検索 ----------------
        /// <summary>名前で探す (最初に見つかったもの)。無ければ nullptr</summary>
        static GameObject* FindByName(const std::string& name);
        /// <summary>タグで全部探す</summary>
        static std::vector<GameObject*> FindAllByTag(const std::string& tag);
        /// <summary>今いる GameObject の数 (消える予定のものも含む)</summary>
        static size_t GetCount();

    private:
        static GameObjectManager* GetInstance();

        GameObjectManager() = default;
        ~GameObjectManager() = default;
        GameObjectManager(const GameObjectManager&) = delete;
        GameObjectManager& operator=(const GameObjectManager&) = delete;

        static void Add(std::unique_ptr<GameObject> object);
        // Update 中に作られたものを仲間に入れる
        void FlushPending();
        // Destroy されたものを消す
        void RemoveDead();
        // 当たり判定の形を線で描く (DEBUGのみ)
        void DrawColliders();

        std::vector<std::unique_ptr<GameObject>> objects_;
        // Update / Draw の途中で作られたもの (ループ中に objects_ を増やさないため)
        std::vector<std::unique_ptr<GameObject>> pending_;
        // objects_ をループしている最中か
        bool isIterating_ = false;
        bool isPaused_ = false;
        uint32_t nextId_ = 1;

        // 当たり判定を線で表示するか (ParamEditor の "GameObject" グループ)
        bool showColliders_ = false;
        // 表示の色 (当たっていない / 当たっている)
        Vector4 colliderColor_ = { 0.0f, 1.0f, 0.0f, 1.0f };
        Vector4 colliderHitColor_ = { 1.0f, 0.2f, 0.2f, 1.0f };
    };
}
