// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

function clickButtonWithText(texts) {
    const btns = Array.from(document.querySelectorAll('div, span, button, .menuItem')).filter(e => {
        if (e.children.length > 1) return false; // Ignore containers
        if (e.offsetParent === null) return false; // Must be visible
        const t = e.textContent.trim().toUpperCase();
        return texts.some(target => t === target || t.includes(target));
    });
    if (btns.length > 0) {
        btns[btns.length - 1].click(); // Usually the most nested one is the actual button
        return true;
    }
    return false;
}

function notifyStarted() {
    window.chrome.webview.postMessage(JSON.stringify({
        type: 'forwardIpcToAlt',
        action: 'rankedMatchStarted'
    }));
}

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                // まず FIND MATCH を探す
                if (clickButtonWithText(['FIND MATCH', 'START MATCH'])) {
                    notifyStarted();
                } else {
                    // なければRANKEDメニューを開く
                    if (window.showWindow) window.showWindow(50);
                    clickButtonWithText(['RANKED']);
                    
                    setTimeout(() => {
                        if (clickButtonWithText(['FIND MATCH', 'START MATCH'])) {
                            notifyStarted();
                        }
                    }, 1000);
                }
            } 
            else if (msg.action === 'cancelRankedMatch') {
                clickButtonWithText(['CANCEL', 'LEAVE', 'STOP']);
            }
        }
    } catch(e) {}
});

// マッチ完了を監視してAlt Windowへ通知
setInterval(() => {
    const isMatched = Array.from(document.querySelectorAll('*')).some(e => {
        if (e.children.length > 0) return false;
        if (e.offsetParent === null) return false;
        const t = e.textContent.trim().toUpperCase();
        return t === 'REJOIN' || t === 'MATCH FOUND!' || t === 'ACCEPT';
    });

    if (isMatched && !window.__kucMatchNotified) {
        window.__kucMatchNotified = true; // 重複送信防止
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'forwardIpcToAlt',
            action: 'rankedMatchFound'
        }));
        
        setTimeout(() => { window.__kucMatchNotified = false; }, 30000); // 30秒後にリセット
    }
}, 1000);
