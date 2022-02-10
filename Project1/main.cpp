#include <windows.h>
#include <vector>

// DirectX11のコードセット
#include <d3d11.h>

// ライブラリ
#pragma comment(lib, "d3d11.lib")

#define WINDOW_CLASS    L"DX11の導入"
#define WINDOW_TITLE    WINDOW_CLASS
#define WINDOW_WIDTH    750
#define WINDOW_HEIGHT   500

// プロトタイプ宣言
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);    // ウィンドウプロシージャ
BOOL OnInit(HWND hWnd);     // 初期化
void OnRender();            // 描画


// パイプラインオブジェクト
ID3D11Device* g_device;
ID3D11DeviceContext* g_context;
IDXGISwapChain*      g_swapChain;

// メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // ウィンドウクラスを初期化
    WNDCLASSEX	windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS;
    RegisterClassEx(&windowClass);

    // ウィンドウのサイズを求める
    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // ウィンドウハンドルを作成
    HWND hWnd = CreateWindow(
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    // DirectXの初期化
    if (!OnInit(hWnd))
    {
        MessageBox(hWnd, L"DirectX11に対応していないデバイスです。", WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // ウィンドウの表示
    ShowWindow(hWnd, SW_SHOW);

    // メインループ
    MSG	msg = {};
    while (msg.message != WM_QUIT)
    {
        // キュー内のメッセージを処理
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // WM_QUITメッセージの部分をWindowsに返す
    return static_cast<char>(msg.wParam);
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
    case WM_PAINT:
        // 描画
        OnRender();
        return 0;
    case WM_DESTROY:
        // 終了
        PostQuitMessage(0);
        return 0;
    }

    // switch文が処理しなかったメッセージを処理
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

// 初期化
BOOL OnInit(HWND hWnd)
{
    // ドライバー種別を定義
    std::vector<D3D_DRIVER_TYPE> driverTypes
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
        D3D_DRIVER_TYPE_SOFTWARE,
    };

    // スワップチェインの作成
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = static_cast<FLOAT>(WINDOW_WIDTH);
    swapChainDesc.BufferDesc.Height = static_cast<FLOAT>(WINDOW_HEIGHT);
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;

    // ドライバー種別を上から検証し選択
    // 選択したデバイスを用いて描画する
    HRESULT result;
    for (size_t i = 0; i < driverTypes.size(); i++)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            driverTypes[i],
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &g_swapChain,
            &g_device,
            nullptr,
            &g_context
        );
        if (SUCCEEDED(result)) break;
    }
    if (FAILED(result)) return FALSE;

    // 表示領域を作成
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<FLOAT>(WINDOW_WIDTH);
    viewport.Height = static_cast<FLOAT>(WINDOW_HEIGHT);
    viewport.MinDepth = D3D11_MIN_DEPTH;    // 0.0f
    viewport.MaxDepth = D3D11_MAX_DEPTH;    // 1.0f
    g_context->RSSetViewports(1, &viewport);

    return TRUE;
}

// 描画
void OnRender()
{
    // フレームを最終出力
    g_swapChain->Present(0, 0);     // フリップ
}