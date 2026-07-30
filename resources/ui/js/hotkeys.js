/**
 * hotkeys.js — Krunker カスタムホットキー
 */

(function () {
  'use strict';

  // F6キーで新しいマッチを検索（同じリージョン内の最適なマッチへ移動）
  window.addEventListener("keydown", function (e) {
    if (e.key === "F6") {
      e.preventDefault();
      console.log("[KUC] F6 pressed: Finding a new match...");
      // Krunkerの公式マッチメーカー機能を利用して、
      // 選択中のリージョンで最も人数が多い(空きがある)最適なマッチに自動的に移動します。
      location.href = "/";
    }
  });

  console.log('[KUC] hotkeys.js loaded. Press F6 to find a new match.');
})();
