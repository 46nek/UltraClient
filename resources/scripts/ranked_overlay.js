// ============================================================
// Krunker Ultra Client - Ranked Queue Overlay
// ============================================================

// 1. Krunkerの画面を非表示にし、独自UI用のスタイルを適用
const style = document.createElement('style');
style.textContent = `
    /* Krunkerの要素を透明にしてクリック不可にする (DOMツリーからは消さない) */
    body > *:not(#kuc-ranked-ui) {
        opacity: 0 !important;
        pointer-events: none !important;
        z-index: -999 !important;
    }
    
    /* 独自Ranked UI */
    #kuc-ranked-ui {
        position: fixed;
        top: 0; left: 0; right: 0; bottom: 0;
        background: #121212;
        color: #fff;
        z-index: 999999 !important;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    }
    
    .kuc-title { font-size: 28px; font-weight: bold; margin-bottom: 30px; color: #f97316; letter-spacing: 2px;}
    .kuc-btn {
        background: #ea580c; color: white; border: none; padding: 15px 50px;
        font-size: 20px; font-weight: bold; border-radius: 8px; cursor: pointer;
        transition: 0.2s; box-shadow: 0 4px 15px rgba(234, 88, 12, 0.4);
    }
    .kuc-btn:hover { background: #f97316; transform: translateY(-2px); }
    .kuc-btn:active { transform: translateY(0); }
    
    .kuc-status { margin-top: 20px; font-size: 16px; color: #a1a1aa; font-weight: 500; }
    .kuc-timer { font-size: 54px; font-family: monospace; margin-top: 10px; color: #fff; font-weight: bold; }
`;
document.documentElement.appendChild(style);

// 2. Krunkerの3Dループ (requestAnimationFrame) を間引いてCPU負荷をゼロにする
const oldRaf = window.requestAnimationFrame;
window.requestAnimationFrame = (cb) => {
    return setTimeout(() => oldRaf(cb), 100); // 10 FPS
};

// 3. UI構築とマッチメイキングロジック
window.addEventListener('DOMContentLoaded', () => {
    const ui = document.createElement('div');
    ui.id = 'kuc-ranked-ui';
    ui.innerHTML = `
        <div class="kuc-title">RANKED MATCHMAKER</div>
        
        <select id="kuc-region" style="margin-bottom: 30px; padding: 12px 20px; font-size: 16px; background: #27272a; color: white; border: 1px solid #3f3f46; border-radius: 6px; outline: none; cursor: pointer;">
            <option value="tokyo">Tokyo (TYO)</option>
            <option value="syd">Sydney (SYD)</option>
            <option value="sgp">Singapore (SGP)</option>
            <option value="fra">Frankfurt (FRA)</option>
            <option value="sv">Silicon Valley (SV)</option>
            <option value="mia">Miami (MIA)</option>
        </select>
        
        <button class="kuc-btn" id="kuc-match-btn">FIND MATCH</button>
        <div class="kuc-status" id="kuc-status">Status: Idle</div>
        <div class="kuc-timer" id="kuc-timer">00:00</div>
    `;
    document.body.appendChild(ui);
    
    const btn = document.getElementById('kuc-match-btn');
    const status = document.getElementById('kuc-status');
    const timerEl = document.getElementById('kuc-timer');
    let timer = 0;
    let interval = null;
    let isMatching = false;
    
    btn.addEventListener('click', () => {
        if (!isMatching) {
            // マッチ開始
            isMatching = true;
            btn.textContent = 'CANCEL MATCH';
            btn.style.background = '#e11d48';
            btn.style.boxShadow = '0 4px 15px rgba(225, 29, 72, 0.4)';
            status.textContent = 'Status: Searching for match...';
            
            // 隠れているKrunkerのマッチング処理を起動
            try {
                if (window.showWindow) window.showWindow(50);
                setTimeout(() => {
                    const findBtns = Array.from(document.querySelectorAll('.button, div.button, .settingsBtn')).filter(e => 
                        e.textContent.toUpperCase().includes('FIND MATCH') || 
                        e.textContent.toUpperCase().includes('RANKED')
                    );
                    if (findBtns.length > 0) findBtns[findBtns.length - 1].click();
                }, 500);
            } catch(e) { console.error(e); }
            
            timer = 0;
            interval = setInterval(() => {
                timer++;
                let m = Math.floor(timer / 60).toString().padStart(2, '0');
                let s = (timer % 60).toString().padStart(2, '0');
                timerEl.textContent = `${m}:${s}`;
                
                // REJOINボタンが出現（マッチが見つかった）か監視
                const rejoinBtn = Array.from(document.querySelectorAll('.button, div.button')).find(e => 
                    e.textContent.toUpperCase().includes('REJOIN') ||
                    e.textContent.toUpperCase().includes('MATCH FOUND')
                );
                
                if (rejoinBtn) {
                    clearInterval(interval);
                    status.textContent = 'Status: MATCH FOUND! Go to Main Window!';
                    status.style.color = '#4ade80';
                    btn.textContent = 'MATCHED';
                    btn.style.background = '#22c55e';
                    btn.style.boxShadow = '0 4px 15px rgba(34, 197, 94, 0.4)';
                    
                    // Krunker公式の音を鳴らす
                    const audio = new Audio('https://assets.krunker.io/sound/ui/objective_secure.mp3');
                    audio.volume = 1.0;
                    audio.play().catch(()=>{});
                }
            }, 1000);
            
        } else {
            // マッチキャンセル
            isMatching = false;
            btn.textContent = 'FIND MATCH';
            btn.style.background = '#ea580c';
            btn.style.boxShadow = '0 4px 15px rgba(234, 88, 12, 0.4)';
            status.textContent = 'Status: Idle';
            status.style.color = '#a1a1aa';
            clearInterval(interval);
            timerEl.textContent = '00:00';
            
            try {
                const cancelBtn = Array.from(document.querySelectorAll('.button, div.button')).find(e => 
                    e.textContent.toUpperCase().includes('CANCEL') || 
                    e.textContent.toUpperCase().includes('LEAVE')
                );
                if (cancelBtn) cancelBtn.click();
            } catch(e) {}
        }
    });
});
