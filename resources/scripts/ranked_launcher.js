// ============================================================
// Krunker Ultra Client - Ranked Window Launcher
// ============================================================

function launchRanked() {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'openAltWindow'
        }));
    }
}

// 1. Fallback: F7 Keybind
document.addEventListener('keydown', (e) => {
    if (e.key === 'F7') {
        e.preventDefault();
        e.stopPropagation();
        launchRanked();
    }
}, true);

// 2. Inject Button into Krunker's UI
setInterval(() => {
    // 試行1: windowHeader (Krunkerの標準ポップアップ)
    let header = document.querySelector('#windowHeader');
    // 試行2: もしFACEITやRanked用の特定のiframe/divがあればそのコンテナを探す
    let isRankedWindow = false;
    
    if (header && (header.textContent.includes('Ranked') || header.textContent.includes('Matchmaking') || header.textContent.includes('FACEIT'))) {
        isRankedWindow = true;
    }
    
    // KrunkerのUI構造は頻繁に変わるため、"Ranked"というテキストを含むボタンやヘッダーを広く探す
    let rankedMenuObj = document.querySelector('#menuBtnRanked'); // メインメニューのRankedボタン
    
    // ポップアップが開いているかチェック
    let windowHolder = document.getElementById('windowHolder');
    if (windowHolder && windowHolder.style.display !== 'none') {
        if (header && (header.textContent.includes('Ranked') || header.textContent.toLowerCase().includes('comp'))) {
            isRankedWindow = true;
        }
    }

    if (isRankedWindow) {
        if (!document.getElementById('kuc-ranked-btn')) {
            let btn = document.createElement('div');
            btn.id = 'kuc-ranked-btn';
            btn.className = 'button small buttonP'; // Krunker標準の緑/青ボタンクラス
            btn.innerHTML = 'Open KUC Ranked 🚀';
            btn.style.position = 'absolute';
            btn.style.right = '20px';
            btn.style.top = '60px';
            btn.style.zIndex = '999999';
            btn.style.backgroundColor = '#ea580c';
            btn.style.color = 'white';
            btn.style.border = '3px solid #000';
            btn.style.borderRadius = '6px';
            btn.style.padding = '10px 20px';
            btn.style.fontSize = '18px';
            btn.style.cursor = 'pointer';
            btn.onclick = () => {
                launchRanked();
            };
            
            // ボタンのホバー効果
            btn.onmouseenter = () => { btn.style.backgroundColor = '#f97316'; };
            btn.onmouseleave = () => { btn.style.backgroundColor = '#ea580c'; };
            
            if (windowHolder) {
                windowHolder.appendChild(btn);
            } else {
                document.body.appendChild(btn);
            }
        }
    } else {
        let btn = document.getElementById('kuc-ranked-btn');
        if (btn) btn.remove();
    }
}, 500);

// Rankediframeがある場合への対応 (もしKrunkerがiframeでRankedを表示している場合)
setInterval(() => {
    let frames = document.querySelectorAll('iframe');
    let hasRankedIframe = false;
    let targetContainer = document.body;
    
    frames.forEach(f => {
        if (f.src && (f.src.includes('ranked') || f.src.includes('matchmaking'))) {
            hasRankedIframe = true;
            targetContainer = f.parentElement;
        }
    });
    
    if (hasRankedIframe) {
        if (!document.getElementById('kuc-ranked-iframe-btn')) {
            let btn = document.createElement('div');
            btn.id = 'kuc-ranked-iframe-btn';
            btn.innerHTML = 'Open KUC Ranked 🚀';
            btn.style.position = 'absolute';
            btn.style.right = '50px';
            btn.style.top = '50px';
            btn.style.zIndex = '999999';
            btn.style.backgroundColor = '#ea580c';
            btn.style.color = 'white';
            btn.style.border = '3px solid #000';
            btn.style.borderRadius = '6px';
            btn.style.padding = '10px 20px';
            btn.style.fontSize = '18px';
            btn.style.cursor = 'pointer';
            btn.onclick = () => {
                launchRanked();
            };
            targetContainer.appendChild(btn);
        }
    } else {
        let btn = document.getElementById('kuc-ranked-iframe-btn');
        if (btn) btn.remove();
    }
}, 500);
