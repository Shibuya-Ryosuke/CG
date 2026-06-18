#pragma once
#include "Base/WinApp.h"
#include "Base/DirectXCommon.h"
#include "Base/Logger.h"
#include "Base/FontLoader.h"
#include "Graphics/TextureManager.h"
#include "Loader/ModelLoader.h"
#include "Base/ShaderCompiler.h"
#include "3D/ModelCommon.h"
#include "3D/Model.h"
#include "2D/SpriteCommon.h"
#include "2D/Sprite.h"
#include "Mesh/Mesh.h"
#include "Reflect/ReflectCommon.h"
#include "Reflect/ReflectModel.h"
#include "Audio/Audio.h"
#include "Math/Math.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "Input/Input.h"
#include "ImGui/ImGuiManager.h"
#include <cstdint>
#include <string>
#include <memory>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

namespace RyoEngine {

    void Initialize();
    void Finalize();

    void Begin3dDraw();
    void Begin2dDraw();

    void NewFrame();
    void EndFrame();

    /// <summary>
    /// 前のフレームからの経過時間（秒）を取得
    /// </summary>
    float GetDeltaTime();

    /// <summary>
    /// 現在のFPSを取得
    /// </summary>
    float GetFPS();

    /// <summary>
    /// ImGuiのゲーム画面サイズセット
    /// (リリースでは処理なし関数に変化)
    /// </summary>
    /// <param name="viewSize">ゲーム画面サイズ(初期値 960*540)</param>
    void SetImGuiViewSize(Vector2 viewSize);

    void PrintText(const std::string& text, Vector2 position, float scale = 1.0f);

    uint32_t LoadTex(const std::string& filePath);

    WinApp* GetWinApp();
    DirectXCommon* GetDxCommon();
    TextureManager* GetTexManager();
    ModelCommon* GetModelCommon();
    SpriteCommon* GetSpriteCommon();
    ReflectCommon* GetReflectCommon();
}