#include "FontLoader.h"
#include "Logger.h"
#include "../Graphics/TextureManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace RyoEngine {
    bool FontLoader::Initialize(const std::string& fntFilePath, const std::string& textureFilePath) {
        Logger::Log("Font loader : Initializing...\n");
        if (!LoadFnt(fntFilePath)) {
            return false;
        }

        textureHandle_ = TextureManager::GetInstance()->Load(textureFilePath);

        fontSprite_ = std::make_unique<Sprite>();
        fontSprite_->Initialize(textureHandle_);

        Logger::LogSuccess("Font loader : Initialized\n");
        return true;
    }

    bool FontLoader::LoadFnt(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::LogError("Font file loading failed\nSearched file path : " + filePath + "\n");
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string type;
            ss >> type; // 行の先頭（info, common, page, char など）を取得

            // 行全体の高さを取得
            if (type == "common") {
                std::string pair;
                while (ss >> pair) {
                    if (pair.rfind("lineHeight=", 0) == 0) {
                        lineHeight_ = std::stoi(pair.substr(11));
                    }
                }
            }
            // 各文字の情報をパース
            else if (type == "char") {
                FontChar fontChar;
                wchar_t charId = 0;

                std::string pair;
                while (ss >> pair) {
                    // 各キーに応じて数値を抽出
                    size_t pos = pair.find('=');
                    if (pos == std::string::npos) continue;

                    std::string key = pair.substr(0, pos);
                    int32_t value = std::stoi(pair.substr(pos + 1));

                    if (key == "id")       charId = static_cast<wchar_t>(value);
                    else if (key == "x")        fontChar.x = value;
                    else if (key == "y")        fontChar.y = value;
                    else if (key == "width")    fontChar.width = value;
                    else if (key == "height")   fontChar.height = value;
                    else if (key == "xoffset")  fontChar.xoffset = value;
                    else if (key == "yoffset")  fontChar.yoffset = value;
                    else if (key == "xadvance") fontChar.xadvance = value;
                }

                // マップに登録
                charMap_[charId] = fontChar;
            }
        }

        return true;
    }

    const FontChar* FontLoader::GetCharInfo(char character) const {
        auto it = charMap_.find(character);
        if (it != charMap_.end()) {
            return &it->second;
        }

        // 見つからなかった場合は '?' を探す
        it = charMap_.find('?');
        if (it != charMap_.end()) {
            return &it->second;
        }

        return nullptr; // 完全にデータがない場合
    }

    void FontLoader::ScreenPrint(const std::string& text, Vector2 position, float scale) {
        if (text.empty() || !fontSprite_) return;

        // 【変更】wstringへの変換を削除し、通常の std::string のまま処理する
        Vector2 currentPos = position;

        // char型 で1バイトずつシンプルにループを回す
        for (char c : text) {

            // 改行処理
            if (c == '\n') { // L'\n' から '\n' に変更
                currentPos.x = position.x;
                currentPos.y += lineHeight_ * scale;
                continue;
            }

            // 文字情報を取得（char型のcをそのまま渡してOK）
            const FontChar* info = GetCharInfo(c);
            if (!info) continue;

            // オフセットを考慮して描画座標を計算
            Vector2 drawPos;
            drawPos.x = currentPos.x + (info->xoffset * scale);
            drawPos.y = currentPos.y + (info->yoffset * scale);

            // Sprite クラスを操作して一文字切り抜いて描画
            fontSprite_->SetTexCrop(float(info->x), float(info->y), float(info->width), float(info->height));
            fontSprite_->SetTranslate(drawPos);
            fontSprite_->SetSize(Vector2(info->width * scale, info->height * scale));

            // 忘れずにスプライトの更新と描画を呼ぶ
            fontSprite_->Update();
            fontSprite_->Draw();

            // 次の文字のために、文字の横幅（進み量）分だけ右にずらす
            currentPos.x += info->xadvance * scale;
        }
    }
}