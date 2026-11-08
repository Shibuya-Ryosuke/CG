#pragma once
#include <dxcapi.h>
#include <wrl.h>
#include <string>
#include <vector>

namespace RyoEngine {
    class ShaderCompiler {
    public:
        static void Initialize();
        static void Finalize();
        static Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::wstring& filePath, const wchar_t* profile);

        /// <summary>
        /// メモリ上のソース(UTF-8)をコンパイルする。Compile と違い、エラーでも止めずに nullptr を返す
        /// (アプリのシェーダーのように、間違いがあっても実行を続けたいとき用)。警告はログに出すだけ。
        /// </summary>
        /// <param name="source">HLSLのソース (UTF-8)</param>
        /// <param name="sourceName">ログ・エラー表示に使う名前</param>
        /// <param name="profile">vs_6_0 など</param>
        /// <param name="includeDirs">#include を探すフォルダ</param>
        /// <param name="outError">失敗したときのエラー文</param>
        static Microsoft::WRL::ComPtr<IDxcBlob> CompileSource(const std::string& source, const std::wstring& sourceName,
            const wchar_t* profile, const std::vector<std::wstring>& includeDirs, std::string& outError);

    private:
        static ShaderCompiler* GetInstance();

        ShaderCompiler() = default;
        ~ShaderCompiler() = default;
        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;

        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    };
}