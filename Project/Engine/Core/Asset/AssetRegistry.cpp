#include "AssetRegistry.h"
#include "../Base/Logger.h"
#include "../../Graphics/2D/TextureManager.h"
#include "../../Graphics/3D/ModelManager.h"
#include <algorithm>
#include <filesystem>
#include <system_error>

namespace RyoEngine {

    AssetRegistry* AssetRegistry::GetInstance() {
        static AssetRegistry instance;
        return &instance;
    }

    void AssetRegistry::Initialize(const std::string& rootPath) {
        AssetRegistry* instance = GetInstance();

        Logger::Log("AssetRegistry : Initializing...\n");
        instance->rootPath_ = Normalize(rootPath);
        if (!instance->rootPath_.empty() && instance->rootPath_.back() != '/') {
            instance->rootPath_ += '/';
        }
        instance->Scan();
        Logger::LogSuccess("AssetRegistry : Initialized\n");
    }

    void AssetRegistry::Refresh() {
        GetInstance()->Scan();
    }

    // ------------------------------------------------------------------
    // 走査
    // ------------------------------------------------------------------
    void AssetRegistry::Scan() {
        entries_.clear();
        for (auto& table : nameTables_) table.clear();
        for (auto& cache : resolveCaches_) cache.clear();

        namespace fs = std::filesystem;
        // 日本語のパスでも壊れないよう、wstring(UTF-16)経由で扱う (エンジン内の std::string は UTF-8)
        const fs::path root(Logger::ConvertString(rootPath_));

        std::error_code ec;
        if (!fs::is_directory(root, ec)) {
            Logger::LogWarning("[AssetRegistry] 走査するフォルダがありません: {}\n", rootPath_);
            return;
        }

        for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
            it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) {
                break;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }

            const std::string extensionLower = ToLower(Logger::ConvertString(it->path().extension().wstring()));
            Type type{};
            if (!TypeFromExtension(extensionLower, type)) {
                continue;
            }

            // 走査フォルダからの相対パス ("Textures/Player/Idle.png")
            const std::string relative = Normalize(Logger::ConvertString(it->path().lexically_relative(root).generic_wstring()));

            Entry entry;
            entry.type = type;
            entry.fullPath = rootPath_ + relative;
            entry.relativeLower = ToLower(relative);
            const size_t entryIndex = entries_.size();
            entries_.push_back(entry);

            // 名前の登録: "textures/player/idle" "player/idle" "idle" と、それぞれの拡張子付き
            const std::string withoutExtension = entry.relativeLower.substr(0, entry.relativeLower.size() - extensionLower.size());
            NameTable& table = nameTables_[static_cast<size_t>(type)];
            size_t start = 0;
            while (true) {
                table[withoutExtension.substr(start)].push_back(entryIndex);
                table[withoutExtension.substr(start) + extensionLower].push_back(entryIndex);
                const size_t slash = withoutExtension.find('/', start);
                if (slash == std::string::npos) {
                    break;
                }
                start = slash + 1;
            }
        }

        Logger::Log("AssetRegistry : {} 個のアセットを見つけました ({})\n", entries_.size(), rootPath_);
    }

    // ------------------------------------------------------------------
    // 名前 → パス
    // ------------------------------------------------------------------
    std::string AssetRegistry::Resolve(Type type, const std::string& nameOrPath) {
        AssetRegistry* instance = GetInstance();

        if (nameOrPath.empty() || type == Type::Count) {
            return {};
        }

        ResolveCache& cache = instance->resolveCaches_[static_cast<size_t>(type)];
        if (auto cached = cache.find(nameOrPath); cached != cache.end()) {
            return cached->second;
        }

        std::string result;
        const std::string normalized = Normalize(nameOrPath);

        if (FileExists(normalized)) {
            // 1. そのまま実在するパス
            result = normalized;
        } else if (FileExists(instance->rootPath_ + normalized)) {
            // 2. 走査フォルダからの相対パス
            result = instance->rootPath_ + normalized;
        } else {
            // 3. 名前として探す
            const NameTable& table = instance->nameTables_[static_cast<size_t>(type)];
            const auto found = table.find(ToLower(normalized));
            if (found == table.end()) {
                Logger::LogWarning("[AssetRegistry] 見つかりません: \"{}\" ({} 以下を探しました)\n", nameOrPath, instance->rootPath_);
            } else if (found->second.size() > 1) {
                std::string candidates;
                for (size_t index : found->second) {
                    candidates += "\n    " + instance->entries_[index].fullPath;
                }
                Logger::LogWarning("[AssetRegistry] \"{}\" は複数あります。フォルダ名を付けて指定してください (例: \"フォルダ名/{}\"){}\n",
                    nameOrPath, nameOrPath, candidates);
            } else {
                result = instance->entries_[found->second.front()].fullPath;
            }
        }

        // 見つからなかった場合も覚えておく (警告を毎回出さないため。Refresh()で忘れる)
        cache[nameOrPath] = result;
        return result;
    }

    // ------------------------------------------------------------------
    // 先読み
    // ------------------------------------------------------------------
    void AssetRegistry::Preload(const std::string& folder) {
        AssetRegistry* instance = GetInstance();

        std::string prefix = ToLower(Normalize(folder));
        if (!prefix.empty() && prefix.back() != '/') {
            prefix += '/';
        }

        size_t count = 0;
        for (const Entry& entry : instance->entries_) {
            if (entry.relativeLower.compare(0, prefix.size(), prefix) != 0) {
                continue;
            }
            switch (entry.type) {
            case Type::Texture:
                TextureManager::Load(entry.fullPath);
                ++count;
                break;
            case Type::Model:
                ModelManager::LoadObj(entry.fullPath);
                ++count;
                break;
            default:
                break; // 音は対象外
            }
        }
        Logger::Log("AssetRegistry : Preload \"{}\" ({} 個)\n", folder, count);
    }

    // ------------------------------------------------------------------
    // 補助
    // ------------------------------------------------------------------
    bool AssetRegistry::TypeFromExtension(const std::string& extensionLower, Type& outType) {
        if (extensionLower == ".png" || extensionLower == ".jpg" || extensionLower == ".jpeg" || extensionLower == ".dds") {
            outType = Type::Texture;
            return true;
        }
        if (extensionLower == ".obj" || extensionLower == ".gltf" || extensionLower == ".glb") {
            outType = Type::Model;
            return true;
        }
        if (extensionLower == ".wav" || extensionLower == ".mp3" || extensionLower == ".aac" || extensionLower == ".m4a" || extensionLower == ".wma") {
            outType = Type::Sound;
            return true;
        }
        if (extensionLower == ".hlsl") {
            outType = Type::Shader;
            return true;
        }
        return false;
    }

    std::string AssetRegistry::ToLower(const std::string& str) {
        std::string result = str;
        // ASCIIだけ小文字にする (日本語などのUTF-8のバイトはそのまま)
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
        return result;
    }

    std::string AssetRegistry::Normalize(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        while (result.rfind("./", 0) == 0) {
            result.erase(0, 2);
        }
        while (!result.empty() && result.front() == '/') {
            result.erase(0, 1);
        }
        return result;
    }

    bool AssetRegistry::FileExists(const std::string& utf8Path) {
        std::error_code ec;
        return std::filesystem::is_regular_file(std::filesystem::path(Logger::ConvertString(utf8Path)), ec);
    }
}
