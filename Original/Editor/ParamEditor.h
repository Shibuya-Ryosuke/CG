#pragma once

#include "../Math/Math.h"
#include <string>
#include <vector>
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

        // --- グループ階層管理 ---
        static void BeginGroup(const std::string& name, bool open = false);
        static void EndGroup();

        // --- 1. フラグ登録（bool専用） ---
        static void RegisterFlag(const std::string& name, bool* value);

        // --- 2. 数値・ベクトル値登録（スピード・下限・上限を指定可能） ---
        static void RegisterValue(const std::string& name, int32_t* value, float speed = 1.0f, int32_t min = 0, int32_t max = 100);
        static void RegisterValue(const std::string& name, uint32_t* value, float speed = 1.0f, uint32_t min = 0, uint32_t max = 100);
        static void RegisterValue(const std::string& name, float* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector2* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector3* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector4* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);

        // --- 3. Transform登録（scale, rotate, translateを上から順にグループ展開） ---
        static void RegisterValue(const std::string& name, Transform* value, float speed = 0.01f, float min = -100.0f, float max = 100.0f);

        // --- 4. カラー登録（ColorEdit用） ---
        static void RegisterColor(const std::string& name, Vector3* value);
        static void RegisterColor(const std::string& name, Vector4* value);

        // --- ImGui描画 ---
        static void DrawImGuiWindow(const char* windowName = "Parameter Editor");

        // --- JSON保存・読み込み ---
        static void Save(const std::string& filepath = "Resources/ApplicationResources/Json/Editor/paramEditor.json");
        static void Load(const std::string& filepath = "Resources/ApplicationResources/Json/Editor/paramEditor.json");

    private:
        ParamEditor() = default;
        ~ParamEditor() = default;

        // パラメータの種類
        enum class EntryType {
            Flag,
            Int32,
            UInt32,
            Float,
            Vector2,
            Vector3,
            Vector4,
            Color3,
            Color4,
            Group
        };

        // ツリー構造を構成するエントリ構造体
        struct ParamEntry {
            std::string name;
            EntryType type;
            void* ptr = nullptr;

            // 数値設定
            float speed = 1.0f;
            float minVal[4] = { 0, 0, 0, 0 };
            float maxVal[4] = { 100, 100, 100, 100 };
            int32_t minInt = 0;
            int32_t maxInt = 100;

            // グループ用
            bool defaultOpen = false;
            std::vector<ParamEntry> children;
        };

        // 内部実装用のメンバ関数
        void BeginGroupInternal(const std::string& name, bool open);
        void EndGroupInternal();
        void AddEntry(ParamEntry entry);

        void DrawImGuiInternal(const char* windowName);
        void DrawEntries(std::vector<ParamEntry>& entries);

        void SaveToJsonInternal(const std::string& filepath);
        void SaveEntriesToJson(nlohmann::json& j, const std::vector<ParamEntry>& entries);

        void LoadFromJsonInternal(const std::string& filepath);
        void LoadEntriesFromJson(const nlohmann::json& j, std::vector<ParamEntry>& entries);

        // ルートエントリと現在の階層スタック
        std::vector<ParamEntry> rootEntries_;
        std::vector<ParamEntry*> groupStack_;

        // 起動時に先読みしたJSONデータ
        nlohmann::json loadedJson_;
        bool isLoaded_ = false;
    };
}