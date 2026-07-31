// ============================================================
// Krunker Ultra Client - Ranked Window Launcher
// ============================================================

setInterval(() => {
    // メインメニューのRANKEDボタンをフックして、専用ウィンドウを開くようにする
    const menuBtns = Array.from(document.querySelectorAll('.menuItem')).filter(e => 
        e.textContent.toUpperCase().includes('RANKED')
    );
    
    menuBtns.forEach(btn => {
        if (!btn.dataset.kucHooked) {
            btn.dataset.kucHooked = "true";
            
            // Krunkerのデフォルトクリックイベントを無効化する
            const clone = btn.cloneNode(true);
            if (btn.parentNode) {
                btn.parentNode.replaceChild(clone, btn);
            }
            
            clone.addEventListener('click', (e) => {
                e.stopPropagation();
                e.preventDefault();
                
                // C++側にAlt Windowを起動するよう指示
                if (window.chrome && window.chrome.webview) {
                    window.chrome.webview.postMessage(JSON.stringify({
                        type: 'openAltWindow'
                    }));
                }
            });
            
            // UIにUltra Client仕様であることを示す
            const label = clone.querySelector('.menuItemTitle') || clone;
            if (label && !label.textContent.includes('ULTRA')) {
                label.textContent = 'RANKED (ULTRA)';
                clone.style.color = '#ea580c'; // KUC Orange
            }
        }
    });
}, 1000);
