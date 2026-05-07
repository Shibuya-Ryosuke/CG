#include "OriginalEngine.h"

namespace Engine {
    // ここで実際に定義（初期化）する。これが「1つだけ」存在する実体になる
    WinApp* winApp_ = nullptr;
    DirectXCommon* dxCommon_ = nullptr;
    ShaderCompiler* shaderCompiler_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    Object3dCommon* object3dCommon_ = nullptr;
    SpriteCommon* spriteCommon_ = nullptr;
    Audio* audio_ = nullptr;
}