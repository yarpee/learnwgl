#pragma once

#include <Windows.h>

namespace gl
{
	class GlContext
	{
	public:
		GlContext();
		void create(HWND hWnd);
		void destroy();

		HDC GetDC()
		{
			return m_hDC;
		}

	private:
		HGLRC m_context;
		HDC m_hDC;
		HWND m_hWnd;
	};
}
