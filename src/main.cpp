// ============================================================
// main.cpp — エントリーポイント
// ============================================================
// WIN32サブシステム (コンソール非表示) で動作。
// COM初期化はApp::Run内で行う。
// ============================================================

#include "App.hpp"
#include <windows.h>

// ============================================================
// 高性能GPU (NVIDIA/AMD) を強制的に使用させるフラグ
// ※ WebView2プロセスには直接影響しない可能性がありますが、
//    OSのグラフィックススケジューラに対する強いヒントになります。
// ============================================================
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_     LPWSTR    /*lpCmdLine*/,
    _In_     int       /*nCmdShow*/)
{
    return app::App::Instance().Run(hInstance);
}
