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

        // --- 1. フラグ登録（bool専用） ---
        static void RegisterFlag(const std::string& name, bool* value);

        // --- 2. 数値・ベクトル値登録（スピード・下限・上限を指定可能。デフォルト: 1 / 0 / 100） ---
        static void RegisterValue(const std::string& name, int32_t* value, float speed = 1.0f, int32_t min = 0, int32_t max = 100);
        static void RegisterValue(const std::string& name, uint32_t* value, float speed = 1.0f, uint32_t min = 0, uint32_t max = 100);
        static void RegisterValue(const std::string& name, float* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector2* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector3* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector4* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);

        // --- 3. カラー登録（ColorEdit用） ---
        static void RegisterColor(const std::string& name, Vector3* value);
        static void RegisterColor(const std::string& name, Vector4* value);

        // --- ImGui描画 ---
        static void DrawImGuiWindow(const char* windowName = "Parameter Editor");

        // --- JSON保存・読み込み ---
        static void SaveToJson(const std::string& filepath = "Resources/ApplicationResources/Json/Editor");
        static void LoadFromJson(const std::string& filepath = "Resources/ApplicationResources/Json/Editor");

    private:
        ParamEditor() = default;
        ~ParamEditor() = default;

        // 内部実装用のメンバ関数
        void DrawImGuiInternal(const char* windowName);
        void SaveToJsonInternal(const std::string& filepath);
        void LoadFromJsonInternal(const std::string& filepath);

        // 値パラメータの描画および設定情報を保持するための構造体
        template <typename T>
        struct ValueParamInfo {
            T* ptr = nullptr;
            float speed = 1.0f;
            T min{};
            T max{};
        };

        // 各型ごとの保持マップ
        std::unordered_map<std::string, bool*> flagParams_;
        std::unordered_map<std::string, ValueParamInfo<int32_t>> int32Params_;
        std::unordered_map<std::string, ValueParamInfo<uint32_t>> uint32Params_;
        std::unordered_map<std::string, ValueParamInfo<float>> floatParams_;
        std::unordered_map<std::string, ValueParamInfo<Vector2>> vec2Params_;
        std::unordered_map<std::string, ValueParamInfo<Vector3>> vec3Params_;
        std::unordered_map<std::string, ValueParamInfo<Vector4>> vec4Params_;

        std::unordered_map<std::string, Vector3*> color3Params_;
        std::unordered_map<std::string, Vector4*> color4Params_;

        // 起動時に先読みしたJSONデータを一時保持するストレージ
        nlohmann::json loadedJson_;
        bool isLoaded_ = false;
    };
}