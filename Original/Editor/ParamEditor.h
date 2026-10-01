#pragma once

#include "../Math/Math.h"
#include <string>
#include <vector>
#include <list>
#include <json.hpp>

namespace RyoEngine {
    enum class Anchor;
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

        // --- フォルダパス設定 ---
        static void SetFolderPath(const std::string& folderPath);
        static const std::string& GetFolderPath();

        // --- グループ階層管理 ---
        static void BeginGroup(const std::string& name, bool open = false);
        static void EndGroup();

        // --- 1. フラグ登録（bool専用） ---
        static void RegisterFlag(const std::string& name, bool* value);

        // --- 2. 数値・ベクトル値登録 ---
        static void RegisterValue(const std::string& name, int32_t* value, float speed = 1.0f, int32_t min = 0, int32_t max = 100);
        static void RegisterValue(const std::string& name, uint32_t* value, float speed = 1.0f, uint32_t min = 0, uint32_t max = 100);
        static void RegisterValue(const std::string& name, float* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector2* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector3* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Vector4* value, float speed = 1.0f, float min = 0.0f, float max = 100.0f);

        // --- 3. Transform登録 ---
        static void RegisterValue(const std::string& name, Transform* value, float speed = 0.01f, float min = -100.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, Transform2D* value, float speed = 1.0f, float min = -100.0f, float max = 100.0f);
        static void RegisterValue(const std::string& name, UVTransform* value, float speed = 0.01f, float min = -100.0f, float max = 100.0f);

        // --- 4. カラー登録 ---
        static void RegisterColor(const std::string& name, Vector3* value);
        static void RegisterColor(const std::string& name, Vector4* value);

        // --- 5. アンカー登録 ---
        static void RegisterAnchor(const std::string& name, Anchor* value);

        // --- ImGui描画 ---
        static void DrawImGuiWindow(const char* windowName = "Parameter Editor");

        // --- JSON保存・読み込み ---
        static void Save();
        static void Load();

    private:
        ParamEditor() = default;
        ~ParamEditor() = default;

        // 固定ファイル名
        static inline const std::string kFileName = "paramEditor.json";

        // パラメータの種類
        enum class EntryType {
            Flag, Int32, UInt32, Float, Vector2, Vector3, Vector4, Color3, Color4, Anchor, Group
        };

        struct ParamEntry {
            std::string name;
            EntryType type;
            void* ptr = nullptr;

            float speed = 1.0f;
            float minVal[4] = { 0, 0, 0, 0 };
            float maxVal[4] = { 100, 100, 100, 100 };
            int32_t minInt = 0;
            int32_t maxInt = 100;

            bool defaultOpen = false;
            std::list<ParamEntry> children;
        };

        void BeginGroupInternal(const std::string& name, bool open);
        void EndGroupInternal();
        void AddEntry(ParamEntry entry);

        void DrawImGuiInternal(const char* windowName);
        void DrawEntries(std::list<ParamEntry>& entries);

        void SaveToJsonInternal(const std::string& filepath);
        void SaveEntriesToJson(nlohmann::json& j, const std::list<ParamEntry>& entries);

        void LoadFromJsonInternal(const std::string& filepath);
        void LoadEntriesFromJson(const nlohmann::json& j, std::list<ParamEntry>& entries);

        std::string GetFullFilePath() const;

        std::list<ParamEntry> rootEntries_;
        std::vector<ParamEntry*> groupStack_;

        nlohmann::json loadedJson_;
        bool isLoaded_ = false;

        // 保持するフォルダパス
        std::string folderPath_ = "Resources/ApplicationResources/Json/Editor";

        // 上書き確認モーダル表示フラグ
        bool showOverwriteModal_ = false;

        // フォルダパス変更時の一時バッファ
        char folderPathBuffer_[256] = {};
    };
}