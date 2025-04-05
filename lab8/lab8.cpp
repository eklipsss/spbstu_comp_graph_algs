﻿#include "framework.h"
#include "lab8.h"
#include "DDS.h"

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <d3dcompiler.h>
#include <chrono>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MAX_LOADSTRING 100
#define _USE_MATH_DEFINES
#include <math.h>

#include "Matrix.h"

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

ID3D11Texture2D* m_pDepthBuffer = nullptr;
ID3D11DepthStencilView* m_pDepthBufferDSV = nullptr;

ID3D11DepthStencilState* m_pDepthState = nullptr;
ID3D11DepthStencilState* m_pTransDepthState = nullptr;

ID3D11Buffer* m_pSceneBuffer = nullptr;
ID3D11Buffer* m_pGeomBufferInst = nullptr;
ID3D11Buffer* m_pGeomBufferInstVis = nullptr;
ID3D11Buffer* m_pVertexBuffer = nullptr;
ID3D11Buffer* m_pIndexBuffer = nullptr;

ID3D11PixelShader* m_pPixelShader = nullptr;
ID3D11VertexShader* m_pVertexShader = nullptr;
ID3D11InputLayout* m_pInputLayout = nullptr;

ID3D11Buffer* m_pSphereGeomBuffer = nullptr;
ID3D11Buffer* m_pSphereVertexBuffer = nullptr;
ID3D11Buffer* m_pSphereIndexBuffer = nullptr;
ID3D11PixelShader* m_pSpherePixelShader = nullptr;
ID3D11VertexShader* m_pSphereVertexShader = nullptr;
ID3D11InputLayout* m_pSphereInputLayout = nullptr;
UINT m_sphereIndexCount = 0;

ID3D11Buffer* m_pSmallLightSphereGeomBuffers[10] = { nullptr };
ID3D11Buffer* m_pSmallLightSphereVertexBuffer = nullptr;
ID3D11Buffer* m_pSmallLightSphereIndexBuffer = nullptr;
ID3D11PixelShader* m_pSmallLightSpherePixelShader = nullptr;
ID3D11VertexShader* m_pSmallLightSphereVertexShader = nullptr;
ID3D11InputLayout* m_pSmallLightSphereInputLayout = nullptr;
UINT m_SmallLightSphereIndexCount = 0;

ID3D11Buffer* m_pRectGeomBuffer = nullptr;
ID3D11Buffer* m_pRectGeomBuffer2 = nullptr;
ID3D11Buffer* m_pRectVertexBuffer = nullptr;
ID3D11Buffer* m_pRectIndexBuffer = nullptr;
ID3D11PixelShader* m_pRectPixelShader = nullptr;
ID3D11VertexShader* m_pRectVertexShader = nullptr;
ID3D11InputLayout* m_pRectInputLayout = nullptr;

ID3D11Texture2D* m_pCubemapTexture = nullptr;
ID3D11Texture2D* m_pSmallCubemapTexture = nullptr;
ID3D11Texture2D* m_pSmallCubemapTextureNM = nullptr;

ID3D11ShaderResourceView* m_pCubemapView = nullptr;
ID3D11ShaderResourceView* m_pSmallCubemapView = nullptr;
ID3D11ShaderResourceView* m_pSmallCubemapViewNM = nullptr;

ID3D11RasterizerState* m_pRasterizerState = nullptr;

ID3D11BlendState* m_pTransBlendState = nullptr;
ID3D11BlendState* m_pOpaqueBlendState = nullptr;

ID3D11Texture2D* m_pTexture = nullptr;
ID3D11ShaderResourceView* m_pTextureView = nullptr;
ID3D11Texture2D* m_pTextureNM = nullptr;
ID3D11ShaderResourceView* m_pTextureViewNM = nullptr;
ID3D11SamplerState* m_pSampler = nullptr;

ID3D11Texture2D* m_pColorBuffer = nullptr;
ID3D11RenderTargetView* m_pColorBufferRTV = nullptr;
ID3D11ShaderResourceView* m_pColorBufferSRV = nullptr;
ID3D11PixelShader* m_pBWPixelShader = nullptr;
ID3D11VertexShader* m_pBWVertexShader = nullptr;

ID3D11ComputeShader* m_pCullShader = nullptr;
ID3D11Buffer* m_pIndirectArgsSrc = nullptr;
ID3D11Buffer* m_pIndirectArgs = nullptr;
ID3D11Buffer* m_pCullParams = nullptr;
ID3D11Buffer* m_pGeomBufferInstVisGPU = nullptr;
ID3D11UnorderedAccessView* m_pGeomBufferInstVisGPU_UAV = nullptr;
ID3D11UnorderedAccessView* m_pIndirectArgsUAV = nullptr;
ID3D11Query* m_queries[10] = { nullptr };
UINT64 m_curFrame = 0;
UINT64 m_lastCompletedFrame = 0;


UINT                WindowWidth = 1280;
UINT                WindowHeight = 720;

struct Camera
{
    Point3f poi;    
    float r;       
    float phi;     
    float theta;    
};

Camera m_camera;
bool m_rbPressed;
int m_prevMouseX;
int m_prevMouseY;
bool m_rotateModel;
double m_angle;

bool m_showLightBulbs = 1;
bool m_useNormalMaps;
bool m_showNormals;
bool m_doCull = 1;
bool m_useBWFilter = 0;

bool m_computeCull = 0;
bool m_updateCullParams = 0;

int m_gpuVisibleInstances = 0;

size_t m_prevUSec;
const int MaxInst = 100;

struct TextureTangentVertex
{
    Point3f pos;
    Point3f tangent;
    Point3f norm;
    Point2f uv;
};

struct TextureVertex
{
    float x, y, z;
    float u, v;
};

struct ColorVertex
{
    float x, y, z;
    COLORREF color;
};

struct GeomBuffer
{
    DirectX::XMMATRIX m;
    DirectX::XMMATRIX normalM;
    Point4f shine;
    Point4f posAngle;
};

struct SphereGeomBuffer
{
    DirectX::XMMATRIX m;
    Point4f size;
    //DirectX::XMMATRIX normalM;
    //Point4f shine;
};

struct RectGeomBuffer
{
    DirectX::XMMATRIX m;
    Point4f color;
};

struct CullParams
{
    Point4i shapeCount; 
    Point4f bbMin[MaxInst];
    Point4f bbMax[MaxInst];
};

struct Light
{
    Point4f pos = Point4f{ 0,0,0,0 };
    Point4f color = Point4f{ 1,1,1,0 };
};

struct SceneBuffer
{
    DirectX::XMMATRIX vp;
    Point4f cameraPos;
    Point4i lightCount; 
    Point4i postProcess;
    Light lights[10];
    Point4f ambientColor;
    Point4f frustum[6]; 
};

struct AABB
{
    Point3f vmin = Point3f{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()
    };
    Point3f vmax = Point3f{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()
    };

    inline Point3f GetVert(int idx) const
    {
        return Point3f
        {
            (idx & 1) == 0 ? vmin.x : vmax.x,
            (idx & 2) == 0 ? vmin.y : vmax.y,
            (idx & 4) == 0 ? vmin.z : vmax.z
        };
    }
};

struct BoundingRect
{
    Point3f v[4];
};

AABB m_boundingRects[2];

SceneBuffer m_sceneBuffer;
std::vector<GeomBuffer> m_geomBuffers{ MaxInst };
std::vector<AABB> m_geomBBs{ MaxInst };
UINT m_instCount = 2;
UINT m_visibleInstances = 0;

static const float ModelRotationSpeed = (float)M_PI / 2.0f;

const Point3f Rect0Pos = Point3f{ 1.0f, 0, 0 };
const Point3f Rect1Pos = Point3f{ 1.2f, 0, 0 };


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT             InitDirectX(HWND hWnd);
HRESULT             SetupBackBuffer();
HRESULT             InitScene();
HRESULT             InitSphere();
HRESULT             InitSmallLightSphere();
HRESULT             InitRect();
HRESULT             InitCubemap();
HRESULT             InitPostProcess();
HRESULT             InitCull();
void                InitGeom(GeomBuffer& geomBuffer, AABB& bb);
HRESULT             CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, const std::vector<std::string>& defines = {}, ID3DBlob** ppCode = nullptr);
bool                Update();
void                ReadQueries();
void                CalcFrustum(Point4f frustum[6]);
void                CullBoxes();
bool                Render();
void                RenderSphere();
void                RenderSmallLightSpheres();
void                RenderRects();
void                RenderPostProcess();
void                ResizeWindow(int width, int height);
void                Terminate();
void                TermSphere();
void                TermRect();
void                TermDepthBuffer();
void                TermSmallLightSphere();
void                TermGPUCulling();


namespace
{
    void GetSphereDataSize(size_t latCells, size_t lonCells, size_t& indexCount, size_t& vertexCount)
    {
        vertexCount = (latCells + 1) * (lonCells + 1);
        indexCount = latCells * lonCells * 6;
    }

    void CreateSphere(size_t latCells, size_t lonCells, UINT16* pIndices, Point3f* pPos)
    {
        for (size_t lat = 0; lat < latCells + 1; lat++)
        {
            for (size_t lon = 0; lon < lonCells + 1; lon++)
            {
                int index = (int)(lat * (lonCells + 1) + lon);
                float lonAngle = 2.0f * (float)M_PI * lon / lonCells + (float)M_PI;
                float latAngle = -(float)M_PI / 2 + (float)M_PI * lat / latCells;

                Point3f r = Point3f{ sinf(lonAngle) * cosf(latAngle), sinf(latAngle), cosf(lonAngle) * cosf(latAngle) };

                pPos[index] = r * 0.5f;
            }
        }

        for (size_t lat = 0; lat < latCells; lat++)
        {
            for (size_t lon = 0; lon < lonCells; lon++)
            {
                size_t index = lat * lonCells * 6 + lon * 6;
                pIndices[index + 0] = (UINT16)(lat * (latCells + 1) + lon + 0);
                pIndices[index + 2] = (UINT16)(lat * (latCells + 1) + lon + 1);
                pIndices[index + 1] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
                pIndices[index + 3] = (UINT16)(lat * (latCells + 1) + lon + 1);
                pIndices[index + 5] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon + 1);
                pIndices[index + 4] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
            }
        }
    }

    Point4f BuildPlane(const Point3f& p0, const Point3f& p1, const Point3f& p2, const Point3f& p3)
    {
        Point3f norm = (p1 - p0).cross(p3 - p0);
        norm.normalize();
        Point3f pos = (p0 + p1 + p2 + p3) * 0.25f;

        return Point4f(norm.x, norm.y, norm.z, -pos.dot(norm));
    }

    bool IsBoxInside(const Point4f frustum[6], const Point3f& bbMin, const Point3f& bbMax)
    {
        for (int i = 0; i < 6; i++)
        {
            const Point3f norm = frustum[i];
            Point4f p(
                signbit(norm.x) ? bbMin.x : bbMax.x,
                signbit(norm.y) ? bbMin.y : bbMax.y,
                signbit(norm.z) ? bbMin.z : bbMax.z,
                1.0f
            );
            float s = p.dot(frustum[i]);
            if (s < 0.0f)
            {
                return false;
            }
        }

        return true;
    }

}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    srand(12345);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB8, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    std::wstring dir;
    dir.resize(MAX_PATH + 1);
    GetCurrentDirectory(MAX_PATH + 1, &dir[0]);
    size_t configPos = dir.find(L"x64");
    if (configPos != std::wstring::npos)
    {
        dir.resize(configPos);
        //dir += szTitle;
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

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB8));

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
        if (Update())
        {
            Render();
        }
    }

    if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();

    Terminate();

    return (int)msg.wParam;
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB8));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB8);
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
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

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

    case WM_LBUTTONDOWN:
        m_rbPressed = true;
        m_prevMouseX = LOWORD(lParam);
        m_prevMouseY = HIWORD(lParam);
        break;

    case WM_LBUTTONUP:
        m_rbPressed = false;
        break;

    case WM_MOUSEMOVE:
        if (m_rbPressed)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int dx = x - m_prevMouseX;
            int dy = y - m_prevMouseY;

            m_camera.phi -= dx * 0.01f;
            m_camera.theta -= dy * 0.01f;

            m_prevMouseX = x;
            m_prevMouseY = y;
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


class D3DInclude : public ID3DInclude
{
    STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        FILE* pFile = nullptr;
        fopen_s(&pFile, pFileName, "rb");
        assert(pFile != nullptr);
        if (pFile == nullptr)
        {
            return E_FAIL;
        }

        fseek(pFile, 0, SEEK_END);
        long long size = _ftelli64(pFile);
        fseek(pFile, 0, SEEK_SET);

        VOID* pData = malloc(size);
        if (pData == nullptr)
        {
            fclose(pFile);
            return E_FAIL;
        }

        size_t rd = fread(pData, 1, size, pFile);
        assert(rd == (size_t)size);

        if (rd != (size_t)size)
        {
            fclose(pFile);
            free(pData);
            return E_FAIL;
        }

        *ppData = pData;
        *pBytes = (UINT)size;

        return S_OK;
    }
    STDMETHOD(Close)(THIS_ LPCVOID pData)
    {
        free(const_cast<void*>(pData));
        return S_OK;
    }
};


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
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; 
        //swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; 
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(g_pd3dDevice, &swapChainDesc, &g_pSwapChain);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = SetupBackBuffer();
    }


    if (SUCCEEDED(result))
    {
        result = InitScene();
    }

    if (SUCCEEDED(result))
    {
        m_camera.poi = Point3f{ 0,0,0 };
        m_camera.r = 5.0f;
        //m_camera.r = 10.0f;
        m_camera.phi = -(float)M_PI / 4;
        m_camera.theta = (float)M_PI / 4;
    }

    SAFE_RELEASE(pSelectedAdapter);
    SAFE_RELEASE(pFactory);

    if (SUCCEEDED(result))
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        m_sceneBuffer.lightCount.x = 1;
        m_sceneBuffer.lights[0].pos = Point4f{ 0, 1.05f, 0, 1 };
        m_sceneBuffer.lights[0].color = Point4f{ 0.81,0.58,1 };
        m_sceneBuffer.ambientColor = Point4f(0, 0, 0.2f, 0);
    }

    if (FAILED(result))
    {
        Terminate();
    }

    return result;
}


HRESULT SetupBackBuffer()
{
    ID3D11Texture2D* pBackBuffer = NULL;
    HRESULT result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);

        SAFE_RELEASE(pBackBuffer);
    }
    if (SUCCEEDED(result))
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.ArraySize = 1;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.Height = WindowHeight;
        desc.Width = WindowWidth;
        desc.MipLevels = 1;

        result = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthBuffer, "DepthBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateDepthStencilView(m_pDepthBuffer, nullptr, &m_pDepthBufferDSV);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthBuffer, "DepthBufferView");
        }
    }

    SAFE_RELEASE(m_pColorBuffer);
    SAFE_RELEASE(m_pColorBufferRTV);
    SAFE_RELEASE(m_pColorBufferSRV);

    if (SUCCEEDED(result))
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ArraySize = 1;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.Height = WindowHeight;
        desc.Width = WindowWidth;
        desc.MipLevels = 1;

        result = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &m_pColorBuffer);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pColorBuffer, "ColorBuffer");
        }
    }
    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateRenderTargetView(m_pColorBuffer, nullptr, &m_pColorBufferRTV);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pColorBufferRTV, "ColorBufferRTV");
        }
    }
    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateShaderResourceView(m_pColorBuffer, nullptr, &m_pColorBufferSRV);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pColorBufferSRV, "ColorBufferSRV");
        }
    }

    assert(SUCCEEDED(result));

    return result;
}


HRESULT InitScene()
{
    // Textured cube
    static const TextureTangentVertex Vertices[24] = {
        // Bottom face
        {Point3f{-0.5, -0.5,  0.5}, Point3f{1, 0, 0}, Point3f{0, -1, 0}, Point2f{0, 1}},
        {Point3f{ 0.5, -0.5,  0.5}, Point3f{1, 0, 0}, Point3f{0, -1, 0}, Point2f{1, 1}},
        {Point3f{ 0.5, -0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, -1, 0}, Point2f{1, 0}},
        {Point3f{-0.5, -0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, -1, 0}, Point2f{0, 0}},
        // Top face
        {Point3f{-0.5,  0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 1, 0}, Point2f{0, 1}},
        {Point3f{ 0.5,  0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 1, 0}, Point2f{1, 1}},
        {Point3f{ 0.5,  0.5,  0.5}, Point3f{1, 0, 0}, Point3f{0, 1, 0}, Point2f{1, 0}},
        {Point3f{-0.5,  0.5,  0.5}, Point3f{1, 0, 0}, Point3f{0, 1, 0}, Point2f{0, 0}},
        // Front face
        {Point3f{ 0.5, -0.5, -0.5}, Point3f{0, 0, 1}, Point3f{1, 0, 0}, Point2f{0, 1}},
        {Point3f{ 0.5, -0.5,  0.5}, Point3f{0, 0, 1}, Point3f{1, 0, 0}, Point2f{1, 1}},
        {Point3f{ 0.5,  0.5,  0.5}, Point3f{0, 0, 1}, Point3f{1, 0, 0}, Point2f{1, 0}},
        {Point3f{ 0.5,  0.5, -0.5}, Point3f{0, 0, 1}, Point3f{1, 0, 0}, Point2f{0, 0}},
        // Back face
        {Point3f{-0.5, -0.5,  0.5}, Point3f{0, 0, -1}, Point3f{-1, 0, 0}, Point2f{0, 1}},
        {Point3f{-0.5, -0.5, -0.5}, Point3f{0, 0, -1}, Point3f{-1, 0, 0}, Point2f{1, 1}},
        {Point3f{-0.5,  0.5, -0.5}, Point3f{0, 0, -1}, Point3f{-1, 0, 0}, Point2f{1, 0}},
        {Point3f{-0.5,  0.5,  0.5}, Point3f{0, 0, -1}, Point3f{-1, 0, 0}, Point2f{0, 0}},
        // Left face
        {Point3f{ 0.5, -0.5,  0.5}, Point3f{-1, 0, 0}, Point3f{0, 0, 1}, Point2f{0, 1}},
        {Point3f{-0.5, -0.5,  0.5}, Point3f{-1, 0, 0}, Point3f{0, 0, 1}, Point2f{1, 1}},
        {Point3f{-0.5,  0.5,  0.5}, Point3f{-1, 0, 0}, Point3f{0, 0, 1}, Point2f{1, 0}},
        {Point3f{ 0.5,  0.5,  0.5}, Point3f{-1, 0, 0}, Point3f{0, 0, 1}, Point2f{0, 0}},
        // Right face
        {Point3f{-0.5, -0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 0, -1}, Point2f{0, 1}},
        {Point3f{ 0.5, -0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 0, -1}, Point2f{1, 1}},
        {Point3f{ 0.5,  0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 0, -1}, Point2f{1, 0}},
        {Point3f{-0.5,  0.5, -0.5}, Point3f{1, 0, 0}, Point3f{0, 0, -1}, Point2f{0, 0}}
    };
    static const UINT16 Indices[36] = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22
    };

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
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
        result = CompileAndCreateShader(L"SimpleTexture.vs", (ID3D11DeviceChild**)&m_pVertexShader, {}, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTexture.ps", (ID3D11DeviceChild**)&m_pPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 4, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pInputLayout, "InputLayout");
        }
    }

    SAFE_RELEASE(pVertexShaderCode);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(GeomBuffer) * MaxInst;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInst);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pGeomBufferInst, "GeomBufferInst");
        }
        if (SUCCEEDED(result))
        {
            const float diag = sqrtf(2.0f) / 2.0f * 0.5f;

            m_geomBuffers[0].shine.x = 0.0f;
            m_geomBuffers[0].shine.y = ModelRotationSpeed;
            m_geomBuffers[0].shine.z = 0.0f;
            int useNM = 1;
            m_geomBuffers[0].shine.w = *reinterpret_cast<float*>(&useNM);
            m_geomBuffers[0].posAngle = Point4f{ 0.00001f, 0, 0, 0 };
            m_geomBBs[0].vmin = m_geomBuffers[0].posAngle + Point3f{ -diag, -0.5f, -diag };
            m_geomBBs[0].vmax = m_geomBuffers[0].posAngle + Point3f{ diag,  0.5f,  diag };

            m_geomBuffers[1].shine.x = 64.0f;
            m_geomBuffers[1].shine.y = 0.0f;
            m_geomBuffers[1].shine.z = 0.0f;
            m_geomBuffers[1].shine.w = *reinterpret_cast<float*>(&useNM);
            m_geomBuffers[1].posAngle = Point4f{ 2.0f, 0, 0, 0 };
            DirectX::XMMATRIX m = DirectX::XMMatrixMultiply(
                DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -(float)m_geomBuffers[1].posAngle.w),
                DirectX::XMMatrixTranslation(m_geomBuffers[1].posAngle.x, m_geomBuffers[1].posAngle.y, m_geomBuffers[1].posAngle.z)
            );
            m_geomBuffers[1].m = m;
            m = DirectX::XMMatrixInverse(nullptr, m);
            m = DirectX::XMMatrixTranspose(m);
            m_geomBuffers[1].normalM = m;
            m_geomBBs[1].vmin = m_geomBuffers[1].posAngle + Point3f{ -0.5f, -0.5f, -0.5f };
            m_geomBBs[1].vmax = m_geomBuffers[1].posAngle + Point3f{ 0.5f, 0.5f, 0.5f };

            for (int i = 2; i < 10; i++)
            {
                InitGeom(m_geomBuffers[i], m_geomBBs[i]);
            }
            m_instCount = 10;
            m_updateCullParams = true; 
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Point4i) * MaxInst;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInstVis);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pGeomBufferInstVis, "GeomBufferInstVis");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SceneBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pSceneBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSceneBuffer, "SceneBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.AntialiasedLineEnable = FALSE;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = 0;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthBiasClamp = 0.0f;
        desc.DepthClipEnable = TRUE;
        desc.ScissorEnable = FALSE;
        desc.MultisampleEnable = FALSE;

        result = g_pd3dDevice->CreateRasterizerState(&desc, &m_pRasterizerState);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRasterizerState, "RasterizerState");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        result = g_pd3dDevice->CreateBlendState(&desc, &m_pTransBlendState);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTransBlendState, "TransBlendState");
        }
        if (SUCCEEDED(result))
        {
            desc.RenderTarget[0].BlendEnable = FALSE;
            result = g_pd3dDevice->CreateBlendState(&desc, &m_pOpaqueBlendState);
        }
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pOpaqueBlendState, "OpaqueBlendState");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = FALSE;

        result = g_pd3dDevice->CreateDepthStencilState(&desc, &m_pDepthState);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pDepthState, "DepthState");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_GREATER;
        desc.StencilEnable = FALSE;

        result = g_pd3dDevice->CreateDepthStencilState(&desc, &m_pTransDepthState);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTransDepthState, "TransDepthState");
        }
    }

    DXGI_FORMAT textureFmt;
    if (SUCCEEDED(result))
    {
        TextureDesc textureDesc[2];
        bool ddsRes = LoadDDS(L"stone.dds", textureDesc[0]);
        if (ddsRes)
        {
            ddsRes = LoadDDS(L"puppy.dds", textureDesc[1]);
        }

        textureFmt = textureDesc[0].fmt;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureDesc[0].fmt;
        desc.ArraySize = 2;
        desc.MipLevels = textureDesc[0].mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = textureDesc[0].height;
        desc.Width = textureDesc[0].width;

        std::vector<D3D11_SUBRESOURCE_DATA> data;
        data.resize(desc.MipLevels * 2);
        for (UINT32 j = 0; j < 2; j++)
        {
            UINT32 blockWidth = DivUp(desc.Width, 4u);
            UINT32 blockHeight = DivUp(desc.Height, 4u);
            UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);
            const char* pSrcData = reinterpret_cast<const char*>(textureDesc[j].pData);

            for (UINT32 i = 0; i < desc.MipLevels; i++)
            {
                data[j * desc.MipLevels + i].pSysMem = pSrcData;
                data[j * desc.MipLevels + i].SysMemPitch = pitch;
                data[j * desc.MipLevels + i].SysMemSlicePitch = 0;

                pSrcData += pitch * blockHeight;
                blockHeight = std::max(1u, blockHeight / 2);
                blockWidth = std::max(1u, blockWidth / 2);
                pitch = blockWidth * GetBytesPerBlock(desc.Format);
            }
        }
        result = g_pd3dDevice->CreateTexture2D(&desc, data.data(), &m_pTexture);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTexture, WCSToMBS(L"Diffuse textures"));
        }
        for (UINT32 j = 0; j < 2; j++)
        {
            free(textureDesc[j].pData);
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = textureFmt;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.ArraySize = 2;
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.MipLevels = 11;
        desc.Texture2DArray.MostDetailedMip = 0;

        result = g_pd3dDevice->CreateShaderResourceView(m_pTexture, &desc, &m_pTextureView);
        assert(SUCCEEDED(result));
    }
    if (SUCCEEDED(result))
    {
        const std::wstring TextureName = L"stoneNM.dds";

        TextureDesc textureDesc;
        bool ddsRes = LoadDDS(TextureName.c_str(), textureDesc);

        textureFmt = textureDesc.fmt;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureDesc.fmt;
        desc.ArraySize = 1;
        desc.MipLevels = textureDesc.mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = textureDesc.height;
        desc.Width = textureDesc.width;

        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);
        const char* pSrcData = reinterpret_cast<const char*>(textureDesc.pData);

        std::vector<D3D11_SUBRESOURCE_DATA> data;
        data.resize(desc.MipLevels);
        for (UINT32 i = 0; i < desc.MipLevels; i++)
        {
            data[i].pSysMem = pSrcData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;

            pSrcData += pitch * blockHeight;
            blockHeight = std::max(1u, blockHeight / 2);
            blockWidth = std::max(1u, blockWidth / 2);
            pitch = blockWidth * GetBytesPerBlock(desc.Format);
        }
        result = g_pd3dDevice->CreateTexture2D(&desc, data.data(), &m_pTextureNM);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTextureNM, WCSToMBS(TextureName));
        }

        free(textureDesc.pData);
    }
    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = textureFmt;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 11;
        desc.Texture2D.MostDetailedMip = 0;

        result = g_pd3dDevice->CreateShaderResourceView(m_pTextureNM, &desc, &m_pTextureViewNM);
    }
    if (SUCCEEDED(result))
    {
        D3D11_SAMPLER_DESC desc = {};

        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD = -FLT_MAX;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;

        result = g_pd3dDevice->CreateSamplerState(&desc, &m_pSampler);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = InitSphere();
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = InitCubemap();
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = InitRect();
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = InitSmallLightSphere();
    }

    if (SUCCEEDED(result))
    {
        result = InitPostProcess();
    }

    if (SUCCEEDED(result))
    {
        result = InitCull();
    }

    return result;
}


HRESULT InitSphere()
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    static const size_t SphereSteps = 32;

    std::vector<Point3f> sphereVertices;
    std::vector<UINT16> indices;

    size_t indexCount;
    size_t vertexCount;

    GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

    sphereVertices.resize(vertexCount);
    indices.resize(indexCount);

    m_sphereIndexCount = (UINT)indexCount;

    CreateSphere(SphereSteps, SphereSteps, indices.data(), sphereVertices.data());

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(sphereVertices.size() * sizeof(Point3f));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = sphereVertices.data();
        data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Point3f));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereVertexBuffer, "SphereVertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(indices.size() * sizeof(UINT16));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = indices.data();
        data.SysMemPitch = (UINT)(indices.size() * sizeof(UINT16));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereIndexBuffer, "SphereIndexBuffer");
        }
    }

    ID3DBlob* pSphereVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SphereTexture.vs", (ID3D11DeviceChild**)&m_pSphereVertexShader, {}, &pSphereVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SphereTexture.ps", (ID3D11DeviceChild**)&m_pSpherePixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 1, pSphereVertexShaderCode->GetBufferPointer(), pSphereVertexShaderCode->GetBufferSize(), &m_pSphereInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereInputLayout, "SphereInputLayout");
        }
    }

    SAFE_RELEASE(pSphereVertexShaderCode);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SphereGeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        SphereGeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixIdentity();
        geomBuffer.size.x = 2.0f;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBuffer;
        data.SysMemPitch = sizeof(geomBuffer);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereGeomBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereGeomBuffer, "SphereGeomBuffer");
        }
    }

    return result;
}


HRESULT InitSmallLightSphere()
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    static const size_t SphereSteps = 8;

    std::vector<Point3f> sphereVertices;
    std::vector<UINT16> indices;

    size_t indexCount;
    size_t vertexCount;

    GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

    sphereVertices.resize(vertexCount);
    indices.resize(indexCount);

    m_SmallLightSphereIndexCount = (UINT)indexCount;

    CreateSphere(SphereSteps, SphereSteps, indices.data(), sphereVertices.data());

    for (auto& v : sphereVertices)
    {
        v = v * 0.125f;
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(sphereVertices.size() * sizeof(Point3f));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = sphereVertices.data();
        data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Point3f));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSmallLightSphereVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSmallLightSphereVertexBuffer, "SmallLightSphereVertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(indices.size() * sizeof(UINT16));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = indices.data();
        data.SysMemPitch = (UINT)(indices.size() * sizeof(UINT16));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSmallLightSphereIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSmallLightSphereIndexBuffer, "SmallLightSphereIndexBuffer");
        }
    }

    ID3DBlob* pSmallSphereVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"TransColor.vs", (ID3D11DeviceChild**)&m_pSmallLightSphereVertexShader, {}, &pSmallSphereVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"TransColor.ps", (ID3D11DeviceChild**)&m_pSmallLightSpherePixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 1, pSmallSphereVertexShaderCode->GetBufferPointer(), pSmallSphereVertexShaderCode->GetBufferSize(), &m_pSmallLightSphereInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSmallLightSphereInputLayout, "SmallLightSphereInputLayout");
        }
    }

    SAFE_RELEASE(pSmallSphereVertexShaderCode);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(RectGeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        RectGeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixIdentity();
        geomBuffer.color = Point4f{ 1,1,1,1 };

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBuffer;
        data.SysMemPitch = sizeof(geomBuffer);
        data.SysMemSlicePitch = 0;

        for (int i = 0; i < 10 && SUCCEEDED(result); i++)
        {
            result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSmallLightSphereGeomBuffers[i]);
            if (SUCCEEDED(result))
            {
                result = SetResourceName(m_pSmallLightSphereGeomBuffers[i], "SmallLightSphereGeomBuffer");
            }
        }
    }

    assert(SUCCEEDED(result));

    return result;
}


HRESULT InitRect()
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    static const ColorVertex Vertices[] =
    {
        {0.0, -1.0, -1.5, RGB(128,0,128)},
        {0.0,  1.0, -1.5, RGB(128,0,128)},
        {0.0,  1.0,  1.5, RGB(128,0,128)},
        {0.0, -1.0,  1.5, RGB(128,0,128)}
    };
    static const UINT16 Indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            m_boundingRects[j].vmin.x = std::min(m_boundingRects[j].vmin.x, Vertices[i].x);
            m_boundingRects[j].vmin.y = std::min(m_boundingRects[j].vmin.y, Vertices[i].y);
            m_boundingRects[j].vmin.z = std::min(m_boundingRects[j].vmin.z, Vertices[i].z);
            m_boundingRects[j].vmax.x = std::max(m_boundingRects[j].vmax.x, Vertices[i].x);
            m_boundingRects[j].vmax.y = std::max(m_boundingRects[j].vmax.y, Vertices[i].y);
            m_boundingRects[j].vmax.z = std::max(m_boundingRects[j].vmax.z, Vertices[i].z);
        }
    }
    m_boundingRects[0].vmin = m_boundingRects[0].vmin + Rect0Pos;
    m_boundingRects[1].vmin = m_boundingRects[1].vmin + Rect0Pos;
    m_boundingRects[1].vmin = m_boundingRects[1].vmin + Rect1Pos;
    m_boundingRects[1].vmax = m_boundingRects[1].vmax + Rect1Pos;

    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = Vertices;
        data.SysMemPitch = (UINT)sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pRectVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRectVertexBuffer, "RectVertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = Indices;
        data.SysMemPitch = (UINT)sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pRectIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRectIndexBuffer, "RectIndexBuffer");
        }
    }

    ID3DBlob* pRectVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"TransColor.vs", (ID3D11DeviceChild**)&m_pRectVertexShader, {}, &pRectVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"TransColor.ps", (ID3D11DeviceChild**)&m_pRectPixelShader, { "USE_LIGHTS" });
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 2, pRectVertexShaderCode->GetBufferPointer(), pRectVertexShaderCode->GetBufferSize(), &m_pRectInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRectInputLayout, "RectInputLayout");
        }
    }

    SAFE_RELEASE(pRectVertexShaderCode);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(RectGeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        RectGeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixTranslation(Rect0Pos.x, Rect0Pos.y, Rect0Pos.z);
        geomBuffer.color = Point4f{ 0.75f, 0, 0.05f, 0.5f };

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBuffer;
        data.SysMemPitch = sizeof(geomBuffer);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pRectGeomBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRectGeomBuffer, "RectGeomBuffer");
        }

        if (SUCCEEDED(result))
        {
            geomBuffer.m = DirectX::XMMatrixTranslation(Rect1Pos.x, Rect1Pos.y, Rect1Pos.z);
            geomBuffer.color = Point4f{ 0.5f, 0.6f, 0.15, 0.5f };

            result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pRectGeomBuffer2);
        }
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRectGeomBuffer2, "RectGeomBuffer2");
        }
    }

    return result;
}


HRESULT InitCubemap()
{
    HRESULT result = S_OK;

    DXGI_FORMAT textureFmt;
    if (SUCCEEDED(result))
    {
        const std::wstring TextureNames[6] =
        {
            L"px.dds", L"nx.dds",
            L"py.dds", L"ny.dds",
            L"pz.dds", L"nz.dds"
        };
        TextureDesc texDescs[6];
        bool ddsRes = true;
        for (int i = 0; i < 6 && ddsRes; i++)
        {
            ddsRes = LoadDDS(TextureNames[i].c_str(), texDescs[i], true);
        }

        textureFmt = texDescs[0].fmt; 

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureFmt;
        desc.ArraySize = 6;
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = texDescs[0].height;
        desc.Width = texDescs[0].width;

        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);

        D3D11_SUBRESOURCE_DATA data[6];
        for (int i = 0; i < 6; i++)
        {
            data[i].pSysMem = texDescs[i].pData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;
        }
        result = g_pd3dDevice->CreateTexture2D(&desc, data, &m_pCubemapTexture);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapTexture, "CubemapTexture");
        }
        for (int i = 0; i < 6; i++)
        {
            free(texDescs[i].pData);
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format = textureFmt;
        desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
        desc.TextureCube.MipLevels = 1;
        desc.TextureCube.MostDetailedMip = 0;

        result = g_pd3dDevice->CreateShaderResourceView(m_pCubemapTexture, &desc, &m_pCubemapView);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapView, "CubemapView");
        }
    }

    return result;
}


HRESULT InitPostProcess()
{
    HRESULT result = S_OK;

    result = CompileAndCreateShader(L"BW.vs", (ID3D11DeviceChild**)&m_pBWVertexShader);
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"BW.ps", (ID3D11DeviceChild**)&m_pBWPixelShader);
    }

    return result;
}


HRESULT InitCull()
{
    HRESULT result = S_OK;

    result = CompileAndCreateShader(L"FrustumCull.cs", (ID3D11DeviceChild**)&m_pCullShader);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(UINT);

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pIndirectArgsSrc);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndirectArgsSrc, "IndirectArgsSrc");
        }
        if (SUCCEEDED(result))
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;
            uavDesc.Buffer.NumElements = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) / sizeof(UINT);
            uavDesc.Buffer.Flags = 0;

            result = g_pd3dDevice->CreateUnorderedAccessView(m_pIndirectArgsSrc, &uavDesc, &m_pIndirectArgsUAV);
        }
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndirectArgsSrc, "IndirectArgsUAV");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pIndirectArgs);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndirectArgsSrc, "IndirectArgs");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = sizeof(CullParams);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pCullParams);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCullParams, "CullParams");
        }
    }

    if (SUCCEEDED(result))
    {

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Point4i) * MaxInst;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(Point4i);

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInstVisGPU);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pGeomBufferInstVisGPU, "GeomBufferInstVisGPU");
        }
        if (SUCCEEDED(result))
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;
            uavDesc.Buffer.NumElements = MaxInst;
            uavDesc.Buffer.Flags = 0;

            result = g_pd3dDevice->CreateUnorderedAccessView(m_pGeomBufferInstVisGPU, &uavDesc, &m_pGeomBufferInstVisGPU_UAV);
        }
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndirectArgsSrc, "GeomBufferInstVisGPU_UAV");
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_QUERY_DESC desc;
        desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
        desc.MiscFlags = 0;
        for (int i = 0; i < 10 && SUCCEEDED(result); i++)
        {
            result = g_pd3dDevice->CreateQuery(&desc, &m_queries[i]);
        }
    }

    assert(SUCCEEDED(result));

    return result;
}


void InitGeom(GeomBuffer& geomBuffer, AABB& bb)
{
    Point3f offset = Point3f{ randNormf(), randNormf(), randNormf() } *5.0f - Point3f{ 5.5f, 2.5f, 2.5f };

    geomBuffer.shine.x = randNormf() > 0.5f ? 64.0f : 0.0f;
    geomBuffer.shine.y = randNormf() * 3 * (float)M_PI;
    geomBuffer.posAngle = Point4f{ offset.x, offset.y, offset.z, 0 };

    const float diag = sqrtf(2.0f) / 2.0f * 0.5f;
    bb.vmin = geomBuffer.posAngle + Point3f{ -diag, -0.5f, -diag };
    bb.vmax = geomBuffer.posAngle + Point3f{ diag,  0.5f,  diag };

    int useNM = 1;
    bool puppy = randNormf() > 0.2f;
    if (puppy)
    {
        geomBuffer.shine.z = 1.0f;
        useNM = 0;
    }
    else
    {
        geomBuffer.shine.z = 0.0f;
        useNM = 1;
    }
    geomBuffer.shine.w = *reinterpret_cast<float*>(&useNM);
}


HRESULT CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, const std::vector<std::string>& defines, ID3DBlob** ppCode)
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
    else if (ext == L"cs")
    {
        entryPoint = "cs";
        platform = "cs_5_0";
    }

    UINT flags1 = 0;
#ifdef _DEBUG
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif 

    D3DInclude includeHandler;

    std::vector<D3D_SHADER_MACRO> shaderDefines;
    shaderDefines.resize(defines.size() + 1);
    for (int i = 0; i < defines.size(); i++)
    {
        shaderDefines[i].Name = defines[i].c_str();
        shaderDefines[i].Definition = "";
    }
    shaderDefines.back().Name = nullptr;
    shaderDefines.back().Definition = nullptr;

    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErrMsg = nullptr;
    HRESULT result = D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(), shaderDefines.data(), &includeHandler, entryPoint.c_str(), platform.c_str(), flags1, 0, &pCode, &pErrMsg);
    if (!SUCCEEDED(result) && pErrMsg != nullptr)
    {
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());
    }
    assert(SUCCEEDED(result));
    SAFE_RELEASE(pErrMsg);

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
        else if (ext == L"cs")
        {
            ID3D11ComputeShader* pComputeShader = nullptr;
            result = g_pd3dDevice->CreateComputeShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &pComputeShader);
            if (SUCCEEDED(result))
            {
                *ppShader = pComputeShader;
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


bool Update() 
{
    size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (m_prevUSec == 0)
    {
        m_prevUSec = usec; 
    }

    double deltaSec = (usec - m_prevUSec) / 1000000.0;

    for (UINT i = 0; i < m_instCount; i++)
    {
        if (fabs(m_geomBuffers[i].shine.y) > 0.0001)
        {
            m_geomBuffers[i].posAngle.w = m_geomBuffers[i].posAngle.w + (float)deltaSec * m_geomBuffers[i].shine.y;

            DirectX::XMMATRIX m = DirectX::XMMatrixMultiply(
                DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -(float)m_geomBuffers[i].posAngle.w),
                DirectX::XMMatrixTranslation(m_geomBuffers[i].posAngle.x, m_geomBuffers[i].posAngle.y, m_geomBuffers[i].posAngle.z)
            );

            m_geomBuffers[i].m = m;
            m = DirectX::XMMatrixInverse(nullptr, m);
            m = DirectX::XMMatrixTranspose(m);
            m_geomBuffers[i].normalM = m;
        }
    }

    g_pd3dDeviceContext->UpdateSubresource(m_pGeomBufferInst, 0, nullptr, m_geomBuffers.data(), 0, 0);

    {
        for (int i = 0; i < m_sceneBuffer.lightCount.x; i++)
        {
            RectGeomBuffer geomBuffer;
            geomBuffer.m = DirectX::XMMatrixTranslation(m_sceneBuffer.lights[i].pos.x, m_sceneBuffer.lights[i].pos.y, m_sceneBuffer.lights[i].pos.z);
            geomBuffer.color = m_sceneBuffer.lights[i].color;

            g_pd3dDeviceContext->UpdateSubresource(m_pSmallLightSphereGeomBuffers[i], 0, nullptr, &geomBuffer, 0, 0);
        }
    }

    m_prevUSec = usec;

    DirectX::XMMATRIX v;
    Point4f cameraPos;
    {
        Point3f pos = m_camera.poi + Point3f{ cosf(m_camera.theta) * cosf(m_camera.phi), sinf(m_camera.theta), cosf(m_camera.theta) * sinf(m_camera.phi) } * m_camera.r;
        float upTheta = m_camera.theta + (float)M_PI / 2;
        Point3f up = Point3f{ cosf(upTheta) * cosf(m_camera.phi), sinf(upTheta), cosf(upTheta) * sinf(m_camera.phi) };

        v = DirectX::XMMatrixLookAtLH(
            DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
            DirectX::XMVectorSet(m_camera.poi.x, m_camera.poi.y, m_camera.poi.z, 0.0f),
            DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
        );

        cameraPos = pos;
    }

    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 3;
    float c = 1.0f / tanf(fov / 2);
    float aspectRatio = (float)WindowHeight / WindowWidth;
    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(tanf(fov / 2) * 2 * f, tanf(fov / 2) * 2 * f * aspectRatio, f, n);

    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = g_pd3dDeviceContext->Map(m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result))
    {
        m_sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
        m_sceneBuffer.cameraPos = cameraPos;
        CalcFrustum(m_sceneBuffer.frustum);

        memcpy(subresource.pData, &m_sceneBuffer, sizeof(SceneBuffer));

        g_pd3dDeviceContext->Unmap(m_pSceneBuffer, 0);
    }

    if (m_updateCullParams)
    {
        CullParams cullParams;
        cullParams.shapeCount = m_instCount;
        for (UINT i = 0; i < m_instCount; i++)
        {
            cullParams.bbMin[i] = m_geomBBs[i].vmin;
            cullParams.bbMax[i] = m_geomBBs[i].vmax;
        }

        g_pd3dDeviceContext->UpdateSubresource(m_pCullParams, 0, nullptr, &cullParams, 0, 0);

        m_updateCullParams = false;
    }

    return SUCCEEDED(result);
}


void ReadQueries()
{
    D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
    while (m_lastCompletedFrame < m_curFrame)
    {
        HRESULT result = g_pd3dDeviceContext->GetData(m_queries[m_lastCompletedFrame % 10], &stats, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), 0);
        if (result == S_OK)
        {
            m_gpuVisibleInstances = (int)stats.IAPrimitives / 12;

            ++m_lastCompletedFrame;
        }
        else
        {
            break;
        }
    }
}


void CalcFrustum(Point4f frustum[6])
{
    Point3f dir = -Point3f{ cosf(m_camera.theta) * cosf(m_camera.phi), sinf(m_camera.theta), cosf(m_camera.theta) * sinf(m_camera.phi) };
    float upTheta = m_camera.theta + (float)M_PI / 2;
    Point3f up = Point3f{ cosf(upTheta) * cosf(m_camera.phi), sinf(upTheta), cosf(upTheta) * sinf(m_camera.phi) };
    Point3f right = up.cross(dir);
    Point3f pos = m_camera.poi + Point3f{ cosf(m_camera.theta) * cosf(m_camera.phi), sinf(m_camera.theta), cosf(m_camera.theta) * sinf(m_camera.phi) } *m_camera.r;

    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 3;

    float x = tanf(fov * 0.5f) * n;
    float y = tanf(fov * 0.5f) * n * (float) WindowHeight / WindowWidth;

    Point3f nearVertices[4];
    nearVertices[0] = pos + dir * n - up * y - right * x;
    nearVertices[1] = pos + dir * n - up * y + right * x;
    nearVertices[2] = pos + dir * n + up * y + right * x;
    nearVertices[3] = pos + dir * n + up * y - right * x;

    x = tanf(fov * 0.5f) * f;
    y = tanf(fov * 0.5f) * f * (float) WindowHeight / WindowWidth;

    Point3f farVertices[4];
    farVertices[0] = pos + dir * f - up * y - right * x;
    farVertices[1] = pos + dir * f - up * y + right * x;
    farVertices[2] = pos + dir * f + up * y + right * x;
    farVertices[3] = pos + dir * f + up * y - right * x;

    frustum[0] = BuildPlane(nearVertices[0], nearVertices[1], nearVertices[2], nearVertices[3]);
    frustum[1] = BuildPlane(nearVertices[0], farVertices[0], farVertices[1], nearVertices[1]);
    frustum[2] = BuildPlane(nearVertices[1], farVertices[1], farVertices[2], nearVertices[2]);
    frustum[3] = BuildPlane(nearVertices[2], farVertices[2], farVertices[3], nearVertices[3]);
    frustum[4] = BuildPlane(nearVertices[3], farVertices[3], farVertices[0], nearVertices[0]);
    frustum[5] = BuildPlane(farVertices[1], farVertices[0], farVertices[3], farVertices[2]);
}


void CullBoxes()
{
    if (m_computeCull)
    {
        D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args;
        args.IndexCountPerInstance = 36;
        args.InstanceCount = 0;
        args.StartIndexLocation = 0;
        args.BaseVertexLocation = 0;
        args.StartInstanceLocation = 0;

        g_pd3dDeviceContext->UpdateSubresource(m_pIndirectArgsSrc, 0, nullptr, &args, 0, 0);

        UINT groupNumber = DivUp(m_instCount, 64u);

        ID3D11Buffer* constBuffers[2] = { m_pSceneBuffer, m_pCullParams };
        g_pd3dDeviceContext->CSSetConstantBuffers(0, 2, constBuffers);

        ID3D11UnorderedAccessView* uavBuffers[2] = { m_pIndirectArgsUAV, m_pGeomBufferInstVisGPU_UAV };
        g_pd3dDeviceContext->CSSetUnorderedAccessViews(0, 2, uavBuffers, nullptr);

        g_pd3dDeviceContext->CSSetShader(m_pCullShader, nullptr, 0);

        g_pd3dDeviceContext->Dispatch(groupNumber, 1, 1);

        g_pd3dDeviceContext->CopyResource(m_pGeomBufferInstVis, m_pGeomBufferInstVisGPU);
    }
    else
    {
        Point4f frustum[6];
        CalcFrustum(frustum);

        std::vector<Point4i> ids(MaxInst);

        m_visibleInstances = 0;
        for (UINT i = 0; i < m_instCount; i++)
        {
            if (IsBoxInside(frustum, m_geomBBs[i].vmin, m_geomBBs[i].vmax))
            {
                ids[m_visibleInstances].x = i;
                ++m_visibleInstances;
            }
        }

        D3D11_MAPPED_SUBRESOURCE subresource;
        HRESULT result = g_pd3dDeviceContext->Map(m_pGeomBufferInstVis, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            memcpy(subresource.pData, ids.data(), sizeof(Point4i) * m_visibleInstances);
            g_pd3dDeviceContext->Unmap(m_pGeomBufferInstVis, 0);
        }
    }
}


bool Render()
{
    g_pd3dDeviceContext->ClearState();

    ID3D11RenderTargetView* views[] = { m_pColorBufferRTV };
    g_pd3dDeviceContext->OMSetRenderTargets(1, views, m_pDepthBufferDSV); 

    static const FLOAT BackColor[4] = { 0.4f, 0.2f, 0.5f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(m_pColorBufferRTV, BackColor);
    g_pd3dDeviceContext->ClearDepthStencilView(m_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0); 

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

    g_pd3dDeviceContext->OMSetDepthStencilState(m_pDepthState, 0); 

    g_pd3dDeviceContext->RSSetState(m_pRasterizerState);

    g_pd3dDeviceContext->OMSetBlendState(m_pOpaqueBlendState, nullptr, 0xFFFFFFFF); 

    CullBoxes();

    ID3D11SamplerState* samplers[] = { m_pSampler };
    g_pd3dDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_pTextureView, m_pTextureViewNM }; 
    g_pd3dDeviceContext->PSSetShaderResources(0, 2, resources); 

    g_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 44 };
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, m_pGeomBufferInst, m_pGeomBufferInstVis };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 3, cbuffers);
    g_pd3dDeviceContext->PSSetConstantBuffers(0, 3, cbuffers); 

    g_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    if (m_doCull)
    {
        if (m_computeCull)
        {
            g_pd3dDeviceContext->CopyResource(m_pIndirectArgs, m_pIndirectArgsSrc);
            g_pd3dDeviceContext->Begin(m_queries[m_curFrame % 10]);
            g_pd3dDeviceContext->DrawIndexedInstancedIndirect(m_pIndirectArgs, 0);
            g_pd3dDeviceContext->End(m_queries[m_curFrame % 10]);
            ++m_curFrame;
        }
        else
        {
            g_pd3dDeviceContext->DrawIndexedInstanced(36, m_visibleInstances, 0, 0, 0);
        }
    }
    else
    {
        g_pd3dDeviceContext->DrawIndexedInstanced(36, m_instCount, 0, 0, 0);
    }

    if (m_showLightBulbs)
    {
        RenderSmallLightSpheres();
    }

    RenderSphere();

    RenderRects();

    RenderPostProcess();

    ReadQueries(); 

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Lights");

        ImGui::Checkbox("Show bulbs", &m_showLightBulbs);
        ImGui::Checkbox("Use normal maps", &m_useNormalMaps);
        ImGui::Checkbox("Show normals", &m_showNormals);
        ImGui::Checkbox("Use black & white", &m_useBWFilter);

        m_sceneBuffer.lightCount.y = m_useNormalMaps ? 1 : 0;
        m_sceneBuffer.lightCount.z = m_showNormals ? 1 : 0;
        m_sceneBuffer.lightCount.w = m_doCull ? 1 : 0;

        m_sceneBuffer.postProcess.x = m_useBWFilter ? 1 : 0;

        bool add = ImGui::Button("+");
        ImGui::SameLine();
        bool remove = ImGui::Button("-");

        if (add && m_sceneBuffer.lightCount.x < 10)
        {
            ++m_sceneBuffer.lightCount.x;
            m_sceneBuffer.lights[m_sceneBuffer.lightCount.x - 1] = Light();
        }
        if (remove && m_sceneBuffer.lightCount.x > 0)
        {
            --m_sceneBuffer.lightCount.x;
        }

        char buffer[1024];
        for (int i = 0; i < m_sceneBuffer.lightCount.x; i++)
        {
            ImGui::Text("Light %d", i);
            sprintf_s(buffer, "Pos %d", i);
            ImGui::DragFloat3(buffer, (float*)&m_sceneBuffer.lights[i].pos, 0.1f, -10.0f, 10.0f);
            sprintf_s(buffer, "Color %d", i);
            ImGui::ColorEdit3(buffer, (float*)&m_sceneBuffer.lights[i].color);
        }

        ImGui::End();

        ImGui::Begin("Instances");
        add = ImGui::Button("+");
        ImGui::SameLine();
        remove = ImGui::Button("-");
        ImGui::Text("Count %d", m_instCount);

        if (m_computeCull)
        {
            ImGui::Text("Visible (GPU) %d", m_gpuVisibleInstances);
        }
        else
        {
            ImGui::Text("Visible %d", m_visibleInstances);
        }

        //ImGui::Checkbox("Cull", &m_doCull);
        //ImGui::Checkbox("Cull on GPU", &m_computeCull); 
        ImGui::Checkbox("Cull", &m_doCull);
        if (!m_doCull)
        {
            ImGui::BeginDisabled();
            m_computeCull = false; 
        }
        ImGui::Checkbox("Cull on GPU", &m_computeCull);
        if (!m_doCull)
        {
            ImGui::EndDisabled();
        }

        ImGui::End();
        if (add && m_instCount < MaxInst)
        {
            Point4f pos = m_geomBuffers[m_instCount].posAngle;
            if (pos.x == 0 && pos.y == 0 && pos.z == 0)
            {
                InitGeom(m_geomBuffers[m_instCount], m_geomBBs[m_instCount]);
            }
            ++m_instCount;
        }
        if (remove && m_instCount > 0)
        {
            --m_instCount;
        }
        m_updateCullParams = add || remove;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT result = g_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(result));

    return SUCCEEDED(result);
}


void RenderSphere()
{
    ID3D11SamplerState* samplers[] = { m_pSampler };
    g_pd3dDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_pCubemapView };
    g_pd3dDeviceContext->PSSetShaderResources(0, 1, resources);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pSphereIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pSphereVertexBuffer };
    UINT strides[] = { 12 }; 
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, m_pSphereGeomBuffer };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pSphereInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pSphereVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);

    g_pd3dDeviceContext->PSSetShader(m_pSpherePixelShader, nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(m_sphereIndexCount, 0, 0);
}


void RenderSmallLightSpheres()
{
    g_pd3dDeviceContext->OMSetBlendState(m_pOpaqueBlendState, nullptr, 0xffffffff);
    g_pd3dDeviceContext->OMSetDepthStencilState(m_pDepthState, 0);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pSmallLightSphereIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pSmallLightSphereVertexBuffer };
    UINT strides[] = { 12 };
    UINT offsets[] = { 0 };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pSmallLightSphereInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pSmallLightSphereVertexShader, nullptr, 0);
    g_pd3dDeviceContext->PSSetShader(m_pSmallLightSpherePixelShader, nullptr, 0);

    for (int i = 0; i < m_sceneBuffer.lightCount.x; i++)
    {
        ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, m_pSmallLightSphereGeomBuffers[i] };
        g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->DrawIndexed(m_SmallLightSphereIndexCount, 0, 0);
    }
}


void RenderRects()
{
    g_pd3dDeviceContext->OMSetDepthStencilState(m_pTransDepthState, 0);

    g_pd3dDeviceContext->OMSetBlendState(m_pTransBlendState, nullptr, 0xFFFFFFFF);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pRectIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pRectVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, nullptr };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pRectInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pRectVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
    g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
    g_pd3dDeviceContext->PSSetShader(m_pRectPixelShader, nullptr, 0);

    float d0 = 0.0f, d1 = 0.0f;
    Point3f cameraPos = m_camera.poi + Point3f{ cosf(m_camera.theta) * cosf(m_camera.phi), sinf(m_camera.theta), cosf(m_camera.theta) * sinf(m_camera.phi) } *m_camera.r;
    for (int i = 0; i < 8; i++)
    {
        d0 = std::max(d0, (cameraPos - m_boundingRects[0].GetVert(i)).lengthSqr());
        d1 = std::max(d1, (cameraPos - m_boundingRects[1].GetVert(i)).lengthSqr());
    }

    if (d0 > d1)
    {
        cbuffers[1] = m_pRectGeomBuffer;
        g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->DrawIndexed(6, 0, 0);

        cbuffers[1] = m_pRectGeomBuffer2;
        g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->DrawIndexed(6, 0, 0);
    }
    else
    {
        cbuffers[1] = m_pRectGeomBuffer2;
        g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->DrawIndexed(6, 0, 0);

        cbuffers[1] = m_pRectGeomBuffer;
        g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
        g_pd3dDeviceContext->DrawIndexed(6, 0, 0);
    }
}


void RenderPostProcess()
{
    ID3D11RenderTargetView* views[] = { g_pRenderTargetView };
    g_pd3dDeviceContext->OMSetRenderTargets(1, views, nullptr);

    ID3D11SamplerState* samplers[] = { m_pSampler };
    g_pd3dDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_pColorBufferSRV };
    g_pd3dDeviceContext->PSSetShaderResources(0, 1, resources);

    g_pd3dDeviceContext->OMSetDepthStencilState(nullptr, 0);
    g_pd3dDeviceContext->RSSetState(nullptr);
    g_pd3dDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    g_pd3dDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    g_pd3dDeviceContext->IASetInputLayout(nullptr);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pBWVertexShader, nullptr, 0);
    g_pd3dDeviceContext->PSSetShader(m_pBWPixelShader, nullptr, 0);

    g_pd3dDeviceContext->Draw(3, 0);
}


void ResizeWindow(int width, int height)
{
    if (g_pRenderTargetView)
    {
        g_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        g_pRenderTargetView->Release();
        g_pRenderTargetView = nullptr;
        SAFE_RELEASE(m_pDepthBuffer)
        SAFE_RELEASE(m_pDepthBufferDSV)
    }

    HRESULT result = g_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    assert(SUCCEEDED(result));

    if (SUCCEEDED(result))
    {
        WindowWidth = width;
        WindowHeight = height;

        result = SetupBackBuffer();

        float n = 0.1f;
        float fov = (float)M_PI / 3;
        float halfW = tanf(fov / 2) * n;
        float halfH = (float)WindowHeight / WindowWidth * halfW;

        float r = sqrtf(n * n + halfH * halfH + halfW * halfW) * 1.1f * 2.0f;

        SphereGeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixIdentity();
        geomBuffer.size = r;
        g_pd3dDeviceContext->UpdateSubresource(m_pSphereGeomBuffer, 0, nullptr, &geomBuffer, 0, 0);
    }
}


void Terminate()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    SAFE_RELEASE(m_pSampler);

    SAFE_RELEASE(m_pColorBuffer);
    SAFE_RELEASE(m_pColorBufferRTV);
    SAFE_RELEASE(m_pColorBufferSRV);
    SAFE_RELEASE(m_pBWPixelShader);
    SAFE_RELEASE(m_pBWVertexShader);

    SAFE_RELEASE(m_pTextureView);
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pTextureViewNM);
    SAFE_RELEASE(m_pTextureNM);

    SAFE_RELEASE(m_pRasterizerState);
    SAFE_RELEASE(m_pDepthState);
    SAFE_RELEASE(m_pTransDepthState);

    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pVertexShader);

    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);
   
    SAFE_RELEASE(m_pTransBlendState); 
    SAFE_RELEASE(m_pOpaqueBlendState); 

    SAFE_RELEASE(m_pSceneBuffer);
    SAFE_RELEASE(m_pGeomBufferInst);
    SAFE_RELEASE(m_pGeomBufferInstVis);
    

    TermSphere();

    TermRect();

    TermDepthBuffer();

    TermSmallLightSphere();

    TermGPUCulling();

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

            if (pDebug != nullptr)
            {
                pDebug->Release();
                pDebug = nullptr;
            }
        }
    }
#endif 
    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}


void TermSphere()
{
    SAFE_RELEASE(m_pSphereInputLayout);
    SAFE_RELEASE(m_pSpherePixelShader);
    SAFE_RELEASE(m_pSphereVertexShader);

    SAFE_RELEASE(m_pSphereIndexBuffer);
    SAFE_RELEASE(m_pSphereVertexBuffer);

    SAFE_RELEASE(m_pSphereGeomBuffer);

    SAFE_RELEASE(m_pCubemapTexture);
    SAFE_RELEASE(m_pCubemapView);
}


void TermRect()
{
    SAFE_RELEASE(m_pRectInputLayout);
    SAFE_RELEASE(m_pRectPixelShader);
    SAFE_RELEASE(m_pRectVertexShader);

    SAFE_RELEASE(m_pRectIndexBuffer);
    SAFE_RELEASE(m_pRectVertexBuffer);

    SAFE_RELEASE(m_pRectGeomBuffer);
    SAFE_RELEASE(m_pRectGeomBuffer2);
}


void TermDepthBuffer()
{
    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);
}


void TermSmallLightSphere()
{
    SAFE_RELEASE(m_pSmallLightSphereIndexBuffer);
    SAFE_RELEASE(m_pSmallLightSphereVertexBuffer);
    SAFE_RELEASE(m_pSmallLightSphereInputLayout);
    SAFE_RELEASE(m_pSmallLightSphereVertexShader);
    SAFE_RELEASE(m_pSmallLightSpherePixelShader);
    for (int i = 0; i < 10; i++)
    {
        SAFE_RELEASE(m_pSmallLightSphereGeomBuffers[i]);
    }
}


void TermGPUCulling()
{
    SAFE_RELEASE(m_pCullShader);
    SAFE_RELEASE(m_pIndirectArgsSrc);
    SAFE_RELEASE(m_pIndirectArgs);
    SAFE_RELEASE(m_pCullParams);
    SAFE_RELEASE(m_pIndirectArgsUAV);
    SAFE_RELEASE(m_pGeomBufferInstVisGPU);
    SAFE_RELEASE(m_pGeomBufferInstVisGPU_UAV);
    for (int i = 0; i < 10; i++)
    {
        SAFE_RELEASE(m_queries[i]);
    }
}


