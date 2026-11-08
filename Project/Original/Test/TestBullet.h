#pragma once
#include <RyoEngine.h>
#include <memory>

namespace Test {
    /// <summary>
    /// テスト用の弾 (GameObject)。まっすぐ飛び、敵に当たるか時間が来たら消える。
    /// 作るときは GameObjectManager::Create&lt;TestBullet&gt;(位置, 速度, 消えるまでの時間)
    /// </summary>
    class TestBullet : public RyoEngine::GameObject {
    public:
        TestBullet(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity, float lifeTime);
        ~TestBullet() override = default;
        // コピーコンストラクタの削除
        TestBullet(const TestBullet&) = delete;
        // コピー代入演算子の削除
        TestBullet& operator=(const TestBullet&) = delete;

        // モデルと当たり判定の準備 (同じobjは全弾で共有。GPU側は自動で1回のDrawInstancedにまとまる)
        void Initialize() override;
        // 移動・寿命の処理と行列の確定
        void Update() override;
        // 描画の提出 (弾は大量なのでフラスタムカリングはModel側で行う)
        void Draw() override;

        // 敵に当たったら消える
        void OnCollisionEnter(RyoEngine::GameObject* other) override;

    private:
        // 弾1発 = Model 1体。Transformの持ち主は GameObject (GetTransform())。Model は SetParent でくっつける
        std::unique_ptr<RyoEngine::Model> model_{};
        RyoEngine::Vector3 velocity_{};

        float timer_ = 10.0f;
        // 見た目と当たり判定の大きさ (GameObject のスケール。当たり判定にも自動で掛かる)
        float scale_ = 0.3f;
    };
}
