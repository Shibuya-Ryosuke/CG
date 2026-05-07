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
#include <memory>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

namespace Engine {

   

    extern WinApp* winApp_;
    extern DirectXCommon* dxCommon_;
    extern ShaderCompiler* shaderCompiler_;
    extern TextureManager* textureManager_;
    extern Object3dCommon* object3dCommon_;
    extern SpriteCommon* spriteCommon_;
    extern Audio* audio_;
}