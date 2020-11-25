// https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop.txt

// https://github.com/markkilgard/NVprSDK/blob/master/nvpr_examples/nvpr_dx9/nvpr_dx9.cpp
// https://github.com/tliron/opengl-3d-vision-bridge/blob/master/opengl_3dv.c

#include <Windows.h>
#include <d3d9.h>
#include <cmath>
#include <stdio.h>
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <cassert>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "opengl32.lib")

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
IDirect3D9Ex* g_pD3D = NULL; // Used to create the D3DDevice
IDirect3DDevice9Ex* g_pd3dDevice = NULL; // Our rendering device
IDirect3DVertexBuffer9* g_pVB = NULL; // Buffer to hold Vertices
IDirect3DSwapChain9* g_swapChain = NULL;
IDirect3DSurface9* g_backBufferColor = NULL;
IDirect3DSurface9* g_backBufferDepthStencil = NULL;

// create the Direct3D render targets
IDirect3DSurface9* dxColorBuffer = NULL;
IDirect3DSurface9* dxDepthBuffer = NULL;

HANDLE gl_handleD3D = 0;
HANDLE gl_handles[2] = { 0, 0 };
GLuint gl_names[2] = { 0, 0 };
GLuint gl_fbo = 0;

static HWND tempHwnd;
static HDC tempdc;
static HGLRC temprc;

D3DPRESENT_PARAMETERS g_d3dpp;

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
	FLOAT x, y, z, w; // The transformed position for the vertex
	DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZW|D3DFVF_DIFFUSE)

bool minOrMaxed = false;

VOID OnLostDevice()
{
	if (gl_handles[0] != 0) {
		wglDXUnregisterObjectNV(gl_handleD3D, gl_handles[0]);
		wglDXUnregisterObjectNV(gl_handleD3D, gl_handles[1]);
		gl_handles[0] = 0;
		gl_handles[1] = 0;
	}

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
	if (dxColorBuffer) {
		dxColorBuffer->Release();
		dxColorBuffer = NULL;
	}
	if (dxDepthBuffer) {
		dxDepthBuffer->Release();
		dxDepthBuffer = NULL;
	}
}

VOID OnResetDevice()
{
	DX_CHECK(g_pd3dDevice->GetSwapChain(0, &g_swapChain));
	DX_CHECK(g_swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &g_backBufferColor));
	DX_CHECK(g_pd3dDevice->GetDepthStencilSurface(&g_backBufferDepthStencil));

	// Call function 'GetClientRect' while resizing, cause window title bar error
	// RECT rect = { 0, };
	// GetClientRect(g_hWnd, &rect);

	UINT width = g_d3dpp.BackBufferWidth;
	UINT height = g_d3dpp.BackBufferHeight;
	
	/*
	 * Must not be lockable, to support interop
	 */
	bool lockable = false;

	DX_CHECK(g_pd3dDevice->CreateRenderTarget(
		width, height, D3DFMT_A8R8G8B8,
		D3DMULTISAMPLE_4_SAMPLES, 0, lockable,
		&dxColorBuffer, NULL));

	DX_CHECK(g_pd3dDevice->CreateDepthStencilSurface(
		width, height, D3DFMT_D24S8,
		D3DMULTISAMPLE_4_SAMPLES, 0, lockable,
		&dxDepthBuffer, NULL));

	gl_handles[0] = wglDXRegisterObjectNV(gl_handleD3D, dxColorBuffer,
		gl_names[0], GL_TEXTURE_2D_MULTISAMPLE,
		WGL_ACCESS_READ_WRITE_NV);
	assert(gl_handles[0] != 0);

	gl_handles[1] = wglDXRegisterObjectNV(gl_handleD3D, dxDepthBuffer,
		gl_names[1], GL_TEXTURE_2D_MULTISAMPLE,
		WGL_ACCESS_READ_WRITE_NV);
	assert(gl_handles[1] != 0);

	glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_MULTISAMPLE, gl_names[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D_MULTISAMPLE, gl_names[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D_MULTISAMPLE, gl_names[1], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

VOID Cleanup()
{
	OnLostDevice();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(GL_FRAMEBUFFER, &gl_fbo);
	glDeleteTextures(2, gl_names);

    wglDXCloseDeviceNV(gl_handleD3D);

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

	if (wParam == SIZE_MINIMIZED)
	{
		minOrMaxed = true;
	}
	else if (wParam == SIZE_MAXIMIZED)
	{
		minOrMaxed = true;
		OnLostDevice();
		DX_CHECK(g_pd3dDevice->Reset(&g_d3dpp));
		OnResetDevice();
	}
	else if (wParam == SIZE_RESTORED)
	{
		/*
		 * https://is03.tistory.com/44
		 * 
		 * to resize screen, reset is required.
		 * and release textures and DEFAULT_POOL type buffer (vertex/index)
		 * see implementations in bgfx
		 */
		OnLostDevice();
		DX_CHECK(g_pd3dDevice->Reset(&g_d3dpp));
		OnResetDevice();
	}

    glViewport(0, 0, g_d3dpp.BackBufferWidth, g_d3dpp.BackBufferHeight);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

static void GLCreate()
{
	// GL context on temporary window, no drawing will happen to this window
	{
		tempHwnd = CreateWindowA("STATIC", "temp", 0, 0, 0, 0, 0, 0, 0, 0, 0);
		assert(tempHwnd);

		tempdc = GetDC(tempHwnd);
		assert(tempdc);

		PIXELFORMATDESCRIPTOR pfd = { 0, };
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int format = ChoosePixelFormat(tempdc, &pfd);
		assert(format);

		DescribePixelFormat(tempdc, format, sizeof(pfd), &pfd);
		BOOL set = SetPixelFormat(tempdc, format, &pfd);
		assert(set);

		temprc = wglCreateContext(tempdc);
		assert(temprc);

		BOOL make = wglMakeCurrent(tempdc, temprc);
		assert(make);

		auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		int attrib[] =
		{
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0,
		};

		HGLRC newrc = wglCreateContextAttribsARB(tempdc, NULL, attrib);
		assert(newrc);

		make = wglMakeCurrent(tempdc, newrc);
		assert(make);

		wglDeleteContext(temprc);
		temprc = newrc;

		if (!gladLoadGL())
			assert(false);
		if (!gladLoadWGL(tempdc))
			assert(false);

		assert(GLAD_WGL_NV_DX_interop2 != 0);
	}

	glDebugMessageCallback(DebugCallback, 0);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

HRESULT InitD3D(HWND hWnd)
{
	/* 
	 * IDirect3D9Ex device is required.
	 *
	 * if not, wglDXRegisterObjectNV returns 0
	 */
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D)))
		return E_FAIL;

	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/full-scene-antialiasing
	/*
	 * The code below assumes that pD3D is a valid pointer 
	 *   to a IDirect3D9 interface.
	 */
	DWORD quality;
	if (FAILED(g_pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
						  D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, TRUE, 
						  D3DMULTISAMPLE_4_SAMPLES, &quality)))
		return E_FAIL;
	
    // Set up the structure used to create the D3DDevice
    ZeroMemory( &g_d3dpp, sizeof( g_d3dpp ) );
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // required. to enable multisampling
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    // Create the D3DDevice
	if (FAILED(g_pD3D->CreateDeviceEx(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING |
		D3DCREATE_PUREDEVICE | D3DCREATE_MULTITHREADED,
		&g_d3dpp, NULL, &g_pd3dDevice)))
    {
        return E_FAIL;
    }

	GLCreate();

	// register the Direct3D device with GL
	gl_handleD3D = wglDXOpenDeviceNV(g_pd3dDevice);
	assert(gl_handleD3D);

	glGenFramebuffers(1, &gl_fbo);
	glGenTextures(2, gl_names);

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
    if( FAILED( g_pd3dDevice->CreateVertexBuffer( 3 * sizeof( CUSTOMVERTEX ),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_MANAGED, &g_pVB, NULL ) ) )
    {
        return E_FAIL;
    }

    // Now we fill the vertex buffer. To do this, we need to Lock() the VB to
    // gain access to the Vertices. This mechanism is required becuase vertex
    // buffers may be in device memory.
    VOID* pVertices;
    if( FAILED( g_pVB->Lock( 0, sizeof( Vertices ), ( void** )&pVertices, 0 ) ) )
        return E_FAIL;
    memcpy( pVertices, Vertices, sizeof( Vertices ) );
    g_pVB->Unlock();

    return S_OK;
}

VOID Render()
{ 
	if (dxColorBuffer == nullptr)
		return;

	g_pd3dDevice->SetRenderTarget(0, dxColorBuffer);

	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0xFF), 1.f, 0);
	g_pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);

	if (TRUE) // SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
#if 0
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
        g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

		// End the scene
		g_pd3dDevice->EndScene();
#endif

#if 0
		// lock the render targets for GL access
		wglDXLockObjectsNV(gl_handleD3D, 2, gl_handles);

		glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);

            glBegin(GL_TRIANGLES);
            glColor3f(1, 0, 0);
            glVertex2f(0.f, -0.5f);
            glColor3f(0, 1, 0);
            glVertex2f(0.5f, 0.5f);
            glColor3f(0, 0, 1);
            glVertex2f(-0.5f, 0.5f);
            glEnd();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// unlock the render targets
		wglDXUnlockObjectsNV(gl_handleD3D, 2, gl_handles);
#endif
	}

	// just set back to screen surface
	g_pd3dDevice->SetRenderTarget(0, g_backBufferColor);

	// copy rendertarget to backbuffer
	g_pd3dDevice->StretchRect(dxColorBuffer, NULL, g_backBufferColor, NULL, D3DTEXF_NONE);

	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
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
        if (InitVB())
        {
            // Show the window
            ShowWindow(hWnd, SW_SHOWDEFAULT);
            UpdateWindow(hWnd);

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                } else {
					Render();
				}
            }
        }
    }

	UnregisterClass(L"D3D Tutorial", wc.hInstance);

	return 0;
}
