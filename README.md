1. Vertices - Simple triangle, copy source from DirectX Sample Tutorial 2
2. Triangle - Triangle rendered in ndc space
3. Resize - Enable resize
4. Antialiasing - Enable multi sampling
5. AntialiasRenderTarget - Render surface to msaa target and resolve it with stretch texture
6. Swapchain - Swapchain manage and handle device lost

==========================================================================

!! [3.Resize]

우선, XPDM(XP Driver Model)의 모델에서는 driver lost 상황이 잦았다.
Alt-tab 만 해도 발생했다던가, 그 때문에, 옛날 XP 시절 소스인 Luna 책의
d3dApp.cpp 만 봐도 매 frame 'isDeviceLost()' 를 호출하며 대기한다

```
    // while 로 성공할때 까지 반복

    // Get the state of the graphics device.
	HRESULT hr = gd3dDevice->TestCooperativeLevel();

	// If the device is lost and cannot be reset yet then
	// sleep for a bit and we'll try again on the next 
	// message loop cycle.
	if( hr == D3DERR_DEVICELOST )
	{
		Sleep(20);
		return true;
	}
	// Driver error, exit.
	else if( hr == D3DERR_DRIVERINTERNALERROR )
	{
		MessageBox(0, "Internal Driver Error...Exiting", 0, 0);
		PostQuitMessage(0);
		return true;
	}
	// The device is lost but we can reset and restore it.
	else if( hr == D3DERR_DEVICENOTRESET )
	{
		onLostDevice();
		HR(gd3dDevice->Reset(&md3dPP));
		onResetDevice();
		return false;
	}
	else
		return false;
```

그 때 복귀를 위해서는 Reset 함수를 호출하는데,

https://docs.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-reset

> Calling IDirect3DDevice9::Reset causes all texture memory surfaces to be lost, managed textures to be flushed from video memory, and all state information to be lost. Before calling the IDirect3DDevice9::Reset method for a device, an application should release any explicit render targets, depth stencil surfaces, additional swap chains, state blocks, and D3DPOOL_DEFAULT resources associated with the device.

텍스쳐 (surface) 다 없어질 거고, D3DPOOL_DEFAULT pool 에 속한 리소스도 없어지므로 새로 생성해야된다고 한다

실제 구현은, 아래에 써진 대로,

https://is03.tistory.com/44

Reset 호출 전에 다 해제를 해주고, 복구해줘야한다

그래서 위와 같이 onLostDevice, onResetDevice 함수와 함께 Reset을 사용해야한다.

문제는, 화면 surface 크기 갱신에도 Reset을 호출해야한다.

그 때문에, WM_SIZE에 onLostDevice/onResetDeivce/Reset 함수가 추가되었다.

대략 여기까지가 3번째 Resize 예제의 내용

번외로 Win10에서의 Reset 함수의 에러 메시지는 너무 간단해서 원인 찾기가 골치아픈데,

예전에는 자세한 로그가 같이 출력 되었으나 이젠 그냥 INVALIDCALL 만 돌려준다

https://dataprocess.tistory.com/502


!! [6.Swapchain]

윈 vista 이후의 OS에서 코드를 실행해보면, alt-tab 한다고 문제 생기진 않는다.

이건, VISTA 이후로 WDDM 에 따라 lost 상황이 거의 없어져서 생긴 현상이다.

https://docs.microsoft.com/en-us/previous-versions//ms681824(v=vs.85)?redirectedfrom=MSDN

> Devices are now only lost under two circumstances; when the hardware is reset because it is hanging, and when the device driver is stopped. When hardware hangs, the device can be reset by calling ResetEx. If hardware hangs, texture memory is lost.

아래 내역 참고하면,

https://github.com/bkaradzic/bgfx/blob/master/src/renderer_d3d9.cpp


```
// hr = device->Present(&pp);

if (isLost(hr) )
{
	do
	{
		do
		{
			hr = m_device->TestCooperativeLevel();
		}
		while (D3DERR_DEVICENOTRESET != hr);

		reset();
		hr = m_device->TestCooperativeLevel();
	}
	while (FAILED(hr) );

	break;
}
```

여전히, Reset 함수 호출 전 INVALIDCALL DEFAULT_POOL 의 버퍼와 텍스쳐를 해제 후 재생성해야 한다.


!! [7.DeviceEx]

DeviceEx

https://gamedev.stackexchange.com/questions/7993/idirect3ddevice9ex-and-d3dpool-managed

IDirect3DDevice9Ex::ResetEx

> Resets the type, size, and format of the swap chain with all other surfaces persistent.

https://stackoverflow.com/questions/61915988/how-to-handle-direct3d-9ex-d3derr-devicehung-error

https://chromium.googlesource.com/angle/angle/+/chromium/2175/src/libGLESv2/renderer/d3d/d3d9/Renderer9.cpp
