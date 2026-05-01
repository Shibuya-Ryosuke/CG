#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>
#include"../Math/Math.h"

namespace Engine {
    class Object3d {
    public:
        // 初期化と描画
       // void Initialize();
        //void Draw(uint32_t textureHandle);

        // セッター
        void SetScale(const Vector3& scale) { transform_.scale = scale; }
        void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
        void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

    private:

        Transform transform_;

        // --- GPUリソース ---
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        TransformationMatrix* wvpData_ = nullptr;

        // 頂点バッファビュー
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};


        // 頂点バッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

        // インデックスバッファ (Obj読み込みなら必須)
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

        // マテリアル（色やライトの設定）用
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
        Material* materialData_ = nullptr;

        // 座標変換行列（WVP）用
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        TransformationMatrix* wvpData_ = nullptr;
    };
}