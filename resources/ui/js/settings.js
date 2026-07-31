/**
 * settings.js — Krunker Ultra Client 設定UI
 */

(function () {
  'use strict';

  // 重複防止
  const existingPanel = document.getElementById('kuc-settings-overlay');
  if (existingPanel) existingPanel.remove();

  // ============================================================
  // DOM生成
  // ============================================================
  const overlay = document.createElement('div');
  overlay.id = 'kuc-settings-overlay';
  overlay.innerHTML = `
    <div id="kuc-settings-panel">

      <!-- ヘッダー -->
      <div id="kuc-settings-header">
        <div id="kuc-settings-title">
          <span class="kuc-logo-mark">⚡</span>
          Krunker Ultra Client 設定
        </div>
        <button id="kuc-settings-close" title="閉じる (Esc)">✕</button>
      </div>

      <!-- タブ -->
      <div id="kuc-settings-tabs">
        <button class="kuc-tab kuc-active" data-tab="instant">即時適用</button>
        <button class="kuc-tab" data-tab="restart">再起動が必要</button>
      </div>

      <!-- コンテンツ -->
      <div id="kuc-settings-content">

        <!-- ===== 即時適用タブ ===== -->
        <div class="kuc-tab-panel kuc-active" data-panel="instant">
          <p class="kuc-section-title">システム・プロセス</p>

          ${makeToggleRow('kuc-pr-high',
            'プロセス優先度を「高」にする',
            'OS全体でKrunkerにCPUリソースを最優先で割り当てます。他のアプリが重くなる場合があります。',
            true)}

          <p class="kuc-section-title" style="margin-top:20px;">拡張機能</p>

          ${makeToggleRow('kuc-ex-adblock',
            '広告・トラッカーをブロック',
            'ゲーム外の広告・トラッカー通信をC++レベルで即時遮断します。Pingに影響する場合があるため、問題がある場合はオフにしてください。',
            false)}
            
          ${makeToggleRow('kuc-ex-swapper',
            'リソーススワッパー',
            'swapperフォルダに配置したテクスチャ・サウンド・モデルでゲーム内リソースを差し替えます。',
            true)}
          ${makeToggleRow('kuc-ex-userscripts',
            'ユーザースクリプト',
            'scriptsフォルダに配置した.jsファイルをゲーム読み込み時に自動実行します。',
            true)}
          <div class="kuc-setting-row" style="margin-top:12px;">
            <button class="kuc-btn" id="kuc-open-swapper" style="margin-right:8px;">swapperフォルダを開く</button>
            <button class="kuc-btn" id="kuc-open-scripts">scriptsフォルダを開く</button>
          </div>
        </div>

        <!-- ===== 再起動が必要タブ ===== -->
        <div class="kuc-tab-panel" data-panel="restart">
          <p class="kuc-section-title">ブラウザエンジン</p>

          ${makeToggleRow('kuc-bw-hwaccel',
            'ハードウェアアクセラレーション (GPU描画)',
            'GPUを使ってゲームを描画します。オンにすることでFPSが大幅に向上します。基本的にオンのままにしてください。',
            true)}
          ${makeToggleRow('kuc-bw-vsync',
            'GPU VSync（垂直同期）を無効化',
            'モニターの同期を外し、描画遅延を最小化します（※FPS自体はモニター上限になります）。',
            true)}
          ${makeToggleRow('kuc-bw-bgthrottle',
            'バックグラウンド時のFPS低下を防止',
            'Alt-Tabなどで別ウィンドウを操作中でもKrunkerのFPSを下げません。配信や別作業をしながらプレイする方向けです。',
            true)}
          ${makeToggleRow('kuc-bw-mousefix',
            'マウスフリック修正 (高ポーリングレート対策)',
            '高ポーリングレート(1000Hz以上)のマウス使用時に、射撃(左クリック)でPingが跳ね上がる・フリーズする問題を修正します。',
            true)}
          ${makeToggleRow('kuc-bw-ignoregpu',
            'GPUブラックリストを無視',
            '古いグラボでChromiumが非対応と判断した場合でも、強制的にGPUアクセラレーションを有効化します。通常は不要です。',
            false)}

          <p class="kuc-section-title" style="margin-top:20px;">ネットワーク最適化</p>

          ${makeToggleRow('kuc-nw-nagle',
            'Nagleアルゴリズムを無効化 (TCP_NODELAY)',
            '細かいデータをまとめる処理を省いて即座に送信します。FPSゲームのPing改善に有効です。',
            true)}

          <p class="kuc-section-title" style="margin-top:20px;">その他</p>
          ${makeToggleRow('kuc-ex-fullscreen',
            '起動時にフルスクリーンにする',
            '次回起動時に自動的に全画面で開始します。',
            false)}

          <div class="kuc-setting-row" style="margin-top:12px;">
            <button class="kuc-btn kuc-btn--primary" id="kuc-open-alt-window" style="width:100%; text-align:center;">
              サブウィンドウを開く (Ranked待機用 / ショートカット: F7)
            </button>
          </div>
        </div>

      </div><!-- /settings-content -->

      <!-- フッター -->
      <div id="kuc-settings-footer">
        <button class="kuc-btn kuc-btn--primary" id="kuc-settings-save">保存して適用</button>
      </div>

    </div><!-- /settings-panel -->

    <!-- 管理者権限通知バナー -->
    <div id="kuc-admin-banner" style="display:none; position:fixed; bottom:20px; left:50%; transform:translateX(-50%);
      background:#ef4444; color:#fff; padding:12px 20px; border-radius:8px; z-index:99999;
      max-width:520px; text-align:center; font-size:13px; box-shadow:0 4px 20px rgba(0,0,0,0.5);">
      <strong>⚠ 管理者権限が必要です</strong><br>
      <span id="kuc-admin-banner-msg"></span>
    </div>
  `;

  document.body.appendChild(overlay);

  // ============================================================
  // ヘルパー関数
  // ============================================================
  function makeToggleRow(id, name, desc, defaultOn) {
    const checked = defaultOn ? 'checked' : '';
    return `
      <div class="kuc-setting-row">
        <div class="kuc-setting-info">
          <span class="kuc-setting-name">${name}</span>
          <span class="kuc-setting-desc">${desc}</span>
        </div>
        <label class="kuc-toggle">
          <input type="checkbox" id="${id}" ${checked}>
          <div class="kuc-toggle-track">
            <div class="kuc-toggle-thumb"></div>
          </div>
        </label>
      </div>
    `;
  }

  // ============================================================
  // タブ切り替え
  // ============================================================
  document.querySelectorAll('.kuc-tab').forEach(function (tab) {
    tab.addEventListener('click', function () {
      const targetPanel = tab.dataset.tab;
      document.querySelectorAll('.kuc-tab').forEach(t => t.classList.remove('kuc-active'));
      document.querySelectorAll('.kuc-tab-panel').forEach(p => p.classList.remove('kuc-active'));
      tab.classList.add('kuc-active');
      const panel = document.querySelector(`.kuc-tab-panel[data-panel="${targetPanel}"]`);
      if (panel) panel.classList.add('kuc-active');
    });
  });

  // ============================================================
  // 開閉ロジック
  // ============================================================
  function openSettings() { overlay.classList.add('kuc-open'); }
  function closeSettings() { overlay.classList.remove('kuc-open'); }

  document.getElementById('kuc-settings-close')?.addEventListener('click', closeSettings);
  overlay.addEventListener('click', function (e) {
    if (e.target === overlay) closeSettings();
  });
  document.addEventListener('keydown', function (e) {
    if (e.key === 'F2') {
      e.preventDefault();
      overlay.classList.contains('kuc-open') ? closeSettings() : openSettings();
    }
    if (e.key === 'Escape' && overlay.classList.contains('kuc-open')) {
      closeSettings();
    }
  });

  // ============================================================
  // 保存ボタン・フォルダを開く
  // ============================================================
  document.getElementById('kuc-open-swapper')?.addEventListener('click', () => {
    sendMessage({ type: 'openFolder', folder: 'swapper' });
  });
  document.getElementById('kuc-open-scripts')?.addEventListener('click', () => {
    sendMessage({ type: 'openFolder', folder: 'scripts' });
  });
  document.getElementById('kuc-open-alt-window')?.addEventListener('click', () => {
    sendMessage({ type: 'openAltWindow' });
  });

  document.getElementById('kuc-settings-save')?.addEventListener('click', function () {
    const settingsData = {
      process: {
        highPriority:    document.getElementById('kuc-pr-high')?.checked ?? true,
        cpuAffinityMask: 0,
      },
      browser: {
        hardwareAccel:              document.getElementById('kuc-bw-hwaccel')?.checked ?? true,
        disableVSync:               document.getElementById('kuc-bw-vsync')?.checked ?? true,
        ignoreGpuBlocklist:         document.getElementById('kuc-bw-ignoregpu')?.checked ?? false,
        disableBackgroundThrottling: document.getElementById('kuc-bw-bgthrottle')?.checked ?? true,
        mouseFlickFix:              document.getElementById('kuc-bw-mousefix')?.checked ?? true,
      },
      network: {
        disableNagle:     document.getElementById('kuc-nw-nagle')?.checked ?? true,
        applyTcpRegistry: document.getElementById('kuc-nw-tcpreg')?.checked ?? false,
      },
      extension: {
        blockAds:        document.getElementById('kuc-ex-adblock')?.checked ?? true,
        startFullscreen: document.getElementById('kuc-ex-fullscreen')?.checked ?? false,
        enableSwapper: document.getElementById('kuc-ex-swapper')?.checked ?? true,
        enableUserscripts: document.getElementById('kuc-ex-userscripts')?.checked ?? true,
      },
    };

    sendMessage({ type: 'saveSettings', settings: settingsData });
    closeSettings();
  });


  // ============================================================
  // 管理者権限バナー表示
  // ============================================================
  function showBanner(msg, color, duration) {
    const banner = document.getElementById('kuc-admin-banner');
    const msgEl  = document.getElementById('kuc-admin-banner-msg');
    if (!banner || !msgEl) return;
    msgEl.textContent = msg;
    banner.style.background = color || '#ef4444';
    banner.style.display = 'block';
    setTimeout(() => { banner.style.display = 'none'; }, duration || 5000);
  }

  // ============================================================
  // C++ → JS メッセージ受信
  // ============================================================
  window.chrome?.webview?.addEventListener('message', function (evt) {
    try {
      const msg = typeof evt.data === 'string' ? JSON.parse(evt.data) : evt.data;

      if (msg.type === 'settingsLoaded' && msg.settings) {
        populateSettings(msg.settings);
      }

      if (msg.type === 'adminRequired' && msg.message) {
        showBanner(msg.message, '#ef4444', 8000);
      }
    } catch (_) {}
  });

  // ============================================================
  // 設定値をUIに反映
  // ============================================================
  function populateSettings(s) {
    const p = s.process   || {};
    const b = s.browser   || {};
    const n = s.network   || {};
    const e = s.extension || {};

    setCheck('kuc-pr-high',        p.highPriority);

    setCheck('kuc-bw-hwaccel',     b.hardwareAccel);
    setCheck('kuc-bw-vsync',       b.disableVSync);
    setCheck('kuc-bw-bgthrottle',  b.disableBackgroundThrottling);
    setCheck('kuc-bw-mousefix',    b.mouseFlickFix);
    setCheck('kuc-bw-ignoregpu',   b.ignoreGpuBlocklist);

    setCheck('kuc-nw-nagle',       n.disableNagle);
    setCheck('kuc-nw-tcpreg',      n.applyTcpRegistry);

    setCheck('kuc-ex-adblock',     e.blockAds);
    setCheck('kuc-ex-fullscreen',  e.startFullscreen);
    if (e.enableSwapper !== undefined) setCheck('kuc-ex-swapper', e.enableSwapper);
    if (e.enableUserscripts !== undefined) setCheck('kuc-ex-userscripts', e.enableUserscripts);
  }

  function setCheck(id, value) {
    const el = document.getElementById(id);
    if (el && value != null) el.checked = !!value;
  }

  function sendMessage(obj) {
    try {
      window.chrome?.webview?.postMessage(JSON.stringify(obj));
    } catch (_) {}
  }

  // 設定値のロードを要求
  sendMessage({ type: 'requestSettings' });

  console.log('[KUC] settings.js loaded. Press F2 to open settings.');
})();
