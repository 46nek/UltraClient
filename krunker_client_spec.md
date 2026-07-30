# Krunker 超軽量クライアント仕様書 v1.1

> **目的：** 低遅延・高フレームレートを極限まで追求した、世界最速を目指すKrunkerクライアントの設計仕様。  
> 技術選定を **C++ / WebView2** に確定。競技向けプリセットは「最軽量」基準で定義する。

---

## 目次

1. [技術選定](#1-技術選定)
2. [目標パフォーマンス指標](#2-目標パフォーマンス指標)
3. [アーキテクチャ](#3-アーキテクチャ)
4. [技術スタック](#4-技術スタック)
5. [FPS最大化施策](#5-fps最大化施策)
6. [Ping最小化施策](#6-ping最小化施策)
7. [パフォーマンス設定プリセット](#7-パフォーマンス設定プリセット)
8. [開発ロードマップ](#8-開発ロードマップ)
9. [競合クライアント比較](#9-競合クライアント比較)

---

## 1. 技術選定

### 採用言語：C++20（Rustではなく）

上限性能一点に絞った場合、**C++ が Rust より有利**である。理由は以下の3点。

| 根拠 | 詳細 |
|------|------|
| **WebView2 SDK との親和性** | WebView2 SDK はネイティブ C++ COM API として設計されている。Rustから呼ぶには `unsafe` + FFI が必須で、呼び出しごとに境界チェックと変換コストが乗る |
| **コンパイラ最適化の実績** | MSVC の `/O2 /GL /LTCG`（リンク時コード生成）は Windows ネイティブワークロードに最適化された枯れた実績がある。Krunkerクライアントのようなイベント駆動ループとの相性が特に良い |
| **マイクロ秒レベルの差** | Raw Input 処理・WSA ソケット操作でRustの所有権チェック機構が僅差（1〜3%）で効くケースが存在する。「世界一」を目指すならこの差を捨てない |

> Rustの安全性・開発体験の優位は認めるが、**上限パフォーマンス一点に絞るならC++が最適解。**

---

## 2. 目標パフォーマンス指標

| 指標 | 目標値 | 備考 |
|------|--------|------|
| 最大FPS | **360+** | ハイエンド環境での上限 |
| 競技プリセット最低FPS | **240+** | 最軽量設定での保証ライン |
| Ping削減率 | **−20%** | Electronベースクライアント比 |
| 起動時間 | **< 2秒** | ゲーム画面表示まで |
| バイナリサイズ | **< 10MB** | 単一実行ファイル |
| メモリ使用量（起動時） | **< 100MB** | Electron比で1/3〜1/5 |

---

## 3. アーキテクチャ

### ElectronではなくC++ / WebView2を採用する理由

| 項目 | Electronベース（従来） | C++ / WebView2（本仕様） |
|------|----------------------|------------------------|
| ランタイム | Node.js + Chromium（二重オーバーヘッド） | 純粋なC++のみ |
| メモリ使用量 | 200〜500MB常時消費 | < 100MB |
| GCフリーズ | 不定期に発生 | なし（決定論的メモリ管理） |
| ネットワーク | ChromiumのTCPスタックに依存 | WSAによる独自最適化 |
| プロセス間通信 | IPCレイテンシが高い | ホスト↔WebView直接通信 |
| バイナリサイズ | 100MB超になりやすい | 数MBの単一バイナリ |
| WebView2ランタイム | 別途インストール必要 | Windows標準Edgeを再利用 |

### プロセス構成

```
Client.exe（C++ メインプロセス）
├── ウィンドウ管理（Win32 API）
├── Raw Input 処理（マウス/キーボード）
├── ネットワーク最適化（WSA）
├── プロセス優先度制御
└── WebView2 ホスト
    └── krunker.io（ゲーム本体）
        ├── アセットキャッシュ（ローカル）
        └── 広告/トラッカーブロック
```

---

## 4. 技術スタック

### コアレイヤー

| コンポーネント | 用途 |
|--------------|------|
| **C++20** | メインプロセス・ウィンドウ管理・ネットワーク最適化 |
| **WebView2 (Microsoft Edge Chromium)** | ゲームレンダリング層。Windows標準のEdgeを再利用するため追加インストール不要 |
| **Windows Sockets API (WSA)** | 低レベルTCP/UDP制御によるping削減 |
| **Win32 Raw Input API** | マウス・キーボードの低レイテンシ入力取得 |
| **MSVC /O2 + /GL + /LTCG** | リンク時コード生成による最高速最適化ビルド |

### ビルド構成

```cmake
# CMakeLists.txt（主要フラグ）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /LTCG /MT")
target_link_libraries(Client PRIVATE
    WebView2LoaderStatic
    ws2_32        # Windows Sockets
    ntdll
    rapidjson
    uriparser
)
```

### 補助ライブラリ

| ライブラリ | 用途 |
|----------|------|
| **RapidJSON** | 設定ファイル・通信データの高速JSON処理 |
| **uriparser** | URL検証・ホワイトリスト制御 |
| **discord-rpc** | Discord Rich Presenceサポート（任意） |
| **semver** | 自動アップデートのバージョン比較 |

---

## 5. FPS最大化施策

### 5-1. レンダリングパイプライン最適化

- WebView2の**ハードウェアアクセラレーションを強制有効化**
- DirectX 11/12レンダリングをデフォルト使用。ソフトウェアフォールバックを排除
- Krunkerゲーム内設定APIを通じてグラフィック最適化プリセットを自動適用
- FPSキャップを競技向けでは `0`（無制限）にデフォルト設定

```cpp
// WebView2 ハードウェアアクセラレーション強制設定例
COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO;
webviewEnvironmentOptions->put_AdditionalBrowserArguments(
    L"--enable-gpu-rasterization "
    L"--enable-zero-copy "
    L"--disable-gpu-vsync "
    L"--max-gum-fps=0"
);
```

### 5-2. メモリ管理

- C++側で**メモリプール**を実装。ゲームセッション中の動的アロケーションを最小化
- GCポーズをゼロにする（`new`/`delete` を避けた静的確保中心の設計）
- WebView2プロセスのメモリ上限を設定し、OSのメモリ圧縮干渉を防止
- 不要なChrome拡張・プラグインを一切ロードしない

### 5-3. プロセス・スレッド優先度制御

```cpp
// プロセス優先度設定
SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
// 競技プリセット時
SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

// ゲームスレッド優先度
SetThreadPriority(gameThread, THREAD_PRIORITY_HIGHEST);
// 競技プリセット時
SetThreadPriority(gameThread, THREAD_PRIORITY_TIME_CRITICAL);
```

- CPUコアへのスレッドアフィニティ設定オプションを提供（ゲームスレッドを高性能コアに固定）
- 競技プリセット時はバックグラウンドアプリ停止を推奨通知

### 5-4. 入力レイテンシ削減

```cpp
// Raw Input 登録例
RAWINPUTDEVICE rid[2];
// マウス
rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
rid[0].usUsage     = HID_USAGE_GENERIC_MOUSE;
rid[0].dwFlags     = RIDEV_INPUTSINK;
rid[0].hwndTarget  = hwnd;
// キーボード
rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
rid[1].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
rid[1].dwFlags     = RIDEV_INPUTSINK;
rid[1].hwndTarget  = hwnd;
RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
```

- OSのマウスアクセラレーション・PointerPrecisionをバイパスし、生のデルタ値を直接取得
- キーボードもRaw Inputで処理し、ゲームウィンドウ非フォーカス時のキー取りこぼしを防ぐ

---

## 6. Ping最小化施策

### 6-1. TCP/IPスタック最適化

レジストリ設定（起動時に自動適用）：

```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\{NIC-GUID}
  TcpAckFrequency = 1    # ACK遅延の無効化（即時ACK送信）
  TCPNoDelay      = 1    # Nagleアルゴリズムの無効化

HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters
  GlobalMaxTcpWindowSize = 65535
  TcpWindowSize          = 65535
```

- ゲームトラフィックにQoSポリシーを設定し、他アプリよりKrunkerの通信を優先

### 6-2. サーバー選択の自動化

```
起動時フロー：
1. 全リージョンのKrunkerサーバーリストを取得
2. 各サーバーへのICMP/TCPping計測を並列実行（std::async）
3. 最低レイテンシのサーバーを自動選択・設定
4. セッション中もping監視を継続（劣化検知時にユーザー通知）
```

### 6-3. トラフィック軽量化

- 広告・トラッカー・不要なKrunker UIアセットをブロックする**ホワイトリスト型フィルタ**を内蔵
- ゲームプレイに不要なAPIリクエストをC++レベルでインターセプト・破棄（WebView2の `NavigationStarting` / `WebResourceRequested` イベントを利用）
- ゲームアセット（テクスチャ・モデル・音声）をローカルにキャッシュし、再ダウンロードを防止

### 6-4. DNS最適化

- Cloudflare DNS（`1.1.1.1`）またはGoogle DNS（`8.8.8.8`）へのDNS-over-HTTPSを自動設定オプション
- HTTPS接続にはHTTP/2を優先し、多重化によるレイテンシを削減

---

## 7. パフォーマンス設定プリセット

> **競技向けは「見た目より1フレームでも多く」が最優先。** 解像度・グラフィックは最低まで下げる。

| 設定項目 | 低スペック（安定30〜60fps） | 標準（60〜144fps） | ⚡ 競技向け（FPS最大化・最軽量） |
|---------|--------------------------|-------------------|-------------------------------|
| 解像度スケール | `0.3` | `0.5〜0.6` | **`0.1〜0.2`（最低）** |
| FPSキャップ | `60` | `144` | **`0`（無制限）** |
| シャドウ | オフ | オフ | **オフ** |
| ポストプロセス全般 | すべてオフ | すべてオフ | **すべてオフ** |
| テクスチャ品質 | 最低 | 低 | **最低** |
| 草・木描画 | オフ | 低 | **オフ** |
| スキン表示 | 有効 | 有効 | **オフ** |
| 射撃中フレームスキップ | 有効 | 有効 | **オフ（スキップしない）** |
| CPUプロセス優先度 | `HIGH` | `HIGH` | **`REALTIME`** |
| スレッド優先度 | `HIGHEST` | `HIGHEST` | **`TIME_CRITICAL`** |
| Nagleアルゴリズム | 無効 | 無効 | **無効** |
| バックグラウンドアプリ停止 | なし | なし | **自動で停止推奨通知** |

### 競技向け設定の根拠

- **解像度0.1〜0.2：** Krunkerはブロック調グラフィックのため極低解像度でも敵の視認性に影響しない。FPS換算のコスパが最も高い設定
- **スキンオフ：** スキンのテクスチア読み込みコストを完全排除
- **草・木オフ：** オブジェクト描画数の削減で描画コールを最小化
- **射撃中フレームスキップオフ：** デフォルトではオン（FPS節約）だが、競技では描画の連続性を優先するためオフ
- **REALTIME優先度：** OSスケジューラから最高優先権を確保。他プロセスへの影響が大きいためユーザーへ事前説明を表示

---

## 8. 開発ロードマップ

### Phase 1 — コアシェル構築（Week 1〜3）

- [ ] C++ + WebView2 でウィンドウ生成・krunker.io ロードの確認
- [ ] Raw Input API によるマウス/キーボード取得
- [ ] プロセス・スレッド優先度設定の実装
- [ ] 単一バイナリビルドの確立（CMake + MSVC）

### Phase 2 — ネットワーク最適化（Week 4〜6）

- [ ] WSA による TCP/IP レジストリ設定の自動適用
- [ ] ping 自動計測と最適サーバー選択
- [ ] 広告/トラッカーブロックフィルタ（WebResourceRequested）
- [ ] ゲームアセットのローカルキャッシュ機能

### Phase 3 — GPU/レンダリング最適化（Week 7〜9）

- [ ] DirectX 強制指定・ハードウェアアクセラレーション設定の自動化
- [ ] Krunker ゲーム内設定 API を通じたグラフィック最適化プリセット自動適用
- [ ] メモリプールの実装・チューニング

### Phase 4 — UI・ユーザービリティ（Week 10〜12）

- [ ] 設定画面（プリセット選択 / 詳細設定）
- [ ] FPS / ping のリアルタイムオーバーレイ表示
- [ ] ユーザースクリプトサポート
- [ ] 自動アップデート機能（semver 比較）
- [ ] Discord Rich Presence 連携（任意）

### Phase 5 — ベンチマーク・チューニング（Week 13〜15）

- [ ] 各競合クライアントとの定量比較ベンチマーク実施
  - IDKR（Electron）
  - Crankshaft（Electron / TypeScript）
  - Chief Client++（C++ / WebView2）
- [ ] 実測値に基づく最終チューニング
- [ ] 公開ビルドの準備

---

## 9. 競合クライアント比較

| クライアント | ベース技術 | 強み | 弱み | 本仕様との差別化 |
|------------|----------|------|------|----------------|
| **IDKR** | Electron / Node.js | 安定性・軽量UI | Electronオーバーヘッド、GCフリーズ | Node.js廃止でフリーズ根絶 |
| **Crankshaft** | Electron / TypeScript | 機能豊富・Mac対応 | 古いElectronバージョン依存 | 最新WebView2で制約なし |
| **Chief Client++** | C++ / WebView2 | 軽量・単一バイナリ | 機能少なめ・開発停滞 | 本仕様が後継として機能追加 |
| **本仕様（目標）** | C++ / WebView2 | **最速・最軽量・フル機能** | Windows限定（初期） | 全要素で業界最高水準を目指す |

---

## 付録：参考リソース

- [WebView2 API Reference (C++)](https://docs.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/)
- [Chief Client++ ソースコード](https://github.com/6ct/clientpp)（アーキテクチャ参考）
- [IDKR ソースコード](https://github.com/idkr-client/idkr)（機能参考）
- [Raw Input API (Win32)](https://docs.microsoft.com/en-us/windows/win32/inputdev/raw-input)
- [WSA / TCP/IP チューニング](https://docs.microsoft.com/en-us/troubleshoot/windows-server/networking/tcp-ip-performance)

---

*Krunker Ultra Client Spec v1.1 — 2026*  
*対象プラットフォーム：Windows 10/11 (x64) — Linux/Mac対応はPhase 6以降*
