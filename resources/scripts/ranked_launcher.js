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

function findFindMatchButton() {
    let elements = document.querySelectorAll('div, button, span');
    for (let i = 0; i < elements.length; i++) {
        let el = elements[i];
        let text = el.textContent.trim().toUpperCase();
        if (text === 'FIND MATCH' || text === 'PLAY RANKED') {
            // Check if it's the actual button (prevents matching huge container divs that happen to only contain the button text)
            // Krunker buttons usually have classes like 'button', 'btn', or cursor: pointer
            let style = window.getComputedStyle(el);
            if (el.tagName === 'BUTTON' || el.className.toLowerCase().includes('button') || el.className.toLowerCase().includes('btn') || style.cursor === 'pointer') {
                return el;
            }
        }
    }
    return null;
}

setInterval(() => {
    let findMatchBtn = findFindMatchButton();
    
    if (findMatchBtn) {
        if (!document.getElementById('kuc-ranked-btn')) {
            let kucBtn = document.createElement('div');
            kucBtn.id = 'kuc-ranked-btn';
            
            // Try to copy Krunker's button classes to blend in
            kucBtn.className = findMatchBtn.className; 
            
            kucBtn.innerHTML = 'KUC Fast Queue 🚀';
            
            // Apply inline styles to make it stand out and look good
            kucBtn.style.backgroundColor = '#ea580c';
            kucBtn.style.color = 'white';
            kucBtn.style.marginLeft = '15px';
            kucBtn.style.cursor = 'pointer';
            kucBtn.style.display = 'inline-flex';
            kucBtn.style.alignItems = 'center';
            kucBtn.style.justifyContent = 'center';
            kucBtn.style.padding = window.getComputedStyle(findMatchBtn).padding;
            if (!kucBtn.style.padding || kucBtn.style.padding === '0px') {
                kucBtn.style.padding = '10px 20px';
            }
            kucBtn.style.borderRadius = window.getComputedStyle(findMatchBtn).borderRadius || '6px';
            kucBtn.style.border = window.getComputedStyle(findMatchBtn).border;
            kucBtn.style.fontWeight = 'bold';
            kucBtn.style.fontSize = window.getComputedStyle(findMatchBtn).fontSize || '18px';
            kucBtn.style.boxSizing = 'border-box';
            
            kucBtn.onclick = (e) => {
                e.preventDefault();
                e.stopPropagation();
                launchRanked();
            };
            
            // Hover effect
            kucBtn.onmouseenter = () => { kucBtn.style.backgroundColor = '#f97316'; };
            kucBtn.onmouseleave = () => { kucBtn.style.backgroundColor = '#ea580c'; };
            
            // Insert it right after the FIND MATCH button
            if (findMatchBtn.parentNode) {
                findMatchBtn.parentNode.insertBefore(kucBtn, findMatchBtn.nextSibling);
                // Ensure parent can fit both side-by-side
                if (window.getComputedStyle(findMatchBtn.parentNode).display !== 'flex') {
                    findMatchBtn.parentNode.style.display = 'flex';
                    findMatchBtn.parentNode.style.justifyContent = 'center';
                    findMatchBtn.parentNode.style.alignItems = 'center';
                }
            }
        }
    } else {
        // If FIND MATCH button disappears (e.g. window closed), remove ours
        let kucBtn = document.getElementById('kuc-ranked-btn');
        if (kucBtn) kucBtn.remove();
    }
}, 500);
