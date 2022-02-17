#include <windows.h>
#include <vector>
#include <concepts>

// DirectX11�̃R�[�h�Z�b�g
#include <d3d11.h>
#include<d3dcompiler.h>
#include <directxmath.h>

// ���C�u����
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

#define WINDOW_CLASS    L"DX11�̓���"
#define WINDOW_TITLE    WINDOW_CLASS
#define WINDOW_WIDTH    750
#define WINDOW_HEIGHT   500

#define SAFE_RELEASE(x) if(x) x->Release();

// �v���g�^�C�v�錾
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);    // �E�B���h�E�v���V�[�W��
HRESULT OnInit(HWND hWnd);          // ������
HRESULT InitDevice(HWND hWnd);      // �f�o�C�X�֘A������
VOID InitView();                 // �r���[�֘A������
HRESULT InitShader();               // �V�F�[�_�֘A������
HRESULT InitBuffer();               // �o�b�t�@�֘A������
VOID InitMatrix();                  // �}�g���b�N�X�֘A������
VOID OnUpdate();                    // �X�V
VOID OnRender();                    // �`��
VOID OnDestroy();                   // ���������
float redNum = 0;

// �\���̈�̐��@(�A�X�y�N�g��)
FLOAT g_aspectRatio = static_cast<FLOAT>(WINDOW_WIDTH) / static_cast<FLOAT>(WINDOW_HEIGHT);

// �p�C�v���C���I�u�W�F�N�g
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

// ���_�\����
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

// �萔�\����
struct ConstantBuffer
{
    DirectX::XMMATRIX m_WVP;
};


// Y����]�ϐ�
static FLOAT y = 0;

// ���C���֐�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // �E�B���h�E�N���X��������
    WNDCLASSEX	windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS;
    RegisterClassEx(&windowClass);

    // �E�B���h�E�̃T�C�Y�����߂�
    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // �E�B���h�E�n���h�����쐬
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

    // �E�B���h�E�̕\��
    ShowWindow(hWnd, SW_SHOW);

    // ���C�����[�v
    MSG	msg = {};
    ZeroMemory(&msg, sizeof(msg));
    if (SUCCEEDED(OnInit(hWnd)))   // DirectX�̏�����
    {
        // �E�B���h�E�̕\��
        ShowWindow(hWnd, SW_SHOW);

        // ���C�����[�v
        while (msg.message != WM_QUIT)
        {
            // �L���[���̃��b�Z�[�W������
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                OnUpdate();     // �X�V
                OnRender();     // �`��
            }
        }
    }

    // WM_QUIT���b�Z�[�W�̕�����Windows�ɕԂ�
    return static_cast<char>(msg.wParam);
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
    case WM_DESTROY:    // �I����
        OnDestroy();    // ���������
        PostQuitMessage(0);
        return 0;
    }

    // switch�����������Ȃ��������b�Z�[�W������
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

// ������
HRESULT OnInit(HWND hWnd)
{
    // �f�o�C�X�֘A������
    if (FAILED(InitDevice(hWnd))) return E_FAIL;

    // �r���[�֘A������
    InitView();

    // �V�F�[�_�֘A������
    if (FAILED(InitShader())) return E_FAIL;

    // �o�b�t�@�֘A������
    if (FAILED(InitBuffer())) return E_FAIL;

    return S_OK;
}

// �f�o�C�X�֘A������
HRESULT InitDevice(HWND hWnd)
{
    // �h���C�o�[��ʂ��`
    std::vector<D3D_DRIVER_TYPE> driverTypes
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
        D3D_DRIVER_TYPE_SOFTWARE,
    };

    // �X���b�v�`�F�C���̍쐬
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

    // �h���C�o�[��ʂ��ォ�猟�؂��I��
    // �I�������f�o�C�X��p���ĕ`�悷��
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
        MessageBox(NULL, L"DirectX11�ɑΉ����Ă��Ȃ��f�o�C�X�ł��B", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    return S_OK;
}

// �r���[�֘A������
VOID InitView()
{
    // �\���̈���쐬
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.Width = WINDOW_WIDTH;
    viewport.Height = WINDOW_HEIGHT;
    viewport.MinDepth = D3D11_MIN_DEPTH;    // 0.0f
    viewport.MaxDepth = D3D11_MAX_DEPTH;    // 1.0f
    g_context->RSSetViewports(1, &viewport);

    // �o�b�N�o�b�t�@���쐬
    ID3D11Texture2D* backBuffer;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTargetView);
    SAFE_RELEASE(backBuffer);

    // �����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɐݒ�
    g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);
}

// �V�F�[�_�֘A������
HRESULT InitShader()
{
    ID3DBlob* vertexShader, * pixelShader;
    ID3DBlob* errorBlob = nullptr;

    // �����̃V�F�[�_�����[�h���R���p�C��
    if (FAILED(D3DCompileFromFile(L"shaders.hlsl"
        , nullptr
        , D3D_COMPILE_STANDARD_FILE_INCLUDE
        , "vsMain", "vs_4_0"
        , D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
        , 0
        , &vertexShader
        , &errorBlob)))
    {
        MessageBox(NULL, L"���_�V�F�[�_��ǂݍ��߂܂���ł����B", WINDOW_TITLE, MB_OK | MB_ICONERROR);
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
        MessageBox(NULL, L"�s�N�Z���V�F�[�_��ǂݍ��߂܂���ł����B", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }
    // �J�v�Z����
    g_device->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), NULL, &g_vertexShader);
    g_device->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), NULL, &g_pixelShader);

    // ���_�C���v�b�g���C�A�E�g���`
    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    // �C���v�b�g���C�A�E�g�̃T�C�Y
    UINT numElements = sizeof(inputElementDescs) / sizeof(inputElementDescs[0]);

    // ���_�C���v�b�g���C�A�E�g���쐬
    if (FAILED(g_device->CreateInputLayout(inputElementDescs, numElements, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), &g_layout)))
    {
        MessageBox(NULL, L"���_�C���v�b�g���C�A�E�g�̒�`���Ԉ���Ă��܂��B", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    SAFE_RELEASE(vertexShader);
    SAFE_RELEASE(pixelShader);

    // ���_�C���v�b�g���C�A�E�g���Z�b�g
    g_context->IASetInputLayout(g_layout);

    return S_OK;
}

// �o�b�t�@�֘A������
HRESULT InitBuffer()
{
    // �O�p�`�̃W�I���g�����`
    Vertex vertices[] =
    {
        { { 0.0f,   0.25f * g_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },    // ��-��
        { { 0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },    // �E-��
        { {-0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },    // ��-��
    };

    // �o�b�t�@���쐬
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(bufferDesc));

    // ���_�o�b�t�@�̐ݒ�
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;                 // CPU�����GPU�ɂ�鏑�����݃A�N�Z�X
    bufferDesc.ByteWidth = sizeof(Vertex) * 3;              // �T�C�Y��Vertex�\���́~3
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        // ���_�o�b�t�@���g�p
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;     // CPU���o�b�t�@�ɏ������ނ��Ƃ�����

    // ���_�o�b�t�@���쐬
    if (FAILED(g_device->CreateBuffer(&bufferDesc, nullptr, &g_vertexBuffer)))
    {
        MessageBox(NULL, L"���_�o�b�t�@���쐬�ł��܂���ł����B", WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    // �\�����钸�_�o�b�t�@��I��
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, &g_vertexBuffer, &stride, &offset);

    // �g�p����v���~�e�B�u�^�C�v��ݒ�
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ���_�o�b�t�@�ɒ��_�f�[�^���R�s�[
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    g_context->Map(g_vertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappedResource);   // �o�b�t�@�̃}�b�s���O
    memcpy(mappedResource.pData, vertices, sizeof(vertices));                               // �f�[�^���R�s�[
    g_context->Unmap(g_vertexBuffer, NULL);

    return S_OK;
}
// �X�V
VOID OnUpdate()
{
redNum += 1.0f / 4096.0f;
    if (redNum > 1.0f)redNum = 0;
}

// �`��
void OnRender()
{

    float color[4] = { redNum,0.0f,1.0f,1.0f };//��,��,��,Alpha
    g_context->ClearRenderTargetView(g_renderTargetView, color);

    // �V�F�[�_�I�u�W�F�N�g���Z�b�g
    g_context->VSSetShader(g_vertexShader, nullptr, 0);
    g_context->PSSetShader(g_pixelShader, nullptr, 0);

    // ���_�o�b�t�@���o�b�N�o�b�t�@�ɕ`��
    g_context->Draw(3, 0);

    // �t���[�����ŏI�o��
    g_swapChain->Present(0 //�t���b�v�܂ł̑҂�����
        , 0);

}
// ���������
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
