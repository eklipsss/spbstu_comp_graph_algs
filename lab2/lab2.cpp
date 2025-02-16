#include "framework.h"
#include "lab2.h"

#include <d3dcompiler.h>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

ID3D11Buffer* m_pVertexBuffer = nullptr;
ID3D11Buffer* m_pIndexBuffer = nullptr;

ID3D11PixelShader* m_pPixelShader = nullptr;
ID3D11VertexShader* m_pVertexShader = nullptr;
ID3D11InputLayout* m_pInputLayout = nullptr;

UINT                WindowWidth = 1280;
UINT                WindowHeight = 720;

HRESULT             InitDirectX(HWND hWnd);
HRESULT             InitScene();
HRESULT             CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);
bool                Render();
void                ResizeWindow(int width, int height);
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                Terminate();

struct Vertex
{
    float x, y, z;
    COLORREF color;
};


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    std::wstring dir;
    dir.resize(MAX_PATH + 1);
    GetCurrentDirectory(MAX_PATH + 1, &dir[0]);
    size_t configPos = dir.find(L"x64");
    if (configPos != std::wstring::npos)
    {
        dir.resize(configPos);
        dir += szTitle;
        SetCurrentDirectory(dir.c_str());
    }

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HWND hWnd = FindWindow(szWindowClass, szTitle);
    if (FAILED(InitDirectX(hWnd)))
    {
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();

        if (g_pRenderTargetView) g_pRenderTargetView->Release();
        if (g_pSwapChain) g_pSwapChain->Release();
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
        if (g_pd3dDevice) g_pd3dDevice->Release();

        return FALSE;
    }

    SetWindowText(hWnd, L"Lips Ekaterina");

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB2));

    MSG msg;

    bool exit = false;
    while (!exit)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT)
            {
                exit = true;
            }
        }
        else
        {
            Render();
        }
    }

    if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();

    Terminate();

    return (int)msg.wParam;
}


HRESULT InitDirectX(HWND hWnd)
{
    HRESULT result;

    IDXGIFactory* pFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    IDXGIAdapter* pSelectedAdapter = NULL;
    if (SUCCEEDED(result))
    {
        IDXGIAdapter* pAdapter = NULL;
        UINT adapterIdx = 0;
        while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
            {
                pSelectedAdapter = pAdapter;
                break;
            }

            pAdapter->Release();

            adapterIdx++;
        }
    }
    assert(pSelectedAdapter != NULL);

    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (SUCCEEDED(result))
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            flags, levels, 1, D3D11_SDK_VERSION, &g_pd3dDevice, &level, &g_pd3dDeviceContext);
        assert(level == D3D_FEATURE_LEVEL_11_0);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = WindowWidth;
        swapChainDesc.BufferDesc.Height = WindowHeight;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // протяжка в момент отжатия мыши
        //swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // мгновенная протяжка
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(g_pd3dDevice, &swapChainDesc, &g_pSwapChain);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        assert(SUCCEEDED(result));

        if (FAILED(result))
            return result;

        result = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        pBackBuffer->Release();
    }

    if (SUCCEEDED(result))
    {
        result = InitScene();
    }

    pFactory->Release();
    pSelectedAdapter->Release();

    if (FAILED(result))
    {
        Terminate();
    }

    return result;
}

HRESULT InitScene()
{
    static const Vertex Vertices[] = {
        {-0.5f * 1.5, -0.5f * 1.5, 0.0f, RGB(240, 128, 128)},
        { 0.5f * 1.5, -0.5f * 1.5, 0.0f, RGB(218, 112, 214)},
        { 0.0f * 1.5,  0.5f * 1.5, 0.0f, RGB(175, 238, 238)}
    };
    static const USHORT Indices[] = {
        0, 2, 1
    };
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Vertices;
        data.SysMemPitch = sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pVertexBuffer, "VertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndexBuffer, "IndexBuffer");
        }
    }

    ID3DBlob* pVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleColor.vs", (ID3D11DeviceChild**)&m_pVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleColor.ps", (ID3D11DeviceChild**)&m_pPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 2, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pInputLayout, "InputLayout");
        }
    }

    pVertexShaderCode->Release();

    return result;
}

HRESULT CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, path.c_str(), L"rb");
    assert(pFile != nullptr);
    if (pFile == nullptr)
    {
        return E_FAIL;
    }

    fseek(pFile, 0, SEEK_END);
    long long size = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);

    std::vector<char> data;
    data.resize(size + 1);

    size_t rd = fread(data.data(), 1, size, pFile);
    assert(rd == (size_t)size);

    fclose(pFile);

    std::wstring ext = Extension(path);

    std::string entryPoint = "";
    std::string platform = "";

    if (ext == L"vs")
    {
        entryPoint = "vs";
        platform = "vs_5_0";
    }
    else if (ext == L"ps")
    {
        entryPoint = "ps";
        platform = "ps_5_0";
    }

    UINT flags1 = 0;
#ifdef _DEBUG
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErrMsg = nullptr;
    HRESULT result = D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(), nullptr, nullptr, entryPoint.c_str(), platform.c_str(), flags1, 0, &pCode, &pErrMsg);
    if (!SUCCEEDED(result) && pErrMsg != nullptr)
    {
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());
    }
    assert(SUCCEEDED(result));

    if (pErrMsg!=nullptr)
    {
        pErrMsg->Release();
        pErrMsg = nullptr;

    }

    if (SUCCEEDED(result))
    {
        if (ext == L"vs")
        {
            ID3D11VertexShader* pVertexShader = nullptr;
            result = g_pd3dDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &pVertexShader);
            if (SUCCEEDED(result))
            {
                *ppShader = pVertexShader;
            }
        }
        else if (ext == L"ps")
        {
            ID3D11PixelShader* pPixelShader = nullptr;
            result = g_pd3dDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &pPixelShader);
            if (SUCCEEDED(result))
            {
                *ppShader = pPixelShader;
            }
        }
    }
    if (SUCCEEDED(result))
    {
        result = SetResourceName(*ppShader, WCSToMBS(path).c_str());
    }

    if (ppCode)
    {
        *ppCode = pCode;
    }
    else
    {
        pCode->Release();
    }

    return result;
}


bool Render()
{
    g_pd3dDeviceContext->ClearState();

    ID3D11RenderTargetView* views[] = { g_pRenderTargetView };
    g_pd3dDeviceContext->OMSetRenderTargets(1, views, nullptr);

    static const FLOAT BackColor[4] = { 0.4f, 0.2f, 0.5f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, BackColor);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)WindowWidth;
    viewport.Height = (FLOAT)WindowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_pd3dDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = WindowWidth;
    rect.bottom = WindowHeight;
    g_pd3dDeviceContext->RSSetScissorRects(1, &rect);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    g_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(3, 0, 0);

    HRESULT result = g_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(result));

    return SUCCEEDED(result);
}


void ResizeWindow(int width, int height)
{
    if (g_pRenderTargetView)
    {
        g_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        g_pRenderTargetView->Release();
        g_pRenderTargetView = nullptr;
    }

    HRESULT result = g_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    assert(SUCCEEDED(result));

    if (SUCCEEDED(result))
    {
        WindowWidth = width;
        WindowHeight = height;

        ID3D11Texture2D* pBackBuffer = nullptr;
        result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        assert(SUCCEEDED(result));

        if (SUCCEEDED(result))
        {
            result = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
            assert(SUCCEEDED(result));

            pBackBuffer->Release();
        }
    }
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB2));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB2);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    {
        RECT rc;
        rc.left = 0;
        rc.right = WindowWidth;
        rc.top = 0;
        rc.bottom = WindowHeight;

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

        MoveWindow(hWnd, 100, 100, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }

    return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_SIZE:
        if (g_pSwapChain && wParam != SIZE_MINIMIZED)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            ResizeWindow(rc.right - rc.left, rc.bottom - rc.top);
            SetWindowText(hWnd, L"Lips Ekaterina - Window Resized!");
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void Terminate()
{
    m_pInputLayout->Release();
    m_pPixelShader->Release();
    m_pVertexShader->Release();

    m_pIndexBuffer->Release();
    m_pVertexBuffer->Release();

    g_pRenderTargetView->Release();
    g_pSwapChain->Release();
    g_pd3dDeviceContext->Release();


#ifdef _DEBUG
    if (g_pd3dDevice != nullptr)
    {
        ID3D11Debug* pDebug = nullptr;
        HRESULT result = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&pDebug);
        assert(SUCCEEDED(result));
        if (pDebug != nullptr)
        {
            if (pDebug->AddRef() != 3) 
            {
                pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            }
            pDebug->Release();
        }
    }
#endif 

    g_pd3dDevice->Release();
}