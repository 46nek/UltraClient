// ==========================================
// Auto Memory Cleaner (Garbage Collection)
// Krunkerを長時間プレイしても重くならないように、
// V8エンジンのガベージコレクションを定期的に強制実行する
// ==========================================
(function() {
    console.log("[Auto Memory Cleaner] Initializing...");

    // --expose-gc フラグが有効な場合のみ window.gc が存在する
    if (typeof window.gc !== 'function') {
        console.warn("[Auto Memory Cleaner] window.gc is not available. Did you add --js-flags='--expose-gc'?");
        return;
    }

    // 3分（180,000ミリ秒）ごとにメモリ掃除を実行
    const GC_INTERVAL_MS = 180000;

    setInterval(() => {
        try {
            window.gc();
            console.log("[Auto Memory Cleaner] Garbage Collection triggered successfully.");
        } catch (e) {
            console.error("[Auto Memory Cleaner] Failed to trigger GC:", e);
        }
    }, GC_INTERVAL_MS);

    console.log("[Auto Memory Cleaner] Active. Will clean memory every 3 minutes.");
})();
