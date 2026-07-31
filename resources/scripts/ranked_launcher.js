// ============================================================
// Krunker Ultra Client - Ranked Window Launcher
// ============================================================

setInterval(() => {
    // 既存の「RANKED」ボタンを探して書き換える
    const btns = Array.from(document.querySelectorAll('*')).filter(e => {
        if (e.children.length > 2) return false;
        if (e.offsetParent === null) return false;
        const t = (e.textContent || '').trim().toUpperCase();
        return t === 'RANKED';
    });
    
    btns.forEach(btn => {
        if (!btn.dataset.kucHooked) {
            btn.dataset.kucHooked = "true";
            
            // 既存のイベントリスナーをバイパスするためにキャプチャフェーズでフック
            btn.addEventListener('click', (e) => {
                e.stopImmediatePropagation();
                e.stopPropagation();
                e.preventDefault();
                
                if (window.chrome && window.chrome.webview) {
                    window.chrome.webview.postMessage(JSON.stringify({
                        type: 'openAltWindow'
                    }));
                }
            }, true);
            
            btn.textContent = 'RANKED (ULTRA)';
            btn.style.color = '#ea580c';
            btn.style.fontWeight = 'bold';
        }
    });

    // もし既存のボタンが見つからなくても、画面上に強制的に開くボタンを配置する（フェイルセーフ）
    if (!document.getElementById('kuc-force-ranked-btn')) {
        const fallbackContainer = document.getElementById('menuItemContainer') || document.body;
        if (fallbackContainer) {
            const forceBtn = document.createElement('div');
            forceBtn.id = 'kuc-force-ranked-btn';
            forceBtn.style.position = 'fixed';
            forceBtn.style.top = '10px';
            forceBtn.style.left = '10px';
            forceBtn.style.zIndex = '999999';
            forceBtn.style.background = '#ea580c';
            forceBtn.style.color = 'white';
            forceBtn.style.padding = '10px 20px';
            forceBtn.style.borderRadius = '5px';
            forceBtn.style.cursor = 'pointer';
            forceBtn.style.fontWeight = 'bold';
            forceBtn.style.boxShadow = '0 0 10px rgba(0,0,0,0.5)';
            forceBtn.textContent = 'OPEN ULTRA RANKED';
            
            forceBtn.addEventListener('click', () => {
                if (window.chrome && window.chrome.webview) {
                    window.chrome.webview.postMessage(JSON.stringify({
                        type: 'openAltWindow'
                    }));
                }
            });
            
            document.body.appendChild(forceBtn);
        }
    }
}, 1000);
