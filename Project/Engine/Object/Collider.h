#pragma once
#include <cstdint>
#include "../Core/Math/Math.h"

namespace RyoEngine {

    /// <summary>当たり判定の形</summary>
    enum class ColliderShape {
        Sphere, // 球。一番軽い。回っても形が変わらない (弾・アイテム・丸っぽい物)
        AABB,   // 回転しない箱。軽い (床・壁など動かない物)
        OBB,    // 回転する箱。GameObject の向きに合わせて回る (細長い物・剣・人型の体など)
    };

    /// <summary>
    /// 当たり判定1つ分のデータ。GameObject が持ち、CollisionManager が毎フレーム判定する。
    /// GameObject::AddSphereCollider / AddAABBCollider / AddOBBCollider で追加する。
    ///
    /// 大きさ・ずれは GameObject の座標(WorldTransform)から見た値。
    /// GameObject を動かす・回す・拡縮すると、当たり判定もついてくる (AABB は回転だけ無視する)。
    /// </summary>
    struct Collider {
        ColliderShape shape = ColliderShape::Sphere;
        // GameObject の座標からのずれ (GameObject の回転・スケールに合わせて回る)
        Vector3 offset = { 0.0f, 0.0f, 0.0f };
        // 球の半径 (Sphere のとき使う)
        float radius = 0.5f;
        // 箱の各軸の半分の長さ (AABB / OBB のとき使う。全長ではない点に注意)
        Vector3 halfSize = { 0.5f, 0.5f, 0.5f };

        // 自分は何者か (ビットを1つ立てる。例: 1 << 0)
        uint32_t layer = 0;
        // 誰と当たりたいか (当たりたい相手の layer のビットを全部立てる)
        // お互いの mask に相手の layer が含まれているときだけ判定する
        uint32_t mask = 0;
        // false にすると判定しない
        bool enabled = true;

        // ---- 以下は CollisionManager が毎フレーム書き込む (使う側は読むだけ) ----
        Sphere worldSphere{};
        AABB worldAABB{};
        OBB worldOBB{};
        // このフレームで何かに当たったか (デバッグ表示の色分け用)
        bool isHitting = false;
    };
}
