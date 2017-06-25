#include <stdio.h>
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "help.h"

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_TEX1)  

struct CUSTOMVERTEX
{
	FLOAT x, y, z, rhw;
	FLOAT u, v;
};

static const float g_yuv_mat[16] = {0.182586f, -0.100644f,  0.439216f, 0.0f,
                                  0.614231f, -0.338572f, -0.398942f, 0.0f,
                                  0.062007f,  0.439216f, -0.040274f, 0.0f,
                                  0.062745f,  0.501961f,  0.501961f, 1.0f};

class d3dchromakey
{
public:
	int m_w;
	int m_h;
	IDirect3D9* m_d3d;
	IDirect3DDevice9* m_d3dd;
	ID3DXEffect* m_d3dxe;
	IDirect3DVertexBuffer9* m_d3dvbuf;
	IDirect3DTexture9* m_d3dt;

public:
	d3dchromakey(HWND hWnd, UINT w, UINT h):m_d3d(NULL),m_d3dd(NULL),m_d3dxe(NULL),m_d3dvbuf(NULL),m_d3dt(NULL)
	{
		void* ptmp;
		D3DPRESENT_PARAMETERS d3dpp;
		
		m_w = w;
		m_h = h;
		
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;	
	
		m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
		
		if (FAILED(m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3dpp, &m_d3dd)))
		{
			printf("CreateDevice fail\n");
		}
		
		m_d3dd->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_d3dd->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_d3dd->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		
		m_d3dd->SetRenderState(D3DRS_LIGHTING, FALSE);
		m_d3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_d3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_d3dd->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		
		m_d3dd->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);  
		m_d3dd->SetTextureStageState(0, D3DTSS_ALPHAOP,  D3DTOP_SELECTARG1);
		
		CUSTOMVERTEX vertices[] =
		{
			{ 20.0f, 20.0f, 0.5f, 1.0f, 0.f, 0.f },
			{ w-20.0f, 20.0f, 0.5f, 1.0f, 1.f, 0.f },
			{ w-20.0f, h-20.0f, 0.5f, 1.0f, 1.f, 1.f },

			{ 20.0f, 20.0f, 0.5f, 1.0f, 0.f, 0.f },
			{ w-20.0f, h-20.0f, 0.5f, 1.0f, 1.f, 1.f },
			{ 20.0f, h-20.0f, 0.5f, 1.0f, 0.f, 1.f },
		};
		
		if (FAILED(m_d3dd->CreateVertexBuffer(6 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX,
			D3DPOOL_DEFAULT, &m_d3dvbuf, NULL)))
		{
			printf("CreateVertexBuffer fail\n");
		}
	
		if (FAILED(m_d3dvbuf->Lock(0, sizeof(vertices), (void**)&ptmp, 0)))
		{
			printf("m_d3dvbuf->Lock fail\n");
		}
		memcpy(ptmp, vertices, sizeof(vertices));
		m_d3dvbuf->Unlock();
		
		if (FAILED(m_d3dd->CreateTexture(w, h, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_d3dt, NULL)))
		{
			printf("CreateTexture fail\n");
		}	

		ID3DXBuffer* d3dxbuf = NULL;
		if (FAILED(D3DXCreateEffectFromFile(m_d3dd, "G:\\dshow\\d3d9effect\\1.fx", NULL, NULL, 0, NULL, &m_d3dxe, &d3dxbuf)))
		{
			printf("D3DXCreateEffectFromFile fail\n");
		}
			
		
		if(FAILED(m_d3dxe->SetTechnique("Draw")))
		{
			printf("SetTechnique fail\n");
		}
		
	}

	~d3dchromakey()
	{
		if (m_d3dt) m_d3dt->Release();
		if (m_d3dvbuf) m_d3dvbuf->Release();
		if (m_d3dxe) m_d3dxe->Release();
		if (m_d3dd) m_d3dd->Release();
		if (m_d3d) m_d3d->Release();
	}
	
	void set_effect_vars()
	{
		D3DXMATRIX ViewProj;
		D3DXMatrixIdentity(&ViewProj);
		if( FAILED(m_d3dxe->SetMatrix("ViewProj", &ViewProj)))
		{
			printf("SetMatrix fail\n");
		}		
		
		if( FAILED(m_d3dxe->SetTexture("image", m_d3dt)))
		{
			printf("SetTexture fail\n");
		}
		
		uint32_t color_i = 0xFFFFFF | (((100 * 255) / 100) << 24);
		vec4 color_f;
		vec4_from_rgba(&color_f, color_i);
		printf("opacity: %f, %f, %f, %f\n",color_f.ptr[0],color_f.ptr[1],color_f.ptr[2],color_f.ptr[3]);
		if( FAILED(m_d3dxe->SetFloatArray("color", color_f.ptr, 4)))
		{
			printf("SetFloatArray fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("contrast", 1.0f)))
		{
			printf("SetFloat fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("brightness", 0)))
		{
			printf("SetFloat fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("gamma", 1.0f)))
		{
			printf("SetFloat fail\n");
		}
		
		vec2 chroma_key;
		vec4 key_rgb;
		vec4_from_rgba(&key_rgb, 0x009900 | 0xFF000000);
		printf("key_rbg: %f, %f, %f, %f\n",key_rgb.ptr[0],key_rgb.ptr[1],key_rgb.ptr[2],key_rgb.ptr[3]);
		
		vec4 key_color_v4;
		matrix4 yuv_mat_m4;
		memcpy(&yuv_mat_m4, g_yuv_mat, sizeof(g_yuv_mat));
		vec4_transform(&key_color_v4, &key_rgb, &yuv_mat_m4);
		vec2_set(&chroma_key, key_color_v4.y, key_color_v4.z);
		printf("chroma_key: %f, %f", chroma_key.ptr[0], chroma_key.ptr[1]);
		if( FAILED(m_d3dxe->SetFloatArray("chroma_key", chroma_key.ptr, 2)))
		{
			printf("SetFloatArray fail\n");
		}
		
		float pixel_size[] = {1.0f/m_w,1.0f/m_h};
		if( FAILED(m_d3dxe->SetFloatArray("pixel_size", pixel_size, 2)))
		{
			printf("SetFloatArray fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("similarity", 400/1000.0f)))
		{
			printf("SetFloat fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("smoothness", 80/1000.0f)))
		{
			printf("SetFloat fail\n");
		}
		
		if( FAILED(m_d3dxe->SetFloat("spill", 100/1000.0f)))
		{
			printf("SetFloat fail\n");
		}
		HRESULT hr;
		if(FAILED(hr = m_d3dxe->ValidateTechnique("Draw")))
		{
			printf("ValidateTechnique fail\n");
			printf("%lx\n", hr);
		}
	}
	
	void render()
	{
		D3DLOCKED_RECT lockRect;
		if (FAILED(m_d3dt->LockRect(0, &lockRect, NULL, 0)))
		{
			printf("m_d3dt->LockRect fail\n");
		}

		int size_of_textrue = m_w*m_h * 4;
		BYTE* dst = (BYTE*)lockRect.pBits;
		for (int j = 0; j < size_of_textrue; j+=4)
		{
			if (j<size_of_textrue/2)
			{
				*dst++ = 255;
				*dst++ = 0;
				*dst++ = 0;
				*dst++ = 255;			
			}
			else
			{
				*dst++ = 0;
				*dst++ = 0;
				*dst++ = 255;
				*dst++ = 255;
			}


		}
		m_d3dt->UnlockRect(0);
		
		m_d3dd->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB( 255, 255, 255, 255 ), 1.0f, 0);

		if (SUCCEEDED(m_d3dd->BeginScene()))
		{
			m_d3dd->SetStreamSource(0, m_d3dvbuf, 0, sizeof(CUSTOMVERTEX));
			m_d3dd->SetFVF(D3DFVF_CUSTOMVERTEX);
			//m_d3dd->SetTexture(0, m_d3dt);
			//m_d3dd->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
			
			
			set_effect_vars();
			m_d3dxe->Begin(NULL,0);
				m_d3dxe->BeginPass(0);
				m_d3dxe->CommitChanges();
				m_d3dd->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
				m_d3dxe->EndPass();
			m_d3dxe->End();
				
			m_d3dd->EndScene();
		}
		
		m_d3dd->Present(NULL, NULL, NULL, NULL);
	}

};


d3dchromakey* g_chromakey = NULL;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (WM_PAINT == message)
	{
		if (g_chromakey) g_chromakey->render();		
	}
	else if (WM_DESTROY == message)
	{
		PostQuitMessage(0);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


int main()
{
	RECT rt;
	MSG  msg;
	HWND hWnd;
	WNDCLASSEXA wcex;
	

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;// (HBRUSH)(COLOR_BTNFACE + 1); //CreateSolidBrush(RGB(222, 208, 142));
	wcex.lpszMenuName = 0;// MAKEINTRESOURCE(IDC_DAMI);
	wcex.lpszClassName = "dxeffectwnd";
	wcex.hIconSm = NULL;

	RegisterClassExA(&wcex);

	hWnd = CreateWindow("dxeffectwnd", "abc",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		300, 200, 800, 600, NULL, NULL, wcex.hInstance, NULL);

	ShowWindow(hWnd, SW_SHOW);

	GetClientRect(hWnd, &rt);
	g_chromakey = new d3dchromakey(hWnd, rt.right, rt.bottom);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete g_chromakey;
	return 0;
}


