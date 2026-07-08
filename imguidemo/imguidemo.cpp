#include "imgui.h"
#include "imguiSDK.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <Astral.h>
#include <IconsFontAwesome6.h>
#include <fa-solid-900.h>
#include <gdiplus.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#pragma comment(lib, "gdiplus.lib")
//icon
static ID3D11ShaderResourceView* s_icon = nullptr;
static ImFont* g_FontLight = nullptr;

void MenuTab1();
void MenuTab2();
void MenuTab3();
ID3D11ShaderResourceView* CreateTextureFromRGBA(
    ID3D11Device* device,
    const unsigned char* data,
    int width,
    int height,
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
{
    if (!device || !data || width <= 0 || height <= 0)
        return nullptr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format; // 使用传入的格式
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = width * 4; // 仍为4字节

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &pTexture);
    if (FAILED(hr) || !pTexture)
        return nullptr;

    ID3D11ShaderResourceView* pSRV = nullptr;
    hr = device->CreateShaderResourceView(pTexture, nullptr, &pSRV);
    pTexture->Release();

    if (FAILED(hr))
        return nullptr;

    return pSRV;
}

void Menu();

namespace module
{
    inline bool InfiniteMp = false;
    inline bool InfiniteJump = false;
    inline bool FullLight = false;
    inline bool SpeedRun = false;

    inline float SpeedRunValue = 2.5f;
}

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ImFont* g_codeFont = nullptr;
static ID3D11ShaderResourceView* g_bgTexture = nullptr;
static int g_bgWidth = 0;
static int g_bgHeight = 0;
static ULONG_PTR g_gdiplusToken = 0;

static bool LoadTextureFromFileGDIPlus(ID3D11Device* device, const wchar_t* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    *out_srv = nullptr;
    *out_width = *out_height = 0;

    using namespace Gdiplus;
    Bitmap* bmp = Bitmap::FromFile(filename);
    if (!bmp)
        return false;
    Status st = bmp->GetLastStatus();
    if (st != Ok)
    {
        delete bmp;
        return false;
    }

    int width = bmp->GetWidth();
    int height = bmp->GetHeight();
    if (width == 0 || height == 0)
    {
        delete bmp;
        return false;
    }

    BitmapData data;
    Rect rc(0, 0, width, height);
    if (bmp->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &data) != Ok)
    {
        delete bmp;
        return false;
    }

    // Create texture with B8G8R8A8 format which matches GDI+ PixelFormat32bppARGB memory layout
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = data.Scan0;
    initData.SysMemPitch = data.Stride;
    initData.SysMemSlicePitch = 0;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &pTexture);

    bmp->UnlockBits(&data);
    delete bmp;

    if (FAILED(hr) || !pTexture)
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();
    if (FAILED(hr))
        return false;

    *out_width = width;
    *out_height = height;
    return true;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};

    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;

    const D3D_FEATURE_LEVEL featureLevelArray[2] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    return D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) == S_OK;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;

    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();

    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }

    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }

    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SIZE:
    {
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();

            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

            CreateRenderTarget();
        }

        return 0;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc =
    {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        _T("imgui"),
        nullptr
    };

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, _T("Astral"), WS_OVERLAPPEDWINDOW, 100, 100, 1000, 650, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        return 1;
    }

    CreateRenderTarget();

    // 初始化 GDI+ (用于加载图片)
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    {
        const wchar_t* bgPath = L"D:\\2222\\old\\临时\\demo1.png";
        LoadTextureFromFileGDIPlus(g_pd3dDevice, bgPath, &g_bgTexture, &g_bgWidth, &g_bgHeight);
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = {
    0x0020, 0x00FF,
    0x2000, 0x206F,
    0x3000, 0x30FF,
    0x4E00, 0x9FFF,
    0xFF08, 0xFF09,
    0
    };

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, nullptr, ranges);
 
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.FontDataOwnedByAtlas = false;

    static const ImWchar icons_ranges[] =
    {
        0xf000, 0xf8ff,
        0
    };

    io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 18.0f, &config, icons_ranges);

    g_FontLight = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyhl.ttc", 15.0f, nullptr, ranges);

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 15.0f;
    style.FrameRounding = 10.0f;
    style.ChildRounding = 10.0f;
    style.LogSliderDeadzone = 0.0f;
    style.GrabMinSize = 14.0f;
    style.GrabRounding = 999.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBarBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;

    style.WindowPadding = ImVec2(13.f, 13.f);
    style.CellPadding = ImVec2(6.f, 6.f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078431f, 0.078431f, 0.082353f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.094118f, 0.094118f, 0.101961f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);

    // 标题栏
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.10f, 0.65f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09f, 0.09f, 0.10f, 0.65f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.09f, 0.10f, 0.65f);

    // Frame 
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.23f, 1.00f); // #C1DD54

    // 按钮
    style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5941f, 0.5392f, 0.8490f, 1.0f);

    // Header
    style.Colors[ImGuiCol_Header] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.87f, 0.33f, 1.00f);

    // 滑块
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.76f, 0.87f, 0.33f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.82f, 0.93f, 0.38f, 1.00f);

    // 勾选框
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.6941f, 0.6392f, 0.9490f, 1.0f);

    // 分隔线
    style.Colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.76f, 0.87f, 0.33f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.82f, 0.93f, 0.38f, 1.00f);

    // 边框
    style.Colors[ImGuiCol_Border] = ImVec4(0.16f, 0.22f, 0.23f, 0.0f);

    // 文本
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.91f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.f, 1.f, 1.f, 1.00f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    MSG msg{};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        auto bgDraw = ImGui::GetBackgroundDrawList();
        auto fgDraw = ImGui::GetForegroundDrawList();

        if (g_bgTexture && g_bgWidth > 0 && g_bgHeight > 0)
        {
            ImVec2 disp = ImGui::GetIO().DisplaySize;
            // 目标高度等于窗口高度，计算缩放因子
            float scale = disp.y / (float)g_bgHeight;
            float dst_w = g_bgWidth * scale;
            float dst_h = disp.y; // = g_bgHeight * scale
            float dst_x = 0.0f;
            if (dst_w < disp.x)
                dst_x = (disp.x - dst_w) * 0.5f;

            ImVec2 p0 = ImVec2(dst_x, 0.0f);
            ImVec2 p1 = ImVec2(dst_x + dst_w, dst_h);
            bgDraw->AddImage((ImTextureID)g_bgTexture, p0, p1);
        }

        ImVec2 WMPos = { 10,10 };
        ImGui::DrawRectShadow(bgDraw, ImVec2(WMPos.x - 3, WMPos.y - 3), ImVec2(WMPos.x + 136, WMPos.y + 26), 8.0f, IM_COL32(0, 0, 0, 180), 8.0f, 12, 0.2f);
        if (s_icon)
        {
            float size = 130.0f;
            float aspect = (float)ASTRAL_HEIGHT / (float)ASTRAL_WIDTH;
            float w = size;
            float h = size * aspect;

            ImVec2 iconpos(WMPos.x, WMPos.y);
            ImVec2 p1(iconpos.x + w, iconpos.y + h);

            bgDraw->AddImage((ImTextureID)s_icon, iconpos, p1);
        }

        static bool s_loaded = false;

        if (!s_loaded)// loadImage
        {
            s_icon = CreateTextureFromRGBA(g_pd3dDevice, AstralIcon, ASTRAL_WIDTH, ASTRAL_HEIGHT, DXGI_FORMAT_B8G8R8A8_UNORM);
            if (!s_icon)
            {
                s_icon = CreateTextureFromRGBA(g_pd3dDevice, AstralIcon, ASTRAL_WIDTH, ASTRAL_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
            }
            s_loaded = true;
        }

        Menu();

        ImGui::Render();

        const float clearColor[4] = { 0.03f,0.03f,0.03f,1.00f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    CleanupDeviceD3D();

    DestroyWindow(hwnd);

    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
int windowWidth = 1000, windowHeight = 750;
int tab = 0;
void Menu() {
    ImVec2 WindowSize = ImVec2(windowWidth, windowHeight);
    ImGui::SetNextWindowSize(WindowSize);
    ImGui::Begin(u8"Astral", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImVec2 pos = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
    ImVec2 size = ImVec2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

    draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x , pos.y + size.y), ImGui::GetColorU32(style.Colors[ImGuiCol_WindowBg]), style.WindowRounding, ImDrawFlags_RoundCornersAll);

    int TabButtonWidth = 170;
    draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + TabButtonWidth + 20 + 26, pos.y + size.y), ImGui::GetColorU32(ImVec4(0.103725f, 0.107647f, 0.115490f, 1.0f)), style.WindowRounding, ImDrawFlags_RoundCornersAll ^ ImDrawFlags_RoundCornersRight);

    ImGui::BeginChild("LeftMain", ImVec2(TabButtonWidth + 20, 0), false, ImGuiWindowFlags_NoBackground);

    ImGui::SetCursorPosY(0);
    ImGui::SetCursorPosX(0);

    ImGui::BeginChild("icon", ImVec2(TabButtonWidth + 20, 70), false, ImGuiWindowFlags_NoBackground);

    ImGui::SetCursorPosY(10);
    ImGui::SetCursorPosX(10);

    if (s_icon)
    {
        float size = 165.0f;                     // 期望宽度
        float aspect = (float)ASTRAL_HEIGHT / (float)ASTRAL_WIDTH;
        float w = size;
        float h = size * aspect;                // 保持比例

        ImVec2 iconpos(pos.x + 26, pos.y + 37);               // 左上角
        ImVec2 p1(iconpos.x + w, iconpos.y + h);        // 右下角，用加法但写清楚

        ImGui::GetForegroundDrawList()->AddImage((ImTextureID)s_icon, iconpos, p1);
    }

    ImGui::EndChild();

    ImGui::BeginChild("tabs", ImVec2(TabButtonWidth + 20, 0), false, ImGuiWindowFlags_NoBackground);

    ImGui::SetCursorPosY(10);

    std::vector<const char*> TabIcons =
    {
        ICON_FA_CROSSHAIRS,
        ICON_FA_PERSON_RUNNING,
        ICON_FA_GEAR
    };

    std::vector<const char*> TabNames =
    {
        u8"战斗",
        u8"移动",
        u8"杂项"
    };

    for (int i = 0; i < (int)TabNames.size(); i++)
    {
        ImGui::Spacing();
        ImGui::SetCursorPosX(10);

        if (ImGui::TabButton(TabIcons[i], TabNames[i], tab == i, ImVec2(TabButtonWidth, 30))) {
            tab = i;
        }
    }
    ImGui::Spacing();
    ImGui::SetCursorPosX(12);
    ImGui::ASeparator();
    ImGui::Spacing();

    ImGui::EndChild();
    ImGui::EndChild();

    static int LastTab = -1;
    static float TabAlpha = 1.0f;
    static float TabTimer = 0.0f;

    float Delta = ImGui::GetIO().DeltaTime;

    if (tab != LastTab)
    {
        LastTab = tab;
        TabAlpha = 0.0f;
        TabTimer = 0.0f;
    }

    if (TabAlpha < 1.0f)
    {
        TabTimer += Delta;
        TabAlpha = ImClamp(TabTimer / 0.25f, 0.0f, 1.0f);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(TabButtonWidth + 20 + 39);
    ImGui::BeginChild("RightMain", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground);
    ImVec4 textcol = style.Colors[ImGuiCol_Text];
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(textcol.x, textcol.y, textcol.z, TabAlpha));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, TabAlpha);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 15.f));

    if (tab == 0)
        MenuTab1();
    else if (tab == 1)
        MenuTab2();
    else if (tab == 2)
        MenuTab3();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::EndChild();

    ImGui::End();
}
void MenuTab1() {
    if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_None))
    {
        ImGui::TableNextColumn();
        ImGui::ABeginChild(u8"属性", ImVec2(0, 0));
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::Text(u8"你好 测试 112233");
        static int tempW = 1000;

        if (ImGui::ASliderInt("X", &tempW, 10, 1920, ImGui::GetWindowSize().x - (style.WindowPadding.x * 2))) {
            windowWidth = tempW;
        }

        ImGui::ASliderInt("Y", &windowHeight, 10, 1080, ImGui::GetWindowSize().x - (style.WindowPadding.x * 2));
        static int index = 0;
        if (ImGui::AButton(u8"我是按钮", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 35))) {
            index++;
        }

        ImGui::Text(u8"我是文字 %d", index);
        static bool a1 = true, a2 = true;
        ImGui::ASeparator();
        ImGui::TextSwitch(u8"我是开关", &a1);

        ImGui::TipText(u8"这个开关的颜色是#B1A3F2");

        ImGui::ASeparator();
        ImGui::TextSwitch(u8"我是开关2", &a2);
        ImGui::ASeparator();
        ImGui::ABeginChildEnd();
        ImGui::ABeginChild(u8"角色", ImVec2(0, 0));

        ImGui::AButton(u8"我是按钮##1", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 35));
        ImGui::AButton(u8"我是按钮##2", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 35));
        ImGui::AButton(u8"我是按钮##3", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 35));
        ImGui::ABeginChildEnd();

        ImGui::TableNextColumn();//------
        ImGui::ABeginChild(u8"状态", ImVec2(0, 0));
        static bool setting_open = false;
        static bool setting_open2 = false;


        ImGuiID id1 = ImGui::ASettingButton(
            "setting_button_1",
            ICON_FA_GEAR,
            &setting_open
        );


        if (ImGui::ASettingBegin(id1, ImVec2(200, 200)))
        {
            ImGui::Text("Setting 1");
            ImGui::ASettingEnd();
        }



        ImGuiID id2 = ImGui::ASettingButton(
            "setting_button_2",
            ICON_FA_GEAR,
            &setting_open2
        );


        if (ImGui::ASettingBegin(id2, ImVec2(200, 200)))
        {
            ImGui::Text("Setting 2");
            ImGui::ASettingEnd();
        }

        ImGui::ABeginChildEnd();
        ImGui::EndTable();
    }
}
void MenuTab2() {
    if (ImGui::BeginTable("table2", 2, ImGuiTableFlags_None))
    {
        ImGui::TableNextColumn();
        ImGui::ABeginChild(u8"属性##2", ImVec2(0, 0));
        {
            ImGuiStyle& style = ImGui::GetStyle();

            ImGui::Button(u8"获取血量", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 40));
            ImGui::Button(u8"应用设置", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 40));
            ImGui::Button(u8"退出程序", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 40));
        }
        ImGui::ABeginChildEnd();
        ImGui::ABeginChild(u8"角色##2", ImVec2(0, 0));

        ImGui::ABeginChildEnd();

        ImGui::TableNextColumn();//------
        ImGui::ABeginChild(u8"状态##2", ImVec2(0, 0));

        ImGui::ABeginChildEnd();
        ImGui::EndTable();
    }
}
void MenuTab3() {
    if (ImGui::BeginTable("table3", 2, ImGuiTableFlags_None))
    {
        ImGui::TableNextColumn();
        ImGui::ABeginChild(u8"属性##3", ImVec2(0, 0));
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::Text(u8"你好 测试 112233");
        ImGui::Button(u8"你好 测试 112233");
        ImGui::Text(u8"你好 测试 112233");
        ImGui::Text(u8"你好 测试 112233");

        ImGui::Button(u8"你好 测试 112233", ImVec2(ImGui::GetWindowSize().x - (style.WindowPadding.x * 2), 40));

        ImGui::Text(u8"你好 测试 112233");
        static bool a1 = true;
        ImGui::Switch(u8"我是开关", &a1);

        ImGui::ABeginChildEnd();
        ImGui::ABeginChild(u8"角色##3", ImVec2(0, 0));

        ImGui::ABeginChildEnd();

        ImGui::TableNextColumn();//------
        ImGui::ABeginChild(u8"状态##3", ImVec2(0, 0));

        ImGui::ABeginChildEnd();
        ImGui::EndTable();
    }
}