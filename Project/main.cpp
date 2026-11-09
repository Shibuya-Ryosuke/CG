#include <RyoEngine.h>
#include <Test/Test.h>
#include "Application/Application.h"

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    //std::unique_ptr<Framework> game = std::make_unique<Test::Test>();
    std::unique_ptr<Framework> game = std::make_unique<App::Application>();
    game->Run();
    return 0;
}