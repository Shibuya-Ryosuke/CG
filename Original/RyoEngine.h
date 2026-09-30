#pragma once
#define _USE_MATH_DEFINES
#include "Base/WinApp.h"
#include "Base/DirectXCommon.h"
#include "Base/Logger.h"
#include "Base/Font.h"
#include "Base/TimeManager.h"
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
#include "Math/BlendMode.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "Input/Input.h"
#include "ImGui/ImGuiManager.h"
#include "Editor/AnimEditor.h"
#include "Editor/ParamEditor.h"
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
    /// <param name="defaultCamera">通常のカメラ</param>
    /// <param name="debugCamera">デバッグカメラ</param>
    /// <returns>有効なカメラの参照</returns>
    Camera& GetActiveCamera(Camera& defaultCamera, DebugCamera& debugCamera);
}