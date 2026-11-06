#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RyoEngine {

    /// <summary>
    /// アセットフォルダ(既定: Resources/ApplicationResources/)を起動時に走査し、
    /// 「名前 → ファイルパス」の一覧を作るクラス。名前でアセットを呼べるようにする。
    ///
    /// ・名前は拡張子を除いたファイル名 ("Textures/Player.png" なら "Player")。大文字小文字は区別しない
    /// ・同じ名前が複数ある場合は、フォルダ名を付けると呼べる ("Player/Idle", "Enemy/Idle" など。何階層付けてもよい)
    /// ・拡張子付きでも呼べる ("Idle.png")
    /// ・今までのパス指定もそのまま使える (実在するファイルのパスなら、名前としては探さない)
    /// ・走査するのは一覧だけで、読み込みは各 Load が呼ばれたときに行う (2回目以降は各Managerのキャッシュが返る)
    ///
    /// 種類は拡張子で判定する
    ///   Texture : .png .jpg .jpeg .dds
    ///   Model   : .obj .gltf .glb
    ///   Sound   : .wav .mp3 .aac .m4a .wma
    /// </summary>
    class AssetRegistry {
    public:
        enum class Type {
            Texture,
            Model,
            Sound,
            Count,
        };

        /// <summary>
        /// 走査するフォルダを設定して走査する。RyoEngine::Initialize から呼ばれる (ゲーム側は呼ばなくてよい)
        /// </summary>
        static void Initialize(const std::string& rootPath = "Resources/ApplicationResources/");

        /// <summary>
        /// フォルダを走査し直す (実行中にファイルを追加・削除したとき用)
        /// </summary>
        static void Refresh();

        /// <summary>
        /// 名前またはパスから、読み込みに使うファイルパスを返す。見つからない・候補が複数ある場合は空文字 (警告は1回だけ出す)。
        /// 調べる順番:
        ///   1. そのまま実在するファイルのパスか ("Resources/EngineResources/..." など)
        ///   2. 走査フォルダからの相対パスとして実在するか (今までの RyoEngine::LoadTex 等の書き方)
        ///   3. 名前として一覧から探す
        /// 同じ入力の結果は覚えておくので、毎フレーム呼んでもファイルを調べ直さない。
        /// </summary>
        static std::string Resolve(Type type, const std::string& nameOrPath);

        /// <summary>
        /// 指定フォルダ(走査フォルダからの相対。例: "Stage1")の中のテクスチャとモデルを先に読み込む。
        /// 初めて使った瞬間のカクつきを避けたいとき、シーンの初期化などで呼ぶ。
        /// 空文字なら全部。テクスチャはsRGBとして読む。
        /// NOTE: 音はBGM/SEのどちらとして読むか決められないので対象外 (使う側で LoadBGM / LoadSE を呼ぶこと)
        /// </summary>
        static void Preload(const std::string& folder);

        static const std::string& GetRootPath() { return GetInstance()->rootPath_; }

    private:
        static AssetRegistry* GetInstance();

        AssetRegistry() = default;
        ~AssetRegistry() = default;
        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;

        // 走査で見つけたファイル1つ分
        struct Entry {
            Type type = Type::Texture;
            std::string fullPath;      // 読み込みに使うパス (走査フォルダ + 相対パス。区切りは '/')
            std::string relativeLower; // 走査フォルダからの相対パス(拡張子付き・小文字)。Preloadのフォルダ判定用
        };

        void Scan();
        // 拡張子(小文字・ドット付き)から種類を判定する。対象外なら false
        static bool TypeFromExtension(const std::string& extensionLower, Type& outType);
        static std::string ToLower(const std::string& str);
        // '\\' を '/' にそろえ、先頭の "./" や '/' を取り除く
        static std::string Normalize(const std::string& path);
        static bool FileExists(const std::string& utf8Path);

        std::string rootPath_ = "Resources/ApplicationResources/";
        std::vector<Entry> entries_;

        // 種類ごとの「名前(小文字) → entries_の添字」。1つの名前に複数あれば曖昧
        using NameTable = std::unordered_map<std::string, std::vector<size_t>>;
        std::array<NameTable, static_cast<size_t>(Type::Count)> nameTables_;

        // Resolve の結果のキャッシュ (入力そのまま → 結果。見つからなかった場合は空文字を覚える)
        using ResolveCache = std::unordered_map<std::string, std::string>;
        std::array<ResolveCache, static_cast<size_t>(Type::Count)> resolveCaches_;
    };
}
