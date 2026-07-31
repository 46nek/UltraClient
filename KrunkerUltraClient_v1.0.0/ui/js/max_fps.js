// ==========================================
// Max FPS Mode / Streamer Mode
// F4キーを押すことでUI（#uiBase等）の表示・非表示を切り替えます
// ==========================================
(function() {
    let uiHidden = false;
    let customStyle = document.createElement('style');
    customStyle.id = 'max-fps-mode-style';
    customStyle.textContent = `
        #uiBase, #chatUI, #killFeed {
            display: none !important;
        }
    `;
    
    window.addEventListener('keydown', function(e) {
        // F4キー
        if (e.key === 'F4' || e.keyCode === 115) {
            uiHidden = !uiHidden;
            if (uiHidden) {
                document.head.appendChild(customStyle);
                // 画面中央に小さく通知を出す（UIBaseの外に直接追加）
                let notif = document.createElement('div');
                notif.textContent = "⚡ Max FPS Mode: ON";
                notif.style.position = 'fixed';
                notif.style.top = '10px';
                notif.style.left = '50%';
                notif.style.transform = 'translateX(-50%)';
                notif.style.color = '#00ff00';
                notif.style.fontFamily = 'sans-serif';
                notif.style.fontSize = '16px';
                notif.style.fontWeight = 'bold';
                notif.style.zIndex = '999999';
                notif.style.textShadow = '1px 1px 2px #000';
                notif.id = 'max-fps-notif';
                document.body.appendChild(notif);
                
                setTimeout(() => {
                    if (notif.parentNode) notif.parentNode.removeChild(notif);
                }, 2000);
            } else {
                if (customStyle.parentNode) {
                    customStyle.parentNode.removeChild(customStyle);
                }
                // 通知
                let notif = document.createElement('div');
                notif.textContent = "⚡ Max FPS Mode: OFF";
                notif.style.position = 'fixed';
                notif.style.top = '10px';
                notif.style.left = '50%';
                notif.style.transform = 'translateX(-50%)';
                notif.style.color = '#ff0000';
                notif.style.fontFamily = 'sans-serif';
                notif.style.fontSize = '16px';
                notif.style.fontWeight = 'bold';
                notif.style.zIndex = '999999';
                notif.style.textShadow = '1px 1px 2px #000';
                document.body.appendChild(notif);
                
                setTimeout(() => {
                    if (notif.parentNode) notif.parentNode.removeChild(notif);
                }, 2000);
            }
        }
    });
    
    console.log("[Max FPS Mode] Initialized. Press F4 to toggle.");
})();
