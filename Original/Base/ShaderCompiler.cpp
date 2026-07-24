#include <d3d12.h>
#include "ShaderCompiler.h"
#include "Logger.h"
#include <format>
#include <cassert>
#include <filesystem>
#include <initguid.h> 
#include <dxcapi.h>

#pragma comment(lib, "dxcompiler.lib")


namespace RyoEngine {
    ShaderCompiler* ShaderCompiler::GetInstance() {
        static ShaderCompiler instance;
        return &instance;
    }

    void ShaderCompiler::Initialize() {
        Logger::Log("ShaderCompiler : Initializing...\n");
        // DXCの初期化
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
        assert(SUCCEEDED(hr));

        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
        assert(SUCCEEDED(hr));

        // インクルードを処理するためのハンドラ
        hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
        assert(SUCCEEDED(hr));
        Logger::LogSuccess("ShaderCompiler : Initialized\n");
    }

    void ShaderCompiler::Finalize()
    {
        Logger::Log("ShaderCompiler : Finalizing...\n");
        // 保持しているリソースをすべて解放する
        includeHandler_.Reset();
        dxcCompiler_.Reset();
        dxcUtils_.Reset();
        Logger::LogSuccess("ShaderComiler : Finalized\n");
    }

    Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::Compile(const std::wstring& filePath, const wchar_t* profile) {

        // 1.hlslファイルを読み込む
        Logger::Log(Logger::ConvertString(std::format(L"* Begin CompileShader *\n- path:{}\n- profile:{}\n", filePath, profile)));

        // 【デバッグ用】プログラムが実際に探しに行っている絶対パスをログに出す
        std::filesystem::path absolutePath = std::filesystem::absolute(filePath);
        Logger::Log(Logger::ConvertString(std::format(L"Looking for file at: {}\n", absolutePath.wstring())));

        // hlslファイルを読み込む
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
        HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);

        // 読めなかったら止める
        if (FAILED(hr)) {
            // assertの前に、ファイルが存在するかチェック
            bool exists = std::filesystem::exists(filePath);
            Logger::Log(Logger::ConvertString(std::format(L"File exists? : {}\n", exists ? L"TRUE" : L"FALSE")));
            assert(SUCCEEDED(hr));
        }
        // 読み込んだファイルの内容を設定する
        DxcBuffer shaderSourceBuffer;
        shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
        shaderSourceBuffer.Size = shaderSource->GetBufferSize();
        shaderSourceBuffer.Encoding = DXC_CP_UTF8;  // UTF8のコードであることを通知

        // 2.Compileする
        LPCWSTR arguments[] = {
            filePath.c_str(),  // コンパイル対象のhlslファイル名
            L"-E", L"main",  // エントリーポイントの指定。基本的にmain以外にはしない
            L"-T", profile,  // ShaderProfileの設定
            L"-Zi", L"-Qembed_debug",  // デバッグ用の情報を埋め込む
            L"-Od",   // 最適化を外しておく
            L"-Zpr",  // メモリレイアウトは行優先
        };
        // 実際にShaderをコンパイルする
        Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
        hr = dxcCompiler_->Compile(
            &shaderSourceBuffer,  // 読み込んだファイル
            arguments,            // コンパイルオプション
            _countof(arguments),  // コンパイルオプションの数
            includeHandler_.Get(),       // includeが含まれた諸々
            IID_PPV_ARGS(&shaderResult)  // コンパイル結果
        );
        // コンパイルエラーではなくdxcが起動できないなど致命的な状況
        assert(SUCCEEDED(hr));

        // 3.警告・エラーが出てないか確認する
    // 警告・エラーが出てたらログに出して止める
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
        shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
        if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
            Logger::Log(shaderError->GetStringPointer());
            // 警告・エラーダメゼッタイ
            assert(false);
        }

        // 4.Compile結果を受け取って返す
        // コンパイル結果から実行用のバイナリ部分を取得
        Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
        hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        assert(SUCCEEDED(hr));
        // 成功したログを出す
        Logger::LogSuccess(Logger::ConvertString(std::format(L"* Compile Succeeded *\n- path:{}\n- profile{}\n", filePath, profile)));
        // 実行用のバイナリを返却
        return shaderBlob;
    }
}
