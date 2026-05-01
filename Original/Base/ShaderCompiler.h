#pragma once
#include <dxcapi.h>
#include <wrl.h>
#include <string>

namespace Engine {
    class ShaderCompiler {
    public:
        void Initialize();
        Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::wstring& filePath, const wchar_t* profile);

    private:
        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    };
}