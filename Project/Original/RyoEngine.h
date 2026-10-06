#pragma once
#define _USE_MATH_DEFINES
#include "Core/Base/WinApp.h"
#include "Core/Base/DirectXCommon.h"
#include "Core/Base/Logger.h"
#include "Core/Base/Font.h"
#include "Core/Base/TimeManager.h"
#include "Graphics/2D/TextureManager.h"
#include "Core/Loader/ModelLoader.h"
#include "Core/Base/ShaderCompiler.h"
#include "Core/Base/IGame.h"
#include "Core/Easing/Easing.h"
#include "Core/Math/Math.h"
#include "Core/Math/Collision.h"
#include "Core/Math/BlendMode.h"
#include "Core/Input/Input.h"

#include "Graphics/3D/ModelCommon.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/InstancedModelCommon.h"
#include "Graphics/3D/InstancedModel.h"
#include "Graphics/2D/SpriteCommon.h"
#include "Graphics/2D/Sprite.h"
#include "Graphics/3D/Mesh/Mesh.h"
#include "Graphics/Light/LightManager.h"
#include "Graphics/Shadow/ShadowMap.h"
#include "Graphics/PostProcess/PostProcess.h"
#include "Graphics/PrimitiveRenderer/PrimitiveRenderer.h"
#include "Graphics/GPUParticle/GPUParticleCommon.h"
#include "Graphics/GPUParticle/GPUParticleEmitter.h"
#include "Graphics/GPUParticle/GPUParticleManager.h"

#include "Audio/Audio.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "ImGui/ImGuiManager.h"
#include "Editor/AnimEditor.h"
#include "Editor/ParamEditor.h"

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
    float GetFps();

    /// <summary>
    /// deltaTimeにscaleがかけられたタイムを返す（スケールの初期値は1.0f。等倍である）
    /// </summary>
    /// <returns></returns>
    float GetScaleTime();

    /// <summary>
    /// 現在のタイムスケールを取得
    /// </summary>
    /// <returns></returns>
    float GetTimeScale();

    /// <summary>
    /// タイムスケールをセット
    /// </summary>
    /// <param name="scale"></param>
    void SetTimeScale(float scale);

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

    /// <summary>
    /// 画像の読み込み
    /// </summary>
    /// <param name="filePath">Resources/ApplicationResources/　の後のパスを記述</param>
    /// <returns></returns>
    uint32_t LoadTex(const std::string& filePath);
    
    /// <summary>
    /// BGMの読み込み
    /// </summary>
    /// <param name="filePath">Resources/ApplicationResources/　の後のパスを記述</param>
    /// <returns></returns>
    uint32_t LoadBGM(const std::string& filePath);

    /// <summary>
    /// SEの読み込み
    /// </summary>
    /// <param name="filePath">Resources/ApplicationResources/　の後のパスを記述</param>
    /// <returns></returns>
    uint32_t LoadSE(const std::string& filePath);

    /// <summary>
    /// テキスト表示（Debug時のみ）
    /// </summary>
    template <typename... Args>
    void PrintText(std::format_string<Args...> fmt, Vector2 position, Args&&... args) {
        if (!GetFontOutputter()) return;
        GetFontOutputter()->ScreenPrint(fmt, position,std::forward<Args>(args)...);
    }

    void Change2DBlendMode(BlendMode blendMode);
    void Change3DBlendMode(BlendMode blendMode);

    /// <summary>
    /// 現在有効なカメラを取得する（カメラの更新は各自で）
    /// </summary>
    /// <param name="mainCamera">通ゲーム内で使用するカメラ</param>
    /// <param name="debugCamera">デバッグカメラ</param>
    /// <returns>有効なカメラの参照</returns>
    Camera& GetActiveCamera(Camera& mainCamera);

    /// <summary>
    /// デバッグカメラの取得（デバッグカメラは全体でひとつあれば良いのでエンジンが保持）
    /// </summary>
    /// <returns></returns>
    DebugCamera& GetDebugCamera();
}