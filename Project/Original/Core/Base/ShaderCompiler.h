#pragma once
#include <dxcapi.h>
#include <wrl.h>
#include <string>

namespace RyoEngine {
    class ShaderCompiler {
    public:
        static void Initialize();
        static void Finalize();
        static Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::wstring& filePath, const wchar_t* profile);

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