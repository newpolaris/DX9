// https://github.com/bkaradzic/bgfx/blob/master/src/renderer_d3d9.cpp

#include <Windows.h>
#include <d3d9.h>
#include <cmath>
#include <stdio.h>

#pragma comment(lib, "d3d9.lib")

const char* GetErrorCode(HRESULT hr)
{
	switch (hr) {
	case D3DERR_WRONGTEXTUREFORMAT:
		return "WRONGTEXTUREFORMAT";
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		return "UNSUPPORTEDCOLOROPERATION";
	case D3DERR_UNSUPPORTEDCOLORARG:
		return "UNSUPPORTEDCOLORARG";
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		return "UNSUPPORTEDALPHAOPERATION";
	case D3DERR_UNSUPPORTEDALPHAARG:
		return "UNSUPPORTEDALPHAARG";
	case D3DERR_TOOMANYOPERATIONS:
		return "TOOMANYOPERATIONS";
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		return "CONFLICTINGTEXTUREFILTER";
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		return "UNSUPPORTEDFACTORVALUE";
	case D3DERR_CONFLICTINGRENDERSTATE:
		return "CONFLICTINGRENDERSTATE";
	case D3DERR_UNSUPPORTEDTEXTUREFILTER:
		return "UNSUPPORTEDTEXTUREFILTER";
	case D3DERR_CONFLICTINGTEXTUREPALETTE:
		return "CONFLICTINGTEXTUREPALETTE";
	case D3DERR_DRIVERINTERNALERROR:
		return "DRIVERINTERNALERROR";

	case D3DERR_NOTFOUND:
		return "NOTFOUND";
	case D3DERR_MOREDATA:
		return "MOREDATA";
	case D3DERR_DEVICELOST:
		return "DEVICELOST";
	case D3DERR_DEVICENOTRESET:
		return "DEVICENOTRESET";
	case D3DERR_NOTAVAILABLE:
		return "NOTAVAILABLE";
	case D3DERR_OUTOFVIDEOMEMORY:
		return "OUTOFVIDEOMEMORY";
	case D3DERR_INVALIDDEVICE:
		return "INVALIDDEVICE";
	case D3DERR_INVALIDCALL:
		return "INVALIDCALL";
	case D3DERR_DRIVERINVALIDCALL:
		return "DRIVERINVALIDCALL";
	case D3DERR_WASSTILLDRAWING:
		return "WASSTILLDRAWING";
	case D3DOK_NOAUTOGEN:
		return "NOAUTOGEN";
	}
	return "UNKNOWN_ERROR";
}

HRESULT DX_CHECK(HRESULT hr) {
	if (FAILED(hr)) {
		OutputDebugStringA(GetErrorCode(hr));
		OutputDebugStringA("\n");
		DebugBreak();
	}
	return hr;
}

HWND g_hWnd = 0;
IDirect3D9* g_pD3D = NULL; // Used to create the D3DDevice
IDirect3DDevice9* g_pd3dDevice = NULL; // Our rendering device
IDirect3DVertexBuffer9* g_pVB = NULL; // Buffer to hold Vertices
IDirect3DSwapChain9* g_swapChain = NULL;
IDirect3DSurface9* g_backBufferColor = NULL;
IDirect3DSurface9* g_backBufferDepthStencil = NULL;
IDirect3DSurface9* g_msaaColor = NULL;

D3DPRESENT_PARAMETERS g_d3dpp;

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
	FLOAT x, y, z, w; // The transformed position for the vertex
	DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZW|D3DFVF_DIFFUSE)

VOID OnPreReset()
{
	if (g_backBufferColor) {
		g_backBufferColor->Release();
		g_backBufferColor = NULL;
	}
	if (g_backBufferDepthStencil) {
		g_backBufferDepthStencil->Release();
		g_backBufferDepthStencil = NULL;
	}
	if (g_swapChain) {
		g_swapChain->Release();
		g_swapChain = NULL;
	}
	if (g_msaaColor) {
		g_msaaColor->Release();
		g_msaaColor = NULL;
	}
}

VOID OnPostReset()
{
	DX_CHECK(g_pd3dDevice->GetSwapChain(0, &g_swapChain));
	DX_CHECK(g_swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &g_backBufferColor));
	DX_CHECK(g_pd3dDevice->GetDepthStencilSurface(&g_backBufferDepthStencil));

	// Call function 'GetClientRect' while resizing, cause window title bar error
	// RECT rect = { 0, };
	// GetClientRect(g_hWnd, &rect);

	UINT width = g_d3dpp.BackBufferWidth;
	UINT height = g_d3dpp.BackBufferHeight;
	bool lockable = false;

	DX_CHECK(g_pd3dDevice->CreateRenderTarget(
		width, height, D3DFMT_A8R8G8B8,
		D3DMULTISAMPLE_4_SAMPLES, 0, lockable,
		&g_msaaColor, NULL));
}

VOID Cleanup()
{
	OnPreReset();

	if (g_pVB != NULL) {
		g_pVB->Release();
		g_pVB = NULL;
	}
	if (g_pd3dDevice != NULL) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
	if (g_pD3D != NULL) {
		g_pD3D->Release();
		g_pD3D = NULL;
	}
}

// https://github.com/bkaradzic/bgfx/blob/master/src/renderer_d3d9.cpp
// Introduction to Directx 9.0c 's sample

VOID Resize(WPARAM wParam, LPARAM lParam)
{
	if (g_pd3dDevice == nullptr)
		return;

	D3DDEVICE_CREATION_PARAMETERS dcp;
	DX_CHECK(g_pd3dDevice->GetCreationParameters(&dcp));

	D3DDISPLAYMODE dm;
	DX_CHECK(g_pD3D->GetAdapterDisplayMode(dcp.AdapterOrdinal, &dm));

	g_d3dpp.BackBufferWidth = LOWORD(lParam);
	g_d3dpp.BackBufferHeight = HIWORD(lParam);

	// device reset error when pixel area is zero
	if (g_d3dpp.BackBufferWidth == 0 || g_d3dpp.BackBufferHeight == 0)
		return;

	OnPreReset();
	DX_CHECK(g_pd3dDevice->Reset(&g_d3dpp));
	OnPostReset();
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT InitD3D(HWND hWnd)
{
	// Create the D3D object.
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/full-scene-antialiasing
	/*
	 * The code below assumes that pD3D is a valid pointer
	 *   to a IDirect3D9 interface.
	 */
	DWORD quality;
	if (FAILED(g_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, TRUE,
		D3DMULTISAMPLE_4_SAMPLES, &quality)))
		return E_FAIL;

	// Set up the structure used to create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // required. to enable multisampling
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

	// Create the D3DDevice
	if (FAILED(g_pD3D->CreateDevice(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING |
		D3DCREATE_PUREDEVICE | D3DCREATE_MULTITHREADED,
		&g_d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}

	// WM_PAINT가 Resize 보다 먼저 호출되므로,
	OnPostReset();

	return S_OK;
}

HRESULT InitVB()
{
	/*
	 * to create vertices that is in NDC space,
	 * declare vertex type as D3DFVF_XYZW instead of D3DFVF_XYZRHW
	 */
	 // Initialize three Vertices for rendering a triangle
	CUSTOMVERTEX Vertices[] =
	{
		{  0.0f,  1.0f, 0.5f, 1.0f, 0xffff0000, }, // x, y, z, w, color
		{  1.0f, -1.0f, 0.5f, 1.0f, 0xff00ff00, },
		{ -1.0f, -1.0f, 0.5f, 1.0f, 0xff00ffff, },
	};

	// Create the vertex buffer. Here we are allocating enough memory
	// (from the default pool) to hold all our 3 custom Vertices. We also
	// specify the FVF, so the vertex buffer knows what data it contains.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_MANAGED, &g_pVB, NULL)))
	{
		return E_FAIL;
	}

	// Now we fill the vertex buffer. To do this, we need to Lock() the VB to
	// gain access to the Vertices. This mechanism is required becuase vertex
	// buffers may be in device memory.
	VOID* pVertices;
	if (FAILED(g_pVB->Lock(0, sizeof(Vertices), (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, Vertices, sizeof(Vertices));
	g_pVB->Unlock();

	return S_OK;
}

bool isLost(HRESULT hr)
{
	return false
		|| hr == D3DERR_DEVICELOST
		|| hr == D3DERR_DRIVERINTERNALERROR;
}

VOID Render()
{
	g_pd3dDevice->SetRenderTarget(0, g_msaaColor);

	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0xFF), 1.f, 0);
	g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);

	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// Draw the triangles in the vertex buffer. This is broken into a few
		// steps. We are passing the Vertices down a "stream", so first we need
		// to specify the source of that stream, which is our vertex buffer. Then
		// we need to let D3D know what vertex shader to use. Full, custom vertex
		// shaders are an advanced topic, but in most cases the vertex shader is
		// just the FVF, so that D3D knows what type of Vertices we are dealing
		// with. Finally, we call DrawPrimitive() which does the actual rendering
		// of our geometry (in this case, just one triangle).
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
		g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

		// End the scene
		g_pd3dDevice->EndScene();
	}

	// just set back to screen surface
	g_pd3dDevice->SetRenderTarget(0, g_backBufferColor);

	// copy rendertarget to backbuffer
	g_pd3dDevice->StretchRect(g_msaaColor, NULL, g_backBufferColor, NULL, D3DTEXF_NONE);

	// reutrns only D3D_OK or D3DERR_DEVICEREMOVED ??
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	
	// https://github.com/bkaradzic/bgfx/blob/master/src/renderer_d3d9.cpp
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-present
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-testcooperativelevel
	HRESULT hr = S_OK;
	do
	{
		do
		{
			hr = g_pd3dDevice->TestCooperativeLevel();
		}
		while (D3DERR_DEVICENOTRESET != hr);

		OnPreReset();
		DX_CHECK(g_pd3dDevice->Reset(&g_d3dpp));
		OnPostReset();			
		hr = g_pd3dDevice->TestCooperativeLevel();
	}
	while (FAILED(hr));

}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
		Render();
		break;

	case WM_SIZE:
		Resize(wParam, lParam);
		return 0;
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main()
{
	// Register the window class
	WNDCLASSEX wc =
	{
		// https://stackoverflow.com/a/32806642/1890382
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_OWNDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		L"D3D Tutorial", NULL
	};
	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow(L"D3D Tutorial", L"D3D Tutorial 02: Vertices",
		WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
		NULL, NULL, wc.hInstance, NULL);

	g_hWnd = hWnd;

	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// Create the vertex buffer
		if (SUCCEEDED(InitVB()))
		{
			// Show the window
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			// Enter the message loop
			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else {
					Render();
				}
			}
		}
	}

	UnregisterClass(L"D3D Tutorial", wc.hInstance);

	return 0;
}
