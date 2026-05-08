#pragma once
#include "Base/WinApp.h"
#include "Base/DirectXCommon.h"
#include "Base/Logger.h"
#include "Graphics/TextureManager.h"
#include "Loader/ModelLoader.h"
#include "Base/ShaderCompiler.h"
#include "3D/Object3dCommon.h"
#include "3D/Object3d.h"
#include "2D/SpriteCommon.h"
#include "2D/Sprite.h"
#include "Audio/Audio.h"
#include "Math/Math.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "Input/Input.h"
#include <memory>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

namespace Engine {

    void Initialize();

    void Finalize();
    

    extern WinApp* winApp_;
    extern DirectXCommon* dxCommon_;
    extern ShaderCompiler* shaderCompiler_;
    extern TextureManager* textureManager_;
    extern Object3dCommon* object3dCommon_;
    extern SpriteCommon* spriteCommon_;
    extern Audio* audio_;

    static WinApp* GetWinApp() { return winApp_; };
    static DirectXCommon* GetDxCommon() { return dxCommon_; };
    static Object3dCommon* GetObject3dCommon() { return object3dCommon_; };
    static SpriteCommon* GetSpriteCommon() { return spriteCommon_; };
}