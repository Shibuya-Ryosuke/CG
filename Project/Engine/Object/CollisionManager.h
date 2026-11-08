#pragma once
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace RyoEngine {
    class GameObject;
    struct Collider;
    class WorldTransform;

    /// <summary>
    /// GameObject が持っている当たり判定(Collider)を集めて判定し、当たった GameObject に知らせるクラス。
    /// GameObjectManager::Update から毎フレーム呼ばれる (ゲーム側は呼ばなくてよい)。
    ///
    /// ・お互いの mask に相手の layer が含まれている組み合わせだけ判定する
    /// ・同じ GameObject の当たり判定どうしは判定しない
    /// ・GameObject 単位で、前のフレームと比べて Enter(当たった瞬間) / Stay(当たっている間) / Exit(離れた瞬間) を呼ぶ
    /// ・総当たりで判定する (数百個くらいまでなら問題ない)
    /// </summary>
    class CollisionManager {
    public:
        /// <summary>当たり判定の形を今の座標で計算し、判定して、Enter/Stay/Exit を呼ぶ</summary>
        static void Check(const std::vector<GameObject*>& objects);

        /// <summary>
        /// GameObject が消える直前に呼ばれる。当たっていた相手に OnCollisionExit を知らせる
        /// </summary>
        static void OnObjectRemoved(GameObject* object);

        /// <summary>覚えている組み合わせを全部忘れる (シーン切り替え時。Exit は呼ばない)</summary>
        static void Clear();

        /// <summary>当たり判定の形を、GameObject の今の座標で計算し直す</summary>
        static void UpdateWorldShape(Collider& collider, const WorldTransform& transform);

    private:
        static CollisionManager* GetInstance();

        CollisionManager() = default;
        ~CollisionManager() = default;
        CollisionManager(const CollisionManager&) = delete;
        CollisionManager& operator=(const CollisionManager&) = delete;

        // 2つの当たり判定が当たっているか (形の組み合わせごとに IsCollision を呼び分ける)
        static bool IsHit(const Collider& a, const Collider& b);
        // 2つの GameObject の組み合わせを表すキー (番号の小さい方が上位)
        static uint64_t MakePairKey(const GameObject* a, const GameObject* b);

        // 前のフレームで当たっていた組み合わせ (キー → 2つの GameObject)
        using PairMap = std::unordered_map<uint64_t, std::pair<GameObject*, GameObject*>>;
        PairMap previousPairs_;
    };
}
