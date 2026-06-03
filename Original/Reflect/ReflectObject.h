#pragma once
#include "../3d/Object3d.h"
#include "../Camera/Camera.h"
#include <memory>
#include <d3d12.h>
#include <wrl.h>
#include "../Camera/DebugCamera.h"

namespace Engine {
    struct TransformationMatrixForReflect {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 ReflectVP;
    };
    class ReflectObject {
    private:
        struct ReflectWvpResource {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            TransformationMatrixForReflect* data = nullptr;
        };
        std::vector<ReflectWvpResource> reflectWvpResources_;

        Object3d* object_ = nullptr; // 鏡の実体（板モデル）

        // 反射面を定義する（とりあえず Y=0 の平面とするための法線）
        Vector3 planeNormal_ = { 0.0f, 1.0f, 0.0f };
        float planeDistance_ = 0.0f; // 原点からの距離

        void UpdateObject3d(const Camera& Camera, Object3d* target, TransformationMatrixForReflect* data);
        void UpdateObject3d(const DebugCamera& debugCamera, Object3d* target, TransformationMatrixForReflect* data);
        void DrawObject3d(Object3d* target, size_t index);
        void CreateReflectionResource();
        
        Microsoft::WRL::ComPtr<ID3D12Resource> reflectionResource_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        uint32_t srvIndex_ = 0;

        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle_{};
       
        Matrix4x4 CalculateReflectionViewProjection(const DebugCamera& debugCamera);

        std::vector<Object3d*> drawObjects_;

    public:

        ReflectObject() = default;
        ~ReflectObject() = default;

        // --- コピー禁止の設定 ---
        // コピーコンストラクタを削除
        ReflectObject(const ReflectObject&) = delete;

        // コピー代入演算子を削除
        ReflectObject& operator=(const ReflectObject&) = delete;

        // --- (任意) 移動の設定 ---
        // 所有権を移動させる「ムーブ」は許可しておくと便利な場合があります
        ReflectObject(ReflectObject&&) = default;
        ReflectObject& operator=(ReflectObject&&) = default;


        // 初期化（板ポリゴンのモデルなどを読み込む）
        void Initialize(const std::string& modelPath);

        // 更新（メインカメラを元に、鏡用の反転カメラ行列を計算する）
        void Update(const Camera& camera);
        void Update(const DebugCamera& debugCamera);

        // 描画（メインシーンの描画中に呼び出す）
        void Draw();

        // 追加：リソース生成用
        ReflectWvpResource CreateSingleReflectWvpResource();
        void RegisterObject(Object3d* obj);

        /// UpdateとDraw
        void ReflectProcess(const Camera& camera);
        void ReflectProcess(const DebugCamera& debugCamera);

        // Getter
        // 鏡ごとのRTVハンドルを取得（PreDrawに渡す用）
        D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const {
            return rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        }

        // 鏡ごとのテクスチャリソースを取得（PostDrawに渡す用）
        ID3D12Resource* GetResource() const { return reflectionResource_.Get(); }
        // 鏡ごとのSRVインデックスを取得（Draw時のテクスチャ割り当て用）
        uint32_t GetSrvIndex() const { return srvIndex_; }
        // ✨ 鏡が持つ反射用バッファのGPUアドレスを返すゲッター
        D3D12_GPU_VIRTUAL_ADDRESS GetReflectWvpGPUAddress(size_t index) const {
            return reflectWvpResources_[index].resource->GetGPUVirtualAddress();
        }
        // ✨ 鏡の中の行列データを直接書き換えるためのポインタを返すゲッター
        TransformationMatrixForReflect* GetReflectWvpData(size_t index) { return reflectWvpResources_[index].data; }
  
        Matrix4x4& GetWorldMatrix() { return GetObj().GetWorldMatrix(); }
        Object3d& GetObj() { return *object_; };
        const Vector3& GetScale() const { return object_->GetScale(); }
        const Vector3& GetRotate() const { return object_->GetRotate(); }
        const Vector3& GetTranslate() const { return object_->GetTranslate(); }


        // --- セッター ---
        void SetTranslate(const Vector3& translate) { object_->SetTranslate(translate); }
        void SetRotate(const Vector3& rotate) { object_->SetRotate(rotate); }
        void SetScale(const Vector3& scale) { object_->SetScale(scale); }
    };

}