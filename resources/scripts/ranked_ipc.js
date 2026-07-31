// ============================================================
// Krunker Ultra Client - Main Window IPC Listener
// ============================================================

function notifyError(msg) {
    if (window.chrome && window.chrome.webview) {
        window.chrome.webview.postMessage(JSON.stringify({
            type: 'forwardIpcToAlt',
            action: 'rankedMatchError',
            message: msg
        }));
    }
}

window.chrome.webview.addEventListener('message', (event) => {
    try {
        let msg = event.data;
        if (typeof msg === 'string') msg = JSON.parse(msg);
        
        if (msg.type === 'forwardIpcToMain') {
            if (msg.action === 'getRankedToken') {
                try {
                    let token = localStorage.getItem("__FRVR_auth_access_token");
                    if (!token) {
                        notifyError("Token not found. Please login to Krunker first.");
                        return;
                    }
                    
                    token = token.replace(/"/g, "");
                    token = token.replace(/\//g, ""); // Remove all slashes just in case
                    
                    if (window.chrome && window.chrome.webview) {
                        window.chrome.webview.postMessage(JSON.stringify({
                            type: 'forwardIpcToAlt',
                            action: 'rankedTokenResponse',
                            token: token
                        }));
                    }
                } catch(e) {
                    notifyError("Failed to access localStorage: " + e.message);
                }
            } else if (msg.action === 'matchFound') {
                // The ranked matchmaking server found a match.
                let payload = msg.payload;
                if (payload && payload.gameId) {
                    location.href = "https://krunker.io/?game=" + payload.gameId;
                } else if (payload && payload.url) {
                    location.href = payload.url;
                } else {
                    // Fallback to reloading, Krunker will show the Rejoin button on load
                    location.reload();
                }
            }
        }
    } catch(e) {}
});
