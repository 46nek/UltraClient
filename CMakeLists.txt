cmake_minimum_required(VERSION 3.20)
project(KrunkerUltraClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# コンパイラフラグ (MSVC)
# ============================================================
if(MSVC)
    # リリースビルド最適化: /O2=最大速度 /GL=リンク時コード生成 /Gy=関数単位COMDAT /Oi=組み込み関数 /MT=静的CRT
    add_compile_options(
        /W4
        /utf-8          # ソースファイルをUTF-8として扱う (日本語コメント対応)
        /wd4819         # コードページ警告を無効化
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/MTd>
    )
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>     # リンク時コード生成
        $<$<CONFIG:Release>:/OPT:REF>  # 未使用関数を除去
        $<$<CONFIG:Release>:/OPT:ICF>  # 同一COMDAT折り畳み
    )
    # Unicode / Windows 10以降
    add_compile_definitions(
        UNICODE
        _UNICODE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )
endif()

# ============================================================
# ソースファイル
# ============================================================
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/window/WindowManager.cpp
    src/webview/WebViewHost.cpp
    src/webview/ResourceFilter.cpp
    src/process/ProcessManager.cpp
    src/network/NetworkOptimizer.cpp
    src/settings/Settings.cpp
    src/util/Logger.cpp
    src/dxgi_hook/latency_optimizer.cpp
)

# Windowsリソース (アイコン)
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/resources/icons/app.rc")
    list(APPEND SOURCES resources/icons/app.rc)
endif()

# ============================================================
# 実行ファイル (WIN32 = WinMain使用・コンソール非表示)
# ============================================================
add_executable(KrunkerUltraClient WIN32 ${SOURCES})

# ============================================================
# インクルードパス
# ============================================================
target_include_directories(KrunkerUltraClient PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/rapidjson/include
    ${CMAKE_SOURCE_DIR}/third_party/uriparser/include
    ${CMAKE_SOURCE_DIR}/third_party/WebView2/include
)

# ============================================================
# Windowsシステムライブラリ
# ============================================================
target_link_libraries(KrunkerUltraClient PRIVATE
    ws2_32      # Windows Sockets API
    ntdll
    ole32       # COM
    oleaut32
    shlwapi
    version
    dwmapi      # DWM (ウィンドウ描画)
    uxtheme
)

# ============================================================
# WebView2
# ============================================================
# オプション1: NuGetパッケージ (Visual Studio推奨)
#   パッケージ名: Microsoft.Web.WebView2
#   配置先:      packages/Microsoft.Web.WebView2.x.x.x/
# オプション2: 手動配置
#   third_party/WebView2/include/ に WebView2.h, WebView2Experimental.h
#   third_party/WebView2/lib/x64/ に WebView2LoaderStatic.lib or WebView2Loader.dll.lib

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/WebView2/lib/x64/WebView2LoaderStatic.lib")
    target_link_libraries(KrunkerUltraClient PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/WebView2/lib/x64/WebView2LoaderStatic.lib
    )
    target_compile_definitions(KrunkerUltraClient PRIVATE USE_WEBVIEW2_STATIC)
    message(STATUS "WebView2: Using static loader")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/WebView2/lib/x64/WebView2Loader.dll.lib")
    target_link_libraries(KrunkerUltraClient PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/WebView2/lib/x64/WebView2Loader.dll.lib
    )
    message(STATUS "WebView2: Using dynamic loader (WebView2Loader.dll)")
else()
    message(WARNING "WebView2 not found. Download NuGet package Microsoft.Web.WebView2 and place under third_party/WebView2/")
endif()

# ============================================================
# uriparser (オプション)
# ============================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/uriparser/lib/uriparser.lib")
    target_link_libraries(KrunkerUltraClient PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/uriparser/lib/uriparser.lib
    )
    target_compile_definitions(KrunkerUltraClient PRIVATE
        URI_STATIC_BUILD
        URI_NO_UNICODE
    )
endif()


# ============================================================
# ビルド後: UIリソースをコピー
# ============================================================
add_custom_command(TARGET KrunkerUltraClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources/ui
        $<TARGET_FILE_DIR:KrunkerUltraClient>/ui
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources/scripts
        $<TARGET_FILE_DIR:KrunkerUltraClient>/scripts
    COMMENT "Copying UI resources, scripts, and DLLs to output directory..."
)

# ============================================================
# インストール
# ============================================================
install(TARGETS KrunkerUltraClient RUNTIME DESTINATION .)
install(DIRECTORY resources/ui DESTINATION .)
