#include "stdafx.h"

#include "debug.h"
#include "glcontext_wgl.h"
#include <gl/GL.h>
#include "wglext.h"
using namespace gl;

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

void dumpExtensions(const char* _extensions)
{
	if (NULL != _extensions)
	{
		char name[1024];
		const char* pos = _extensions;
		const char* end = _extensions + strlen(_extensions);
		while (pos < end)
		{
			int len;
			const char* space = strstr(pos, " ");
			if (NULL != space)
			{
				len = min(sizeof(name), (int)(space - pos));
			}
			else
			{
				len = min(sizeof(name), strlen(pos));
			}

			strncpy_s(name, sizeof(name), pos, len);
			name[len] = '\0';

			TRACEA("%s\n", name);

			pos += len + 1;
		}
	}
}

// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
HGLRC createContext(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (0 == pixelFormat)
	{
		// error
		return NULL;
	}

	DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	TRACEA("Pixel format:\n\tiPixelType %d\n\tcColorBits %d\n\tcAlphaBits %d\n\tcDepthBits %d\n\tcStencilBits %d\n"
		, pfd.iPixelType
		, pfd.cColorBits
		, pfd.cAlphaBits
		, pfd.cDepthBits
		, pfd.cStencilBits
	);

	int result = SetPixelFormat(hDC, pixelFormat, &pfd);
	if (0 == result)
	{
		// error
		return NULL;
	}

	HGLRC context = wglCreateContext(hDC);
	if (NULL == context)
	{
		// error
		return NULL;
	}

	result = wglMakeCurrent(hDC, context);
	if (0 == result)
	{
		// error
		return NULL;
	}

	return context;
}

GlContext::GlContext()
	: m_context(NULL)
	, m_hDC(NULL)
	, m_hWnd(NULL)
{

}

// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
void GlContext::create(HWND hWnd)
{
	m_hWnd = hWnd;
	m_hDC = ::GetDC(m_hWnd);

	HWND dummyHWND = CreateWindow(_T("STATIC"), _T(""), WS_POPUP | WS_DISABLED, -32000, -32000, 0, 0, NULL, NULL, GetModuleHandle(NULL), 0);
	HDC dummyHDC = ::GetDC(dummyHWND);
	HGLRC context = createContext(dummyHDC);

	// wglGetProcAddress必须在有OpenGL context之后调用才有效
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (wglGetExtensionsStringARB != NULL)
	{
		const char* extensions = (const char*)wglGetExtensionsStringARB(dummyHDC);
		TRACEA("WGL extensions:\n");
		dumpExtensions(extensions);
	}

	if (wglChoosePixelFormatARB != NULL && wglCreateContextAttribsARB != NULL)
	{
		int attrs[] =
		{
			WGL_SAMPLE_BUFFERS_ARB, 0,
			WGL_SAMPLES_ARB, 0,
			WGL_SUPPORT_OPENGL_ARB, true,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_DRAW_TO_WINDOW_ARB, true,
			WGL_DOUBLE_BUFFER_ARB, true,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};

		int result;
		UINT numFormats = 0;
		int iPixelFormat = 0;
		do
		{
			result = wglChoosePixelFormatARB(m_hDC, attrs, NULL, 1, &iPixelFormat, &numFormats);
			if (0 == result
				|| 0 == numFormats)
			{
				attrs[3] >>= 1;
				attrs[1] = attrs[3] == 0 ? 0 : 1;
			}

		} while (0 == numFormats);

		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat(m_hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		TRACEA("Pixel format:\n\tiPixelType %d\n\tcColorBits %d\n\tcAlphaBits %d\n\tcDepthBits %d\n\tcStencilBits %d\n"
			, pfd.iPixelType
			, pfd.cColorBits
			, pfd.cAlphaBits
			, pfd.cDepthBits
			, pfd.cStencilBits
		);

		result = SetPixelFormat(m_hDC, iPixelFormat, &pfd);
		if (0 == result)
		{
			// error
			return;
		}

		int contextAttrs[9] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
			WGL_CONTEXT_MINOR_VERSION_ARB, 1,
			0, 0,
			0, 0,
			0
		};

		m_context = wglCreateContextAttribsARB(m_hDC, 0, contextAttrs);
		if (NULL == m_context)
		{
			// nVidia doesn't like context profile mask for contexts below 3.2?
			contextAttrs[6] = WGL_CONTEXT_PROFILE_MASK_ARB == contextAttrs[6] ? 0 : contextAttrs[6];
			m_context = wglCreateContextAttribsARB(m_hDC, 0, contextAttrs);
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(context);
	ReleaseDC(dummyHWND, dummyHDC);
	DestroyWindow(dummyHWND);

	if (NULL == m_context)
	{
		m_context = createContext(m_hDC);
	}

	int result = wglMakeCurrent(m_hDC, m_context);
	if (0 == result)
	{
		// error
		return;
	}

	if (NULL != wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(0);
	}
}

void GlContext::destroy()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(m_context);
	ReleaseDC(m_hWnd, m_hDC);
	m_context = NULL;
	m_hDC = NULL;
	m_hWnd = NULL;
}
