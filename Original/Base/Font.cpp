#include "Font.h"
#include "Logger.h"
#include "../Graphics/TextureManager.h"
#include "../2D/SpriteCommon.h"
#include "DirectXCommon.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace RyoEngine {
    void Font::Initialize(const std::string& fntFilePath, const std::string& textureFilePath) {
        Logger::Log("Font loader : Initializing...\n");
        CreateResource();

        LoadFnt(fntFilePath);

        textureHandle_ = TextureManager::GetInstance()->Load(textureFilePath);

        Logger::LogSuccess("Font loader : Initialized\n");
    }

    void Font::Finalize() {
        Logger::Log("Font : Finalizing...\n");
        if (vertexResource_) {
            vertexResource_->Unmap(0, nullptr);
            vertexResource_.Reset();
        }
        vertexData_ = nullptr;

        if (indexResource_) {
            indexResource_->Unmap(0, nullptr);
            indexResource_.Reset();
        }
        indexData_ = nullptr;

        if (materialResource_) {
            materialResource_->Unmap(0, nullptr);
            materialResource_.Reset();
        }
        materialData_ = nullptr;

        if (wvpResource_) {
            wvpResource_->Unmap(0, nullptr);
            wvpResource_.Reset();
        }
        wvpData_ = nullptr;
        Logger::LogSuccess("Font : Finalized\n");
    }

    void Font::CreateResource() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // ----------------------------------------------------
        // 1. 頂点リソースの作成とマッピング
        // ----------------------------------------------------
        // 1文字あたり4頂点必要
        size_t vertexBufferSize = sizeof(SpriteVertexData) * 4 * MAX_CHARS;
        vertexResource_ = DirectXCommon::CreateBufferResource(device, vertexBufferSize);

        // 頂点バッファビューの設定
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
        vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

        // CPUから書き込むためのポインタを取得（常時Map）
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));


        // ----------------------------------------------------
        // 2. インデックスリソースの作成とマッピング
        // ----------------------------------------------------
        // 1文字あたり6インデックス（三角形2個分）必要
        size_t indexBufferSize = sizeof(uint32_t) * 6 * MAX_CHARS;
        indexResource_ = DirectXCommon::CreateBufferResource(device, indexBufferSize);

        // インデックスバッファビューの設定
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        // CPUから書き込むためのポインタを取得（常時Map）
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));


        // ----------------------------------------------------
        // 3. マテリアルリソースの作成と初期化
        // ----------------------------------------------------
        // マテリアルは全体で1つ（色とUV行列）
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(SpriteMaterial));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

        // 初期値設定
        materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialData_->uvTransform = MakeIdentity4x4(); // UVは頂点側で直接制御するので等倍で固定


        // ----------------------------------------------------
        // 4. WVP(行列)リソースの作成と初期化
        // ----------------------------------------------------
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));

        // スプライトは頂点座標自体がすでにスクリーン座標系(または2D空間)で計算されるため、
        // WVP行列には正投影行列（Orthographic）をあらかじめ入れておきます。
        *wvpData_ = MakeOrthographicMatrix(
            0.0f, 0.0f,
            (float)DirectXCommon::GetInstance()->GetBackBufferWidth(),
            (float)DirectXCommon::GetInstance()->GetBackBufferHeight(),
            0.0f, 100.0f
        );
    }

    bool Font::LoadFnt(const std::string& filePath) {
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

    const FontChar* Font::GetCharInfo(char character) const {
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

    void Font::RegisterText(const std::string& text, Vector2 position, float scale) {
        if (text.empty()) return;
        drawCalls_.push_back({ text, position, scale });
    }
    void Font::DrawAllText() {
        if (drawCalls_.empty()) return;

        uint32_t charCount = 0;
        float texWidth = 256.0;
        float texHeight = 256.0f;

        // たまった描画リクエストを順番に処理
        for (const auto& call : drawCalls_) {
            Vector2 currentPos = call.position;

            for (char c : call.text) {
                if (charCount >= MAX_CHARS) break;

                if (c == '\n') {
                    currentPos.x = call.position.x;
                    currentPos.y += lineHeight_ * call.scale;
                    continue;
                }

                const FontChar* info = GetCharInfo(c);
                if (!info) continue;

                float left = currentPos.x + (info->xoffset * call.scale);
                float right = left + (info->width * call.scale);
                float top = currentPos.y + (info->yoffset * call.scale);
                float bottom = top + (info->height * call.scale);

                float uLeft = info->x / texWidth;
                float uRight = (info->x + info->width) / texWidth;
                float vTop = info->y / texHeight;
                float vBottom = (info->y + info->height) / texHeight;

                uint32_t vIdx = charCount * 4;

                // 累積された charCount の位置に書き込んでいくので、データが衝突しない
                vertexData_[vIdx + 0].position = { left,  bottom, 0.0f, 1.0f };
                vertexData_[vIdx + 1].position = { left,  top,    0.0f, 1.0f };
                vertexData_[vIdx + 2].position = { right, bottom, 0.0f, 1.0f };
                vertexData_[vIdx + 3].position = { right, top,    0.0f, 1.0f };

                vertexData_[vIdx + 0].texcoord = { uLeft,  vBottom };
                vertexData_[vIdx + 1].texcoord = { uLeft,  vTop };
                vertexData_[vIdx + 2].texcoord = { uRight, vBottom };
                vertexData_[vIdx + 3].texcoord = { uRight, vTop };

                uint32_t iIdx = charCount * 6;
                indexData_[iIdx + 0] = vIdx + 0; indexData_[iIdx + 1] = vIdx + 1; indexData_[iIdx + 2] = vIdx + 2;
                indexData_[iIdx + 3] = vIdx + 1; indexData_[iIdx + 4] = vIdx + 3; indexData_[iIdx + 5] = vIdx + 2;

                charCount++;
                currentPos.x += info->xadvance * call.scale;
            }

            if (charCount >= MAX_CHARS) break;
        }

        // 文字が1文字以上あれば描画コマンドを積む
        if (charCount > 0) {
            *wvpData_ = MakeOrthographicMatrix(
                0.0f, 0.0f,
                (float)DirectXCommon::GetInstance()->GetBackBufferWidth(),
                (float)DirectXCommon::GetInstance()->GetBackBufferHeight(),
                0.0f, 100.0f
            );
            materialData_->uvTransform = MakeIdentity4x4();

            auto commandList = DirectXCommon::GetInstance()->GetCommandList();

            // リソースバインドとドローコール（前のターンの通り、Begin2dDraw側でPSO設定されていればバインドのみでOK）
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
            commandList->IASetIndexBuffer(&indexBufferView_);
            commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(textureHandle_));

            // 全ての合計文字数を一発でドロー！
            commandList->DrawIndexedInstanced(charCount * 6, 1, 0, 0, 0);
        }

        // 描画が終わったら、今フレームのリクエストをクリアして来フレームに備える
        drawCalls_.clear();
    }
}