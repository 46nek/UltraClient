// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

function clickDeepest(texts) {
    try {
        const btns = Array.from(document.querySelectorAll('*')).filter(e => {
            const t = (e.textContent || '').trim().toUpperCase();
            if (!t) return false;
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
    } catch(e) {}
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

function notifyError(msg) {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'forwardIpcToAlt',
            action: 'rankedMatchError',
            message: msg
        }));
    }
}

function findRankedWindowId() {
    if (!window.windows) return -1;
    for (let i = 0; i < window.windows.length; i++) {
        try {
            let w = window.windows[i];
            if (w && typeof w.header === 'string' && w.header.toUpperCase().includes('RANKED')) return i;
        } catch(e) {}
    }
    for (let i = 0; i < window.windows.length; i++) {
        try {
            let w = window.windows[i];
            if (w && typeof w.build === 'function') {
                let html = w.build();
                if (typeof html === 'string') {
                    html = html.toUpperCase();
                    if (html.includes('FIND MATCH') && html.includes('RANKED')) return i;
                }
            }
        } catch(e) {}
    }
    return -1;
}

// Function to simulate clicking the main menu Ranked button
function clickMainMenuRanked() {
    return clickDeepest(['RANKED']);
}

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                try {
                    let success = false;
                    let rId = findRankedWindowId();
                    
                    if (rId !== -1 && window.windows[rId].build) {
                        try {
                            let html = window.windows[rId].build();
                            let matchStr = html.match(/onclick="([^"]+)"[^>]*>.*?FIND MATCH/i) || 
                                           html.match(/onclick="([^"]+)"[^>]*>.*?START MATCH/i);
                            if (matchStr) {
                                eval(matchStr[1]);
                                success = true;
                            } else {
                                let dummyDiv = document.createElement('div');
                                dummyDiv.style.display = 'none';
                                dummyDiv.innerHTML = html;
                                document.body.appendChild(dummyDiv);
                                
                                const btns = Array.from(dummyDiv.querySelectorAll('*')).filter(e => {
                                    const t = (e.textContent || '').trim().toUpperCase();
                                    return ['FIND MATCH', 'START MATCH'].some(target => t === target || t.includes(target));
                                });
                                if (btns.length > 0) {
                                    btns[0].click();
                                    success = true;
                                }
                                dummyDiv.remove();
                            }
                        } catch(e) {}
                    }
                    
                    if (!success) {
                        // If we couldn't find the ranked window dynamically, try clicking FIND MATCH on DOM directly
                        if (clickDeepest(['FIND MATCH', 'START MATCH'])) {
                            success = true;
                        } else {
                            // If FIND MATCH is not present, we click RANKED on main menu, wait, and click FIND MATCH
                            if (clickMainMenuRanked()) {
                                setTimeout(() => {
                                    if (clickDeepest(['FIND MATCH', 'START MATCH'])) {
                                        notifyStarted();
                                    } else {
                                        notifyError("FIND MATCH button not found after opening menu.");
                                    }
                                }, 1000);
                                return; // wait for setTimeout
                            }
                        }
                    }
                    
                    if (success) {
                        notifyStarted();
                    } else {
                        notifyError("Could not find Ranked matching function or buttons in Krunker.");
                    }
                } catch(e) {
                    notifyError("Exception occurred: " + e.message);
                }
            } 
            else if (msg.action === 'cancelRankedMatch') {
                try {
                    let rId = findRankedWindowId();
                    if (rId !== -1 && window.windows[rId].build) {
                        let html = window.windows[rId].build();
                        let matchStr = html.match(/onclick="([^"]+)"[^>]*>.*?CANCEL/i) || 
                                       html.match(/onclick="([^"]+)"[^>]*>.*?LEAVE/i);
                        if (matchStr) eval(matchStr[1]);
                    }
                    clickDeepest(['CANCEL', 'LEAVE', 'STOP']);
                } catch(e) {}
            }
        }
    } catch(e) {}
});

// マッチ完了を監視してAlt Windowへ通知
setInterval(() => {
    try {
        const isMatched = Array.from(document.querySelectorAll('*')).some(e => {
            if (e.children.length > 0) return false;
            if (e.offsetParent === null) return false;
            const t = (e.textContent || '').trim().toUpperCase();
            return t === 'REJOIN' || t === 'MATCH FOUND!' || t === 'ACCEPT';
        });

        if (isMatched && !window.__kucMatchNotified) {
            window.__kucMatchNotified = true; 
            window.chrome.webview.postMessage(JSON.stringify({
                type: 'forwardIpcToAlt',
                action: 'rankedMatchFound'
            }));
            
            setTimeout(() => { window.__kucMatchNotified = false; }, 30000); 
        }
    } catch(e) {}
}, 1000);
