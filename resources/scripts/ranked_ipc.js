// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                // KrunkerのUIを開いてボタンを自動クリック
                if (window.showWindow) window.showWindow(50);
                setTimeout(() => {
                    const findBtns = Array.from(document.querySelectorAll('.button, div.button, .settingsBtn')).filter(e => 
                        e.textContent.toUpperCase().includes('FIND MATCH') || 
                        e.textContent.toUpperCase().includes('RANKED')
                    );
                    if (findBtns.length > 0) findBtns[findBtns.length - 1].click();
                }, 500);
            } 
            else if (msg.action === 'cancelRankedMatch') {
                const cancelBtn = Array.from(document.querySelectorAll('.button, div.button')).find(e => 
                    e.textContent.toUpperCase().includes('CANCEL') || 
                    e.textContent.toUpperCase().includes('LEAVE')
                );
                if (cancelBtn) cancelBtn.click();
            }
        }
    } catch(e) {}
});

// マッチ完了を監視してAlt Windowへ通知
setInterval(() => {
    const rejoinBtn = Array.from(document.querySelectorAll('.button, div.button')).find(e => 
        e.textContent.toUpperCase().includes('REJOIN') ||
        e.textContent.toUpperCase().includes('MATCH FOUND')
    );
    if (rejoinBtn && !window.__kucMatchNotified) {
        window.__kucMatchNotified = true; // 重複送信防止
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'forwardIpcToAlt',
            action: 'rankedMatchFound'
        }));
        
        setTimeout(() => { window.__kucMatchNotified = false; }, 30000); // 30秒後にリセット
    }
}, 1000);
