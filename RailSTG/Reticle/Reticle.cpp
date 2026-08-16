#include "Reticle.h"

using namespace RyoEngine;

Reticle::Reticle() = default;
Reticle::~Reticle() = default;

void Reticle::Initialize() {
    // 2Dスプライトの初期化（パスやサイズはエンジンの仕様に合わせてください）
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("resources/RailSTG/Reticle/reticle.png");
    //sprite_->SetTranslate({ 640.0f, 360.0f }); // 画面中央などを初期位置に
}

void Reticle::Update() {
    // 方法B: Windowsのマウスカーソル座標にそのまま追従させる場合
    position_ = Input::GetMouseScreenPos();
   

    // スプライトの位置を更新
    if (sprite_) {
        sprite_->SetTranslate(position_);
        sprite_->Update();
    }
}

void Reticle::Draw() {
    if (sprite_) {
        sprite_->Draw();
    }
}