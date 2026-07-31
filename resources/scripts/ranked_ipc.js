// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

function clickDeepest(texts) {
    const btns = Array.from(document.querySelectorAll('*')).filter(e => {
        const t = e.textContent.trim().toUpperCase();
        return texts.some(target => t === target || t.includes(target));
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

function findRankedWindowId() {
    if (!window.windows) return -1;
    for (let i = 0; i < window.windows.length; i++) {
        let w = window.windows[i];
        if (w && w.header && w.header.toUpperCase().includes('RANKED')) return i;
    }
    for (let i = 0; i < window.windows.length; i++) {
        let w = window.windows[i];
        if (w && w.build) {
            try {
                let html = w.build().toUpperCase();
                if (html.includes('FIND MATCH') && html.includes('RANKED')) return i;
            } catch(e) {}
        }
    }
    return -1;
}

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                let rId = findRankedWindowId();
                if (rId !== -1 && window.windows[rId].build) {
                    try {
                        let html = window.windows[rId].build();
                        // Extract the onclick from FIND MATCH button
                        let matchStr = html.match(/onclick="([^"]+)"[^>]*>.*?FIND MATCH/i) || 
                                       html.match(/onclick="([^"]+)"[^>]*>.*?START MATCH/i);
                        if (matchStr) {
                            eval(matchStr[1]);
                        } else {
                            // If we can't extract, try dummy div
                            let dummyDiv = document.createElement('div');
                            dummyDiv.style.display = 'none';
                            dummyDiv.innerHTML = html;
                            document.body.appendChild(dummyDiv);
                            
                            const btns = Array.from(dummyDiv.querySelectorAll('*')).filter(e => {
                                const t = e.textContent.trim().toUpperCase();
                                return ['FIND MATCH', 'START MATCH'].some(target => t === target || t.includes(target));
                            });
                            if (btns.length > 0) btns[0].click();
                            dummyDiv.remove();
                        }
                    } catch(e) {}
                } else {
                    // Try to click visible buttons if window is already open
                    clickDeepest(['FIND MATCH', 'START MATCH']);
                }
                notifyStarted();
            } 
            else if (msg.action === 'cancelRankedMatch') {
                let rId = findRankedWindowId();
                if (rId !== -1 && window.windows[rId].build) {
                    try {
                        let html = window.windows[rId].build();
                        let matchStr = html.match(/onclick="([^"]+)"[^>]*>.*?CANCEL/i) || 
                                       html.match(/onclick="([^"]+)"[^>]*>.*?LEAVE/i);
                        if (matchStr) eval(matchStr[1]);
                    } catch(e) {}
                }
                clickDeepest(['CANCEL', 'LEAVE', 'STOP']);
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
