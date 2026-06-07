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
#include "Reflect/ReflectCommon.h"
#include "Reflect/ReflectObject.h"
#include "Audio/Audio.h"
#include "Math/Math.h"
#include "Camera/Camera.h"
#include "Camera/DebugCamera.h"
#include "Input/Input.h"
#include "ImGui/ImGuiManager.h"
#include <memory>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

namespace RyoEngine {

    void Initialize();
    void Finalize();

    WinApp* GetWinApp();
    DirectXCommon* GetDxCommon();
    TextureManager* GetTexManager();
    Object3dCommon* GetObject3dCommon();
    SpriteCommon* GetSpriteCommon();
    ReflectCommon* GetReflectCommon();
}