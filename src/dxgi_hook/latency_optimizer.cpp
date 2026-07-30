// ============================================================
// latency_optimizer.cpp — DWM/GPU レイテンシ最適化
// ============================================================
// DLL注入の代わりに、DWM API + Chromiumフラグで
// 表示レイテンシを最小化する。
// ============================================================

#include "latency_optimizer.hpp"
#include "util/Logger.hpp"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <string>

namespace dxgi_hook {

// ============================================================
// DWM ウィンドウ合成遅延の最小化
// ============================================================
void OptimizeWindowLatency(HWND hwnd) {
    if (!hwnd) return;

    // 1. ウィンドウ遷移アニメーションを無効化 (合成遅延を1フレーム削減)
    BOOL disableTransitions = TRUE;
    ::DwmSetWindowAttribute(hwnd,
        DWMWA_TRANSITIONS_FORCEDISABLED,
        &disableTransitions, sizeof(disableTransitions));

    // 2. 非クライアント領域レンダリングを無効化 (タイトルバー描画をスキップ)
    DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
    ::DwmSetWindowAttribute(hwnd,
        DWMWA_NCRENDERING_POLICY,
        &ncrp, sizeof(ncrp));

    // 3. DWM フレームタイミングの取得 (デバッグ/確認用)
    DWM_TIMING_INFO ti = {};
    ti.cbSize = sizeof(ti);
    if (SUCCEEDED(::DwmGetCompositionTimingInfo(hwnd, &ti))) {
        LOG_INFO("LatencyOptimizer: DWM refresh rate = " +
                 std::to_string(ti.rateRefresh.uiNumerator) + "/" +
                 std::to_string(ti.rateRefresh.uiDenominator));
    }

    // 4. DWMFlush で即座に合成を実行
    ::DwmFlush();

    LOG_INFO("LatencyOptimizer: DWM window attributes optimized.");
}

// ============================================================
// Chromium レイテンシ関連引数の追加
// ============================================================
void AppendLatencyArgs(std::wstring& args) {
    // FPS制限とVSyncを強制的に無効化 (低遅延/高フレームレート用)
    // args += L"--disable-frame-rate-limit "; // 射撃時のPing跳ね上がり・フリーズの原因になるため無効化
    args += L"--disable-gpu-vsync ";

    // GPUサンドボックスを緩和 (より直接的なGPUアクセス)
    args += L"--disable-gpu-sandbox ";

    // ゼロコピーテクスチャアップロード (メモリコピー削減)
    args += L"--enable-zero-copy ";

    // GPUドライバのバグ回避ワークアラウンドを無効化 (余計なオーバーヘッド削減)
    args += L"--disable-gpu-driver-bug-workarounds ";

    // レンダラーの優先度を高く設定
    args += L"--renderer-process-limit=1 ";

    LOG_INFO("LatencyOptimizer: Appended latency reduction arguments.");
}

} // namespace dxgi_hook
