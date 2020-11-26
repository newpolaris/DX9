1. Vertices - Simple triangle, copy source from DirectX Sample Tutorial 2
2. Triangle - Triangle rendered in ndc space
3. Resize - Enable resize
4. Antialiasing - Enable multi sampling
5. AntialiasRenderTarget - Render surface to msaa target and resolve it with stretch texture
6. Swapchain - Swapchain manage and handle device lost

==========================================================================

!! [3.Resize]

�켱, XPDM(XP Driver Model)�� �𵨿����� driver lost ��Ȳ�� ��Ҵ�.
Alt-tab �� �ص� �߻��ߴٴ���, �� ������, ���� XP ���� �ҽ��� Luna å��
d3dApp.cpp �� ���� �� frame 'isDeviceLost()' �� ȣ���ϸ� ����Ѵ�

```
    // while �� �����Ҷ� ���� �ݺ�

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

�� �� ���͸� ���ؼ��� Reset �Լ��� ȣ���ϴµ�,

https://docs.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-reset

> Calling IDirect3DDevice9::Reset causes all texture memory surfaces to be lost, managed textures to be flushed from video memory, and all state information to be lost. Before calling the IDirect3DDevice9::Reset method for a device, an application should release any explicit render targets, depth stencil surfaces, additional swap chains, state blocks, and D3DPOOL_DEFAULT resources associated with the device.

�ؽ��� (surface) �� ������ �Ű�, D3DPOOL_DEFAULT pool �� ���� ���ҽ��� �������Ƿ� ���� �����ؾߵȴٰ� �Ѵ�

���� ������, �Ʒ��� ���� ���,

https://is03.tistory.com/44

Reset ȣ�� ���� �� ������ ���ְ�, ����������Ѵ�

�׷��� ���� ���� onLostDevice, onResetDevice �Լ��� �Բ� Reset�� ����ؾ��Ѵ�.

������, ȭ�� surface ũ�� ���ſ��� Reset�� ȣ���ؾ��Ѵ�.

�� ������, WM_SIZE�� onLostDevice/onResetDeivce/Reset �Լ��� �߰��Ǿ���.

�뷫 ��������� 3��° Resize ������ ����

���ܷ� Win10������ Reset �Լ��� ���� �޽����� �ʹ� �����ؼ� ���� ã�Ⱑ ��ġ���µ�,

�������� �ڼ��� �αװ� ���� ��� �Ǿ����� ���� �׳� INVALIDCALL �� �����ش�

https://dataprocess.tistory.com/502


!! [6.Swapchain]

�� vista ������ OS���� �ڵ带 �����غ���, alt-tab �Ѵٰ� ���� ������ �ʴ´�.

�̰�, VISTA ���ķ� WDDM �� ���� lost ��Ȳ�� ���� �������� ���� �����̴�.

https://docs.microsoft.com/en-us/previous-versions//ms681824(v=vs.85)?redirectedfrom=MSDN

> Devices are now only lost under two circumstances; when the hardware is reset because it is hanging, and when the device driver is stopped. When hardware hangs, the device can be reset by calling ResetEx. If hardware hangs, texture memory is lost.

�Ʒ� ���� �����ϸ�,

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

������, Reset �Լ� ȣ�� �� INVALIDCALL DEFAULT_POOL �� ���ۿ� �ؽ��ĸ� ���� �� ������ؾ� �Ѵ�.


!! [7.DeviceEx]

DeviceEx

https://gamedev.stackexchange.com/questions/7993/idirect3ddevice9ex-and-d3dpool-managed

IDirect3DDevice9Ex::ResetEx

> Resets the type, size, and format of the swap chain with all other surfaces persistent.

https://stackoverflow.com/questions/61915988/how-to-handle-direct3d-9ex-d3derr-devicehung-error

https://chromium.googlesource.com/angle/angle/+/chromium/2175/src/libGLESv2/renderer/d3d/d3d9/Renderer9.cpp
