// ============================================================
// Krunker Ultra Client - Ranked Window Launcher (F7 Keybind)
// ============================================================

document.addEventListener('keydown', (e) => {
    // F7 key pressed
    if (e.key === 'F7') {
        e.preventDefault();
        e.stopPropagation();
        if (window.chrome && window.chrome.webview) {
            window.chrome.webview.postMessage(JSON.stringify({
                type: 'openAltWindow'
            }));
        }
    }
}, true);
