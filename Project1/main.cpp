#include <windows.h>
#include <vector>
#include <concepts>

// DirectX11のコードセット
#include <d3d11.h>
#include<d3dcompiler.h>
#include <directxmath.h>

// ライブラリ
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

#define WINDOW_CLASS    L"DX11の導入"
#define WINDOW_TITLE    WINDOW_CLASS
#define WINDOW_WIDTH    750
#define WINDOW_HEIGHT   500

#define SAFE_RELEASE(x) if(x) x->Release();

// プロトタイプ宣言
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);    // ウィンドウプロシージャ
HRESULT OnInit(HWND hWnd);          // 初期化
HRESULT InitDevice(HWND hWnd);      // デバイス関連初期化
VOID InitView();                 // ビュー関連初期化
HRESULT InitShader();               // シェーダ関連初期化
HRESULT InitBuffer();               // バッファ関連初期化
VOID InitMatrix();                  // マトリックス関連初期化
VOID OnUpdate();                    // 更新
VOID OnRender();                    // 描画
VOID OnDestroy();                   // メモリ解放
float redNum = 0;

// 表示領域の寸法(アスペクト比)
FLOAT g_aspectRatio = static_cast<FLOAT>(WINDOW_WIDTH) / static_cast<FLOAT>(WINDOW_HEIGHT);

// パイプラインオブジェクト
ID3D11Device* g_device;
ID3D11DeviceContext* g_context;
IDXGISwapChain*      g_swapChain;
ID3D11RenderTargetView* g_renderTargetView;
ID3D11InputLayout* g_layout;       
ID3D11VertexShader* g_vertexShader;
ID3D11PixelShader* g_pixelShader;  
ID3D11Buffer* g_vertexBuffer;      
ID3D11Buffer* g_indexBuffer;       
ID3D11Buffer* g_constantBuffer;

// 頂点構造体
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

// 定数構造体
struct ConstantBuffer
{
    DirectX::XMMATRIX m_WVP;
};


// Y軸回転変数
static FLOAT y = 0;

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

    // ウィンドウの表示
    ShowWindow(hWnd, SW_SHOW);

    // メインループ
    MSG	msg = {};
    ZeroMemory(&msg, sizeof(msg));
    if (SUCCEEDED(OnInit(hWnd)))   // DirectXの初期化
    {
        // ウィンドウの表示
        ShowWindow(hWnd, SW_SHOW);

        // メインループ
        while (msg.message != WM_QUIT)
        {
            // キュー内のメッセージを処理
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                OnUpdate();     // 更新
                OnRender();     // 描画
            }
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
    case WM_DESTROY:    // 終了時
        OnDestroy();    // メモリ解放
        PostQuitMessage(0);
        return 0;
    }

    // switch文が処理しなかったメッセージを処理
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

// 初期化
HRESULT OnInit(HWND hWnd)
{
    // デバイス関連初期化
    if (FAILED(InitDevice(hWnd))) return E_FAIL;

    // ビュー関連初期化
    InitView();

    // シェーダ関連初期化
    if (FAILED(InitShader())) return E_FAIL;

    // バッファ関連初期化
    if (FAILED(InitBuffer())) return E_FAIL;

    return S_OK;
}

// デバイス関連初期化
HRESULT InitDevice(HWND hWnd)
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
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;
    swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;
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
    HRESULT hr;
    for (size_t i = 0; i < driverTypes.size(); i++)
    {
        hr = D3D11CreateDeviceAndSwapChain(
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
        if (SUCCEEDED(hr)) break;
    }
    if (FAILED(hr))
    {
        MessageBox(NULL, L"DirectX11に対応していないデバイスです。", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    return S_OK;
}

// ビュー関連初期化
VOID InitView()
{
    // 表示領域を作成
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.Width = WINDOW_WIDTH;
    viewport.Height = WINDOW_HEIGHT;
    viewport.MinDepth = D3D11_MIN_DEPTH;    // 0.0f
    viewport.MaxDepth = D3D11_MAX_DEPTH;    // 1.0f
    g_context->RSSetViewports(1, &viewport);

    // バックバッファを作成
    ID3D11Texture2D* backBuffer;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTargetView);
    SAFE_RELEASE(backBuffer);

    // レンダーターゲットをバックバッファに設定
    g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);
}

// シェーダ関連初期化
HRESULT InitShader()
{
    ID3DBlob* vertexShader, * pixelShader;
    ID3DBlob* errorBlob = nullptr;

    // 両方のシェーダをロードしコンパイル
    if (FAILED(D3DCompileFromFile(L"shaders.hlsl"
        , nullptr
        , D3D_COMPILE_STANDARD_FILE_INCLUDE
        , "vsMain", "vs_4_0"
        , D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        , 0
        , &vertexShader
        , &errorBlob)))
    {
        MessageBox(NULL, L"頂点シェーダを読み込めませんでした。", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }
    if (FAILED(D3DCompileFromFile(L"shaders.hlsl"
        , nullptr
        , D3D_COMPILE_STANDARD_FILE_INCLUDE
        , "psMain", "ps_4_0"
        , D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        , 0
        , &pixelShader
        , &errorBlob)))
    {
        MessageBox(NULL, L"ピクセルシェーダを読み込めませんでした。", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }
    // カプセル化
    g_device->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), NULL, &g_vertexShader);
    g_device->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), NULL, &g_pixelShader);

    // 頂点インプットレイアウトを定義
    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    // インプットレイアウトのサイズ
    UINT numElements = sizeof(inputElementDescs) / sizeof(inputElementDescs[0]);

    // 頂点インプットレイアウトを作成
    if (FAILED(g_device->CreateInputLayout(inputElementDescs, numElements, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), &g_layout)))
    {
        MessageBox(NULL, L"頂点インプットレイアウトの定義が間違っています。", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    SAFE_RELEASE(vertexShader);
    SAFE_RELEASE(pixelShader);

    // 頂点インプットレイアウトをセット
    g_context->IASetInputLayout(g_layout);

    return S_OK;
}

// バッファ関連初期化
HRESULT InitBuffer()
{
    // 三角形のジオメトリを定義
    Vertex vertices[] =
    {
        { { 0.0f,   0.25f * g_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },    // 上-赤
        { { 0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },    // 右-緑
        { {-0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },    // 左-青
    };

    // バッファを作成
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(bufferDesc));

    // 頂点バッファの設定
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;                 // CPUおよびGPUによる書き込みアクセス
    bufferDesc.ByteWidth = sizeof(Vertex) * 3;              // サイズはVertex構造体×3
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        // 頂点バッファを使用
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;     // CPUがバッファに書き込むことを許可

    // 頂点バッファを作成
    if (FAILED(g_device->CreateBuffer(&bufferDesc, nullptr, &g_vertexBuffer)))
    {
        MessageBox(NULL, L"頂点バッファを作成できませんでした。", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    // 表示する頂点バッファを選択
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, &g_vertexBuffer, &stride, &offset);

    // 使用するプリミティブタイプを設定
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファに頂点データをコピー
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    g_context->Map(g_vertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappedResource);   // バッファのマッピング
    memcpy(mappedResource.pData, vertices, sizeof(vertices));                               // データをコピー
    g_context->Unmap(g_vertexBuffer, NULL);

    return S_OK;
}
// 更新
VOID OnUpdate()
{
redNum += 1.0f / 4096.0f;
    if (redNum > 1.0f)redNum = 0;
}

// 描画
void OnRender()
{

    float color[4] = { redNum,0.0f,1.0f,1.0f };//赤,緑,青,Alpha
    g_context->ClearRenderTargetView(g_renderTargetView, color);

    // シェーダオブジェクトをセット
    g_context->VSSetShader(g_vertexShader, nullptr, 0);
    g_context->PSSetShader(g_pixelShader, nullptr, 0);

    // 頂点バッファをバックバッファに描画
    g_context->Draw(3, 0);

    // フレームを最終出力
    g_swapChain->Present(0 //フリップまでの待ち時間
        , 0);

}
// メモリ解放
VOID OnDestroy()
{
    SAFE_RELEASE(g_device);
    SAFE_RELEASE(g_context);
    SAFE_RELEASE(g_swapChain);
    SAFE_RELEASE(g_renderTargetView);
    SAFE_RELEASE(g_layout);
    SAFE_RELEASE(g_vertexShader);
    SAFE_RELEASE(g_pixelShader);
    SAFE_RELEASE(g_vertexBuffer);
    SAFE_RELEASE(g_indexBuffer);
    SAFE_RELEASE(g_constantBuffer);
}
