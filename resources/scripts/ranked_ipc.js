// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

function clickDeepestVisible(texts) {
    const btns = Array.from(document.querySelectorAll('*')).filter(e => {
        const t = e.textContent.trim().toUpperCase();
        if (!texts.some(target => t === target || t.includes(target))) return false;
        const rect = e.getBoundingClientRect();
        if (rect.width === 0 || rect.height === 0 || rect.width > window.innerWidth * 0.8) return false;
        return true;
    });

    let deepest = null;
    let maxDepth = -1;
    
    for (const btn of btns) {
        let depth = 0;
        let p = btn;
        while(p) { depth++; p = p.parentElement; }
        if (depth > maxDepth) { maxDepth = depth; deepest = btn; }
    }
    
    if (deepest) {
        deepest.click();
        return true;
    }
    return false;
}

function notifyStarted() {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'forwardIpcToAlt',
            action: 'rankedMatchStarted'
        }));
    }
}

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                if (document.pointerLockElement) document.exitPointerLock();
                
                setTimeout(() => {
                    if (clickDeepestVisible(['FIND MATCH', 'START MATCH'])) {
                        notifyStarted();
                    } else {
                        if (window.showWindow) window.showWindow(50);
                        clickDeepestVisible(['RANKED']);
                        
                        setTimeout(() => {
                            if (clickDeepestVisible(['FIND MATCH', 'START MATCH'])) {
                                notifyStarted();
                            } else {
                                // 強制的に開始を通知（UIが存在しなくても裏でAPIを叩く前提の場合）
                                notifyStarted();
                            }
                        }, 1000);
                    }
                }, 100);
            } 
            else if (msg.action === 'cancelRankedMatch') {
                if (document.pointerLockElement) document.exitPointerLock();
                setTimeout(() => {
                    clickDeepestVisible(['CANCEL', 'LEAVE', 'STOP']);
                }, 100);
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
