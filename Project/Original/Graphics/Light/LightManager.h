#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_set>
#include "Light.h"
#include "../../Core/Math/Math.h"

namespace RyoEngine {

    constexpr uint32_t kMaxLightCount = 64;

    // ライト1個分の管理情報。
    // Light本体はGPUと1:1(64byte)なので触らず、名前・点灯フラグはこちらで持つ。
    struct LightEntry {
        std::string name;      // コードから引くための名前（重複不可）
        Light       light;     // GPUに渡す本体（設定値）
        bool        enabled = true; // falseのときGPUへ渡すコピーだけintensity=0にする（設定値は壊さない）
    };

    class LightManager {
    public:
        static LightManager* GetInstance();

        static void Initialize();
        static void Finalize();
        static void Update();

        // --- フォルダパス設定 ---
        static void SetFolderPath(const std::string& folderPath);
        static const std::string& GetFolderPath();

        // --- JSON 保存・読み込み ---
        static void Save();
        static void Load();

        // --- ライト操作（ゲームコードからは基本使わない。増減はImGuiで行う想定） ---
        static int AddLight(LightType type, const std::string& name = "");
        static int AddLight(const Light& light, const std::string& name = "");
        static void RemoveLight(int index);
        static void ClearLights();

        // =====================================================
        // 名前で引くAPI（ゲームコードはこちらを使う）
        // 存在しない名前の場合は何もせずreturnし、同じ名前につき1回だけ警告ログを出す
        // =====================================================
        static void SetLightPosition(const std::string& name, const Vector3& position);
        static void SetLightEnabled(const std::string& name, bool enabled);
        static void SetLightDirection(const std::string& name, const Vector3& direction);
        static void SetLightColor(const std::string& name, const Vector4& color);
        static void SetLightIntensity(const std::string& name, float intensity);
        static void SetLightRange(const std::string& name, float range);

        // 存在確認（警告ログなし）
        static bool HasLight(const std::string& name);
        // 名前からindexを取得（なければ-1、警告ログなし）
        static int FindLight(const std::string& name);

        // =====================================================
        // indexで引くAPI（ImGuiや旧コード向け）
        // =====================================================
        static size_t GetLightCount() { return GetInstance()->lights_.size(); }
        static const Light& GetLight(int index) { return GetInstance()->lights_[index].light; }
        static const std::string& GetLightName(int index) { return GetInstance()->lights_[index].name; }
        static bool IsLightEnabled(int index) { return GetInstance()->lights_[index].enabled; }

        static void SetLightEnabled(int index, bool enabled) {
            if (auto* l = GetEntryPtr(index)) l->enabled = enabled;
        }

        static void SetLightType(int index, LightType type) {
            if (auto* l = GetEntryPtr(index)) l->light.type = type;
        }
        static LightType GetLightType(int index) { return GetInstance()->lights_[index].light.type; }

        static void SetLightColor(int index, const Vector4& color) {
            if (auto* l = GetEntryPtr(index)) l->light.color = color;
        }
        static const Vector4& GetLightColor(int index) { return GetInstance()->lights_[index].light.color; }

        static void SetLightIntensity(int index, float intensity) {
            if (auto* l = GetEntryPtr(index)) l->light.intensity = intensity;
        }
        static float GetLightIntensity(int index) { return GetInstance()->lights_[index].light.intensity; }

        static void SetLightDirection(int index, const Vector3& direction) {
            if (auto* l = GetEntryPtr(index)) l->light.direction = Normalize(direction);
        }
        static const Vector3& GetLightDirection(int index) { return GetInstance()->lights_[index].light.direction; }

        static void SetLightPosition(int index, const Vector3& position) {
            if (auto* l = GetEntryPtr(index)) l->light.position = position;
        }
        static const Vector3& GetLightPosition(int index) { return GetInstance()->lights_[index].light.position; }

        static void SetLightRange(int index, float range) {
            if (auto* l = GetEntryPtr(index)) l->light.range = range;
        }
        static float GetLightRange(int index) { return GetInstance()->lights_[index].light.range; }

        static void SetLightSpotAngle(int index, float spotAngle) {
            if (auto* l = GetEntryPtr(index)) l->light.spotAngle = spotAngle;
        }
        static float GetLightSpotAngle(int index) { return GetInstance()->lights_[index].light.spotAngle; }

        static void SetLightSpotFalloff(int index, float spotFalloff) {
            if (auto* l = GetEntryPtr(index)) l->light.spotFalloff = spotFalloff;
        }
        static float GetLightSpotFalloff(int index) { return GetInstance()->lights_[index].light.spotFalloff; }

        // --- アンビエントライト ---
        static void SetAmbientLight(const AmbientLight& ambient) {
            GetInstance()->ambientData_->color = ambient.color;
            GetInstance()->ambientData_->intensity = ambient.intensity;
        }
        static void SetAmbientColor(const Vector4& color) { GetInstance()->ambientData_->color = color; }
        static void SetAmbientIntensity(float intensity) { GetInstance()->ambientData_->intensity = intensity; }

        static const AmbientLight& GetAmbientLight() { return *GetInstance()->ambientData_; }
        static const Vector4& GetAmbientColor() { return GetInstance()->ambientData_->color; }
        static float GetAmbientIntensity() { return GetInstance()->ambientData_->intensity; }

        // --- GPUリソース取得 ---
        static ID3D12Resource* GetLightResource() { return GetInstance()->lightResource_.Get(); }
        static ID3D12Resource* GetLightCountResource() { return GetInstance()->lightCountResource_.Get(); }
        static ID3D12Resource* GetAmbientResource() { return GetInstance()->ambientResource_.Get(); }

        static D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() { return GetInstance()->lightResource_->GetGPUVirtualAddress(); }
        static D3D12_GPU_VIRTUAL_ADDRESS GetLightCountGPUVirtualAddress() { return GetInstance()->lightCountResource_->GetGPUVirtualAddress(); }
        static D3D12_GPU_VIRTUAL_ADDRESS GetAmbientGPUVirtualAddress() { return GetInstance()->ambientResource_->GetGPUVirtualAddress(); }

        // --- 後方互換API ---
        static DirectionalLight GetDirectionalLight() {
            const Light& l = GetInstance()->lights_[0].light;
            return DirectionalLight{ l.color, l.direction, l.intensity };
        }
        static const Vector4& GetColor() { return GetInstance()->lights_[0].light.color; }
        static const Vector3& GetDirection() { return GetInstance()->lights_[0].light.direction; }
        static float GetIntensity() { return GetInstance()->lights_[0].light.intensity; }

        static void SetDirectionalLight(const DirectionalLight& light) {
            auto* inst = GetInstance();
            inst->lights_[0].light.type = LightType::Directional;
            inst->lights_[0].light.color = light.color;
            inst->lights_[0].light.direction = Normalize(light.direction);
            inst->lights_[0].light.intensity = light.intensity;
            Update();
        }
        static void SetColor(const Vector4& color) { GetInstance()->lights_[0].light.color = color; Update(); }
        static void SetDirection(const Vector3& direction) { GetInstance()->lights_[0].light.direction = Normalize(direction); Update(); }
        static void SetIntensity(float intensity) { GetInstance()->lights_[0].light.intensity = intensity; Update(); }

        static void DrawImGui();

    private:
        LightManager() = default;
        ~LightManager() = default;
        LightManager(const LightManager&) = delete;
        LightManager& operator=(const LightManager&) = delete;

        static inline const std::string kFileName = "lightManager.json";

        std::string GetFullFilePath() const;
        void SaveToFileInternal(const std::string& filePath);
        void LoadFromFileInternal(const std::string& filePath);

        // indexから取得（範囲外ならnullptr）
        static LightEntry* GetEntryPtr(int index) {
            auto& lights = GetInstance()->lights_;
            if (index < 0 || index >= static_cast<int>(lights.size())) return nullptr;
            return &lights[index];
        }

        // 名前から取得。見つからなければ警告ログ（同名は1回だけ）を出してnullptr
        static LightEntry* FindEntry(const std::string& name);

        // 名前の重複チェック（ignoreIndexは自分自身を除外するためのindex）
        bool IsNameUsed(const std::string& name, int ignoreIndex = -1) const;
        // 重複しない名前を作る（空ならLight、重複なら _1, _2 ... を付ける）
        std::string MakeUniqueName(const std::string& base, int ignoreIndex = -1) const;

        std::vector<LightEntry> lights_;

        // 「見つからない」警告を出した名前（毎フレーム呼ばれてもログが溢れないように）
        std::unordered_set<std::string> warnedNames_;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        Light* lightMappedData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightCountResource_;
        LightCountData* lightCountData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> ambientResource_;
        AmbientLight* ambientData_ = nullptr;

        // 保持するフォルダパス
        std::string folderPath_ = "Resources/ApplicationResources/TD2_1/Json/";

        // 上書き確認モーダル表示フラグ
        bool showOverwriteModal_ = false;

        // フォルダパス変更時の一時バッファ
        char folderPathBuffer_[256] = {};
    };
}