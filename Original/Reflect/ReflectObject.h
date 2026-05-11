#pragma once
#include "../3d/Object3d.h"
#include "../Camera/Camera.h"
#include <memory>

namespace Engine {

    class ReflectObject {
    public:
        ReflectObject() = default;
        ~ReflectObject() = default;

        // --- コピー禁止の設定 ---
        // コピーコンストラクタを削除
        ReflectObject(const ReflectObject&) = delete;

        // コピー代入演算子を削除
        ReflectObject& operator=(const ReflectObject&) = delete;

        // --- (任意) 移動の設定 ---
        // 所有権を移動させる「ムーブ」は許可しておくと便利な場合があります
        ReflectObject(ReflectObject&&) = default;
        ReflectObject& operator=(ReflectObject&&) = default;


        // 初期化（板ポリゴンのモデルなどを読み込む）
        void Initialize(const std::string& modelPath);

        // 更新（メインカメラを元に、鏡用の反転カメラ行列を計算する）
        void Update(const Camera& mainCamera);

        // 描画（メインシーンの描画中に呼び出す）
        void Draw();

        // --- セッター ---
        void SetTranslate(const Vector3& translate) { object_->SetTranslate(translate); }
        void SetRotate(const Vector3& rotate) { object_->SetRotate(rotate); }
        void SetScale(const Vector3& scale) { object_->SetScale(scale); }

    private:
        std::unique_ptr<Object3d> object_; // 鏡の実体（板モデル）

        // 反射面を定義する（とりあえず Y=0 の平面とするための法線）
        Vector3 planeNormal_ = { 0.0f, 1.0f, 0.0f };
        float planeDistance_ = 0.0f; // 原点からの距離
    };

}