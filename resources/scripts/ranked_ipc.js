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

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'startRankedMatch') {
                if (clickDeepest(['FIND MATCH', 'START MATCH'])) {
                    notifyStarted();
                } else {
                    // もしDOMに存在しないなら一時的にwindow[50]を呼び出して作成だけさせる
                    if (window.windows && window.windows[50] && window.windows[50].build) {
                        try {
                            // DOMを一時的に生成
                            let dummyDiv = document.createElement('div');
                            dummyDiv.style.display = 'none';
                            dummyDiv.innerHTML = window.windows[50].build();
                            document.body.appendChild(dummyDiv);
                            
                            // ボタンを探して押す
                            const btns = Array.from(dummyDiv.querySelectorAll('*')).filter(e => {
                                const t = e.textContent.trim().toUpperCase();
                                return ['FIND MATCH', 'START MATCH'].some(target => t === target || t.includes(target));
                            });
                            if (btns.length > 0) {
                                // クリックイベントがKrunker内部関数を叩くか確認
                                // build() が返すのはHTML文字列なので、onclickがインライン(onclick="...")で書かれている場合のみ発火できる
                                // Krunkerはonclick="windows[50].joinRanked()" のような構造をとっている可能性がある
                                btns[0].click();
                            } else {
                                // ダミーDIV内に直接onclickがあるか？
                                const matchStr = dummyDiv.innerHTML.match(/onclick="([^"]*joinRanked[^"]*)"/i);
                                if (matchStr) {
                                    eval(matchStr[1]); // e.g. windows[50].joinRanked()
                                }
                            }
                            // 処理が終わったらダミーは消す
                            dummyDiv.remove();
                            notifyStarted();
                        } catch(e) {
                            // fallback
                            notifyStarted();
                        }
                    } else {
                        // 最後の手段
                        if (window.showWindow) {
                            let wasOpen = document.getElementById('windowHolder') && document.getElementById('windowHolder').style.display !== 'none';
                            window.showWindow(50);
                            setTimeout(() => {
                                clickDeepest(['FIND MATCH', 'START MATCH']);
                                if (!wasOpen && window.clearWindow) window.clearWindow(); // 閉じる
                                notifyStarted();
                            }, 500);
                        }
                    }
                }
            } 
            else if (msg.action === 'cancelRankedMatch') {
                if (!clickDeepest(['CANCEL', 'LEAVE', 'STOP'])) {
                    // もしキャンセルボタンが見つからないなら裏でAPI呼び出しを試す
                    if (window.windows && window.windows[50] && window.windows[50].build) {
                        let html = window.windows[50].build();
                        const matchStr = html.match(/onclick="([^"]*cancelRanked[^"]*)"/i) || html.match(/onclick="([^"]*leaveRanked[^"]*)"/i);
                        if (matchStr) {
                            eval(matchStr[1]);
                        }
                    }
                }
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
