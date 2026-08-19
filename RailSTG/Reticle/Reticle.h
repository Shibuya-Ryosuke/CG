#pragma once
#include <memory>

#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"

class Reticle {
public:
    Reticle();
    ~Reticle();
    Reticle(const Reticle&) = delete;
    Reticle& operator=(const Reticle&) = delete;

    void Initialize();
    void Update();
    void Draw();

    // 2Dのレティクル位置を取得する関数（必要に応じて外部で使う用）
    RyoEngine::Vector2 GetPosition() const { return position_; }

private:
    RyoEngine::Vector2 position_{}; // 画面上の2Dピクセル座標
    std::unique_ptr<RyoEngine::Sprite> sprite_; // 2Dスプライト（エンジンにSpriteがあれば）
};