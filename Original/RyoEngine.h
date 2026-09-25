#pragma once
#define _USE_MATH_DEFINES
#include "Base/WinApp.h"
#include "Base/DirectXCommon.h"
#include "Base/Logger.h"
#include "Base/Font.h"
#include "Graphics/TextureManager.h"
#include "Loader/ModelLoader.h"
#include "Base/ShaderCompiler.h"
#include "3D/ModelCommon.h"
#include "3D/Model.h"
#include "3D/InstancedModelCommon.h"
#include "3D/InstancedModel.h"
#include "2D/SpriteCommon.h"
#include "2D/Sprite.h"
#include "Mesh/Mesh.h"
//#include "Reflect/ReflectCommon.h"
//#include "Reflect/ReflectModel.h"
#include "Audio/Audio.h"
#include "Easing/Easing.h"
#include "Math/Math.h"
#include "Math/Collision.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "Input/Input.h"
#include "ImGui/ImGuiManager.h"
#include "Edit/AnimEdit.h"
#include "Light/LightManager.h"
#include "Shadow/ShadowMap.h"
#include "PostProcess/PostProcess.h"
#include "PrimitiveRenderer/PrimitiveRenderer.h"
#include <cstdint>
#include <string>
#include <format>
#include <utility>
#include <memory>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

namespace RyoEngine {

    WinApp* GetWinApp();
    DirectXCommon* GetDxCommon();
    TextureManager* GetTexManager();
    ModelCommon* GetModelCommon();
    SpriteCommon* GetSpriteCommon();
    //ReflectCommon* GetReflectCommon();
    Font* GetFontOutputter();
    
    void Initialize(const wchar_t* title);
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

    bool GetOnTheGameView();


    inline void SetCameraForPrimitive(const Camera& camera) {
        PrimitiveRenderer::SetCamera(camera);
    }
    inline void DrawSphere(const Vector3& center, float radius, uint32_t subdivision,
        const Vector4& color, PrimitiveDrawMode mode) {
        PrimitiveRenderer::DrawSphere(center, radius, subdivision, color, mode);
    }

    int32_t RandomInt32_t(int32_t min, int32_t max);
    float RandomFloat(float min, float max);

    uint32_t LoadTex(const std::string& filePath);
    
    template <typename... Args>
    void PrintText(std::format_string<Args...> fmt, Vector2 position, Args&&... args) {
        if (!GetFontOutputter()) return;
        GetFontOutputter()->ScreenPrint(fmt, position,std::forward<Args>(args)...);
    }
}