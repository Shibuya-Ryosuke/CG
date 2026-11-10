#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Collider.h"
#include "../Core/Math/WorldTransform.h"

namespace RyoEngine {
    class Model;

    /// <summary>
    /// ゲーム内の「物」の基底クラス (プレイヤー・敵・弾・アイテムなど)。
    ///
    /// ・作成・更新・描画・削除は GameObjectManager が行う。
    ///   作るときは GameObjectManager::Create&lt;Player&gt;(...)。Update / Draw はエンジンが自動で呼ぶ
    /// ・座標(WorldTransform)を持つ。モデルや当たり判定はこの座標についていく。
    ///   モデルは継承先が持ち、model_->SetParent(&GetTransform()) でくっつける
    ///   (AddModel で作ったモデルは GameObject が持ち、TransferMatrix・Draw もエンジンが自動で呼ぶ)
    /// ・当たり判定は Initialize などで AddSphereCollider 等を呼んで追加する。
    ///   当たると OnCollisionEnter / Stay / Exit が呼ばれる (判定は CollisionManager が行う)
    /// ・消すときは Destroy()。そのフレームの最後にまとめて消える (すぐには消えない)
    ///
    /// 呼ばれる順番 (1フレーム):
    ///   全員の Update → 当たり判定 → OnCollisionEnter/Stay/Exit → Destroy されたものを消す → (シーンの Update) → 全員の Draw
    /// </summary>
    class GameObject {
    public:
        GameObject() = default;
        virtual ~GameObject();
        // GameObjectManager がポインタで管理するので、コピー/ムーブは不可
        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;

        // ---------------- 継承先が書くもの ----------------
        /// <summary>作成されたとき1回 (GameObjectManager::Create の中で呼ばれる)</summary>
        virtual void Initialize() {}
        /// <summary>毎フレーム</summary>
        virtual void Update() {}
        /// <summary>毎フレーム</summary>
        virtual void Draw() {}
        /// <summary>消えるとき1回 (Destroy された後、またはシーンが切り替わるとき)</summary>
        virtual void Finalize() {}

        /// <summary>相手と当たった瞬間</summary>
        virtual void OnCollisionEnter(GameObject* other) { (void)other; }
        /// <summary>相手と当たっている間 (毎フレーム。当たった瞬間のフレームも含む)</summary>
        virtual void OnCollisionStay(GameObject* other) { (void)other; }
        /// <summary>相手と離れた瞬間 (相手が消えたときも呼ばれる)</summary>
        virtual void OnCollisionExit(GameObject* other) { (void)other; }

        // ---------------- 座標 ----------------
        /// <summary>
        /// この GameObject の座標 (回転は度数法)。動かすときは GetTransform().GetLocal().translate を変える。
        /// モデルは model_->SetParent(&GetTransform()) でくっつける
        /// </summary>
        WorldTransform& GetTransform() { return transform_; }
        const WorldTransform& GetTransform() const { return transform_; }

        // SRT の近道 (= GetTransform().GetLocal().translate など。親から見た値。親がいなければワールドでの値と同じ)
        // 動かした直後でも最新の値が取れる (行列の計算を待たない)
        const Vector3& GetTranslate() const { return transform_.GetLocal().translate; }
        void SetTranslate(const Vector3& translate) { transform_.GetLocal().translate = translate; }
        /// <summary>回転 (度数法)</summary>
        const Vector3& GetRotate() const { return transform_.GetLocal().rotate; }
        void SetRotate(const Vector3& rotateDeg) { transform_.GetLocal().rotate = rotateDeg; }
        const Vector3& GetScale() const { return transform_.GetLocal().scale; }
        void SetScale(const Vector3& scale) { transform_.GetLocal().scale = scale; }
        /// <summary>ワールド座標 (直前に行列を計算したときの値)</summary>
        Vector3 GetWorldPosition() const { return transform_.GetWorldPosition(); }

        // ---------------- モデル ----------------
        /// <summary>AddModel で付けたモデルを描くか (点滅・やられた演出など。初期値 true)</summary>
        void SetVisible(bool visible) { isVisible_ = visible; }
        bool IsVisible() const { return isVisible_; }

        // ---------------- 名前・タグ ----------------
        /// <summary>名前 (GameObjectManager::FindByName で探すとき用)</summary>
        void SetName(const std::string& name) { name_ = name; }
        const std::string& GetName() const { return name_; }
        /// <summary>タグ (当たった相手の種類を見分けるとき用。例: other->GetTag() == "Enemy")</summary>
        void SetTag(const std::string& tag) { tag_ = tag; }
        const std::string& GetTag() const { return tag_; }

        // ---------------- 削除 ----------------
        /// <summary>消す。そのフレームの最後に、Finalize が呼ばれてから消える</summary>
        void Destroy() { isDead_ = true; }
        bool IsDead() const { return isDead_; }

        // ---------------- 当たり判定 ----------------
        const std::vector<Collider>& GetColliders() const { return colliders_; }
        /// <summary>index番目の当たり判定 (Add○○Collider の戻り値)。大きさ・オンオフを後から変えるとき用</summary>
        Collider& GetCollider(uint32_t index) { return colliders_[index]; }
        uint32_t GetColliderCount() const { return static_cast<uint32_t>(colliders_.size()); }

    protected:
        /// <summary>球の当たり判定を追加する</summary>
        /// <param name="radius">半径</param>
        /// <param name="layer">自分は何者か (ビットを1つ)</param>
        /// <param name="mask">誰と当たりたいか (相手の layer のビットを全部)</param>
        /// <param name="offset">座標からのずれ</param>
        /// <returns>何番目の当たり判定か (GetCollider で使う)</returns>
        uint32_t AddSphereCollider(float radius, uint32_t layer, uint32_t mask, const Vector3& offset = { 0.0f, 0.0f, 0.0f });
        /// <summary>回転しない箱の当たり判定を追加する (halfSize: 各軸の半分の長さ)</summary>
        uint32_t AddAABBCollider(const Vector3& halfSize, uint32_t layer, uint32_t mask, const Vector3& offset = { 0.0f, 0.0f, 0.0f });
        /// <summary>回転する箱の当たり判定を追加する (halfSize: 各軸の半分の長さ)</summary>
        uint32_t AddOBBCollider(const Vector3& halfSize, uint32_t layer, uint32_t mask, const Vector3& offset = { 0.0f, 0.0f, 0.0f });
        /// <summary>当たり判定を全部消す</summary>
        void ClearColliders() { colliders_.clear(); }

        /// <summary>
        /// モデルを作って、この GameObject にくっつける (名前でもパスでもよい。Model::Create と同じ)。
        /// モデルの持ち主は GameObject。TransferMatrix・Draw はエンジンが自動で呼ぶので、継承先は呼ばないこと
        /// (自分でも Draw を呼ぶと2回描かれる)。戻り値は色などを変えるためのポインタ (GameObject が消えるまで使える)
        /// </summary>
        Model* AddModel(const std::string& nameOrPath, uint32_t textureIndex = 0);
        /// <summary>LoadObj で読み込んだ番号からモデルを作って、くっつける</summary>
        Model* AddModel(uint32_t modelIndex, uint32_t textureIndex = 0);

    private:
        friend class GameObjectManager;
        friend class CollisionManager;

        uint32_t AddCollider(const Collider& collider);
        // AddModel で付けたモデルの行列を確定する / 描く (GameObjectManager が呼ぶ)
        void TransferModels();
        void DrawModels();

        WorldTransform transform_;
        std::string name_;
        std::string tag_;
        bool isDead_ = false;
        std::vector<Collider> colliders_;
        // AddModel で付けたモデル (持ち主はこの GameObject)
        std::vector<std::unique_ptr<Model>> models_;
        bool isVisible_ = true;
        uint32_t id_ = 0; // GameObjectManager が付ける通し番号 (当たり判定の組み合わせを覚えるため)
    };
}
