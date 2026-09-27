#pragma once

#include "../Math/Math.h"
#include <string>
#include <unordered_map>
#include <json.hpp>

namespace RyoEngine {
    class ParamEditor {
    public:
        // シングルトンのインスタンス取得（内部用）
        static ParamEditor& GetInstance() {
            static ParamEditor instance;
            return instance;
        }

        // コピー・ムーブ禁止
        ParamEditor(const ParamEditor&) = delete;
        ParamEditor& operator=(const ParamEditor&) = delete;
        ParamEditor(ParamEditor&&) = delete;
        ParamEditor& operator=(ParamEditor&&) = delete;

        // --- パラメータ登録（変数の参照をバインドする） ---
        static void RegisterParam(const std::string& name, bool* value);
        static void RegisterParam(const std::string& name, int32_t* value);
        static void RegisterParam(const std::string& name, uint32_t* value);
        static void RegisterParam(const std::string& name, float* value);
        static void RegisterParam(const std::string& name, Vector2* value);
        static void RegisterParam(const std::string& name, Vector3* value);
        static void RegisterParam(const std::string& name, Vector4* value);

        // --- ImGui描画 ---
        static void DrawImGuiWindow(const char* windowName = "Parameter Editor");

        // --- JSON保存・読み込み ---
        static void SaveToJson(const std::string& filepath = "Resources/ApplicationResources/Json/Editor");
        static void LoadFromJson(const std::string& filepath);

    private:
        ParamEditor() = default;
        ~ParamEditor() = default;

        // 内部実装用のメンバ関数
        void DrawImGuiInternal(const char* windowName);
        void SaveToJsonInternal(const std::string& filepath);
        void LoadFromJsonInternal(const std::string& filepath);

        // 登録時にロード済みJSONから値を復元するためのテンプレートヘルパー
        template <typename T>
        void BindAndApply(const std::string& name, T* value, std::unordered_map<std::string, T*>& map);

        // 各型ごとのポインタ保持マップ
        std::unordered_map<std::string, bool*> boolParams_;
        std::unordered_map<std::string, int32_t*> int32Params_;
        std::unordered_map<std::string, uint32_t*> uint32Params_;
        std::unordered_map<std::string, float*> floatParams_;
        std::unordered_map<std::string, Vector2*> vec2Params_;
        std::unordered_map<std::string, Vector3*> vec3Params_;
        std::unordered_map<std::string, Vector4*> vec4Params_;

        // 起動時に先読みしたJSONデータを一時保持するストレージ
        nlohmann::json loadedJson_;
        bool isLoaded_ = false;
    };
}