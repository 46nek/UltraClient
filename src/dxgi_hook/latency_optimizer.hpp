// ============================================================
// latency_optimizer.hpp — DWM/GPU レイテンシ最適化
// ============================================================
#pragma once
#include <windows.h>
#include <string>

namespace dxgi_hook {

// DWMのウィンドウ合成遅延を最小化する
void OptimizeWindowLatency(HWND hwnd);

// Chromiumに渡すレイテンシ関連の引数を追加する
void AppendLatencyArgs(std::wstring& args);

} // namespace dxgi_hook
