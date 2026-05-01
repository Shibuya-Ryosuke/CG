#pragma once
#include "Base/WinApp.h"
#include "Base/DirectXCommon.h"
#include "Graphics/TextureManager.h"
#include "Base/ShaderCompiler.h"
#include "3D/Object3dCommon.h"
#include "3D/Object3d.h"
#include "2D/SpriteCommon.h"
#include "2D/Sprite.h"
#include "Audio/Audio.h"
#include <memory>

namespace Engine {

    class Framework {
    public:
        // エンジンの初期化
        void Initialize(const wchar_t* title, int32_t width, int32_t height);

        // エンジンの終了処理
        void Finalize();

        // メッセージループ（続行ならtrue）
        bool ProcessMessage();

        // 描画開始処理
        void BeginFrame();

        // 描画終了（コマンドリスト実行、フリップ）
        void EndFrame();

        // ゲッター
        WinApp* GetWinApp() { return winApp_; }
        DirectXCommon* GetDXCommon() { return dxCommon_; }

    private:
        WinApp* winApp_ = nullptr;
        DirectXCommon* dxCommon_ = nullptr;
        TextureManager* textureManager_ = nullptr;
        Object3dCommon* object3dCommon_ = nullptr;
        SpriteCommon* spriteCommon_ = nullptr;
        Audio* audio_ = nullptr;
    };

}