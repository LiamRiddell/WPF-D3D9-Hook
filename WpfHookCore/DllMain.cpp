// --------------------------------------------------------#
//	Includes
// --------------------------------------------------------#
#include "DllMain.hpp"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx9.h"
#include "imgui\imgui_impl_win32.h"

IDirect3DDevice9* g_pDevice;

// --------------------------------------------------------#
//	ImGui
// --------------------------------------------------------#
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC g_oWndProc = NULL;

bool isShuttingDown = false;
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT CALLBACK hWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam) == 1)
	{
		return true;
	}

	return CallWindowProc(g_oWndProc, hwnd, uMsg, wParam, lParam);
}

// --------------------------------------------------------#
//	DirectX
// --------------------------------------------------------#
bool g_imguiSetup = false;

// IDirect9Device::Reset()
typedef HRESULT (WINAPI* tReset)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* params);
tReset oReset;

// IDirect9Device::EndScene()
typedef HRESULT (WINAPI* tEndScene) (IDirect3DDevice9* pDevice);
tEndScene oEndScene;

HRESULT WINAPI hkEndScene(IDirect3DDevice9* pDevice) {
	
	if (!g_imguiSetup || isShuttingDown)
		return oEndScene(pDevice);

	// Update Window Size
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	D3DDEVICE_CREATION_PARAMETERS cparams;
	RECT rect;
	g_pDevice->GetCreationParameters(&cparams);
	GetWindowRect(cparams.hFocusWindow, &rect);
	io.DisplaySize.x = rect.right;
	io.DisplaySize.y = rect.bottom;

	// Begin Rendering
	IDirect3DStateBlock9* state{};
	pDevice->CreateStateBlock(D3DSBT_ALL, &state);
	state->Capture();
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	state->Apply();
	
	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	pDevice->EndStateBlock(&state);
	state->Release();
	state = nullptr;

	return oEndScene(pDevice);
}

// --------------------------------------------------------#
//	Pattern Scanner @Credits to owner
// --------------------------------------------------------#
bool INT_ComparePattern(char* szSource, const char* szPattern, const char* szMask)
{
	for (; *szMask; ++szSource, ++szPattern, ++szMask)
		if (*szMask == 'x' && *szSource != *szPattern)
			return false;
	return true;
}
char* INT_PatternScan(char* pData, UINT_PTR RegionSize, const char* szPattern, const char* szMask, int Len)
{
	for (UINT i = 0; i != RegionSize - Len; ++i, ++pData)
		if (INT_ComparePattern(pData, szPattern, szMask))
			return pData;
	return nullptr;
}
char* PatternScan(char* pStart, UINT_PTR RegionSize, const char* szPattern, const char* szMask)
{
	char* pCurrent = pStart;
	auto Len = lstrlenA(szMask);

	while (pCurrent <= pStart + RegionSize - Len)
	{
		MEMORY_BASIC_INFORMATION MBI{ 0 };
		if (!VirtualQuery(pCurrent, &MBI, sizeof(MEMORY_BASIC_INFORMATION)))
			return nullptr;

		if (MBI.State == MEM_COMMIT && !(MBI.Protect & PAGE_NOACCESS))
		{
			if (pCurrent + MBI.RegionSize > pStart + RegionSize - Len)
				MBI.RegionSize = pStart + RegionSize - pCurrent + Len;

			char* Ret = INT_PatternScan(pCurrent, MBI.RegionSize, szPattern, szMask, Len);

			if (Ret)
				return Ret;
		}
		pCurrent += MBI.RegionSize;
	}

	return nullptr;
}

// --------------------------------------------------------#
// WPF
// --------------------------------------------------------#
class CD3DDeviceManager
{
public:
	char pad_0000[112]; //0x0000
	class CD3DDeviceLevel1List* pD3DDeviceLevelList;
};
class CD3DDeviceLevel1List
{
public:
	class CD3DDeviceLevel1* pD3DDeviceLevel1_1; //0x0000
	char pad_0008[48]; //0x0008
	class CD3DDeviceLevel1* pD3DDeviceLevel1_2; //0x0038
};
class CD3DDeviceLevel1
{
public:
	char pad_0000[1232]; //0x0000
	class CD3D* pD3D; //0x04D0
};
class CD3D
{
public:
	IDirect3DDevice9* pDevice;
}; 

// CD3DeviceManager::Get()
typedef CD3DDeviceManager* (__fastcall tCD3DDeviceManager_Get)();

// CD3DeviceManager::GetSWDevice()
typedef __int64 (__fastcall tCD3DDeviceManager_GetSWDevice)(CD3DDeviceLevel1** ppDevice);

typedef void(__fastcall tCD3DDeviceLevel1_GetUnderlyingDevice)(CD3DDeviceLevel1* pCD3DDeviceLevel1, IDirect3DDevice9** ppIDirect3DDevice9);

void HookWpfGfx() 
{
	// Get the current process
	MODULEINFO modInfo;
	HANDLE hProcess = GetCurrentProcess();

	// Get a handle to the wpf graphics module
	HMODULE hModule = GetModuleHandle("wpfgfx_cor3.dll");

	if (!hModule)
	{
		return;
	}

	// Get the module size
	if (!GetModuleInformation(hProcess, hModule, &modInfo, sizeof(MODULEINFO)))
	{
		return;
	}

	// CD3DDeviceManager::Get() function pointer via pattern scan
	tCD3DDeviceManager_Get* DeviceManager_Get = (tCD3DDeviceManager_Get*)PatternScan((char*)modInfo.lpBaseOfDll, (UINT_PTR)modInfo.SizeOfImage, "\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x83\xC4\x28", "xxxxx????xxx????xxxx");

	if (!DeviceManager_Get)
	{
		return;
	}

	// Get the Device Manager
	CD3DDeviceManager* pDeviceManager = DeviceManager_Get();

	// Get the IDirect9Device
	auto D3DDeviceLevel1_GetUnderlyingDevice = (tCD3DDeviceLevel1_GetUnderlyingDevice*)PatternScan((char*)modInfo.lpBaseOfDll, (UINT_PTR)modInfo.SizeOfImage, "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x40\x48\x8B\x05\x00\x00\x00\x00\x48\x33\xC4\x48\x89\x44\x24\x00\x48\x8B\xDA\x0F\x57\xC0", "xxxx?xxxxxxxx????xxxxxxx?xxxxxx");
	
	IDirect3DDevice9* pDevice;
	D3DDeviceLevel1_GetUnderlyingDevice(pDeviceManager->pD3DDeviceLevelList->pD3DDeviceLevel1_1, &pDevice);

	if (!pDevice) {
		return;
	}

	void* pEndScene = GetVFunc<void*>(pDevice, 42);

	if (MH_CreateHook(pEndScene, &hkEndScene, reinterpret_cast<LPVOID*>(&oEndScene)) != MH_OK) {
		return;
	}

	if (MH_EnableHook(pEndScene) != MH_OK) {
		return;
	}

	g_pDevice = pDevice;
}

// --------------------------------------------------------#
// Thread
// --------------------------------------------------------#
void MainThread(HMODULE hModule)
{
	MH_Initialize();

	HookWpfGfx();

	// Get Window Size
	D3DDEVICE_CREATION_PARAMETERS cparams;
	RECT rect;
	g_pDevice->GetCreationParameters(&cparams);
	GetWindowRect(cparams.hFocusWindow, &rect);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.DisplaySize.x = rect.right;
	io.DisplaySize.y = rect.bottom;
	ImGui::StyleColorsDark();

	ImGui_ImplDX9_Init(g_pDevice);
	ImGui_ImplWin32_Init(cparams.hFocusWindow);

	g_oWndProc = (WNDPROC)SetWindowLongPtr(cparams.hFocusWindow, GWLP_WNDPROC, (LONG_PTR)hWndProc);

	g_imguiSetup = true;

	#pragma region Shutdown
	while (!GetAsyncKeyState(VK_F9) & 1)
	{
		Sleep(100);
	}

	isShuttingDown = true;
	SetWindowLong(cparams.hFocusWindow, GWLP_WNDPROC, (LONG)g_oWndProc);
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	MH_Uninitialize();

	FreeLibraryAndExitThread(hModule, 0);
	Sleep(100);
	#pragma endregion
}

// --------------------------------------------------------#
//	Entry Point
// --------------------------------------------------------#
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hinstDLL, 0, 0);
			break;
	}
	return true;
}