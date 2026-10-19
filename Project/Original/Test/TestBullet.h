#pragma once
#include <RyoEngine.h>
#include <memory>

namespace Test {
    class TestBullet {
    public:
        TestBullet() = default;
        // コピーコンストラクタの削除
        TestBullet(const TestBullet&) = delete;
        // コピー代入演算子の削除
        TestBullet& operator=(const TestBullet&) = delete;

        // modelIndex: LoadObj で読み込んだ弾モデル (全弾で共有。GPU側は自動で1回のDrawInstancedにまとまる)
        void Initialize(uint32_t modelIndex, const RyoEngine::Vector3& position);
        // 移動・寿命の処理と行列の確定
        void Update();
        // 描画の提出 (弾は大量なのでフラスタムカリングはModel側で行う)
        void Draw();
        void ResolveDead();

        // 削除判定用
        bool IsDead() const { return isDead_; }
    private:
        // 弾1発 = Model 1体。Transformの持ち主はModel (GetTransform())
        // Modelはコピー不可なので TestBullet もムーブ専用になる (vector には std::move / emplace_back で入れる)
        std::unique_ptr<RyoEngine::Model> model_{};
        RyoEngine::Vector3 velocity_{};
        bool isDead_ = false;

        float timer_ = 10.0f;
    };
}
