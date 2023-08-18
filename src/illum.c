#define UNICODE 1
#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>
#include <shellapi.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#undef UNICODE
#undef WIN32_LEAN_AND_MEAN
#undef far
#undef near

#include <stdio.h>

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef i64 imm;
typedef u64 umm;

typedef u8 bool;
#define true 1
#define false 0

typedef struct String
{
	u8* data;
	umm size;
} String;

#define ISTRING(S)        { .data = (u8*)(S), sizeof(S) - 1 }
#define STRING(S) (String){ .data = (u8*)(S), sizeof(S) - 1 }

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(0[A]))

bool
String_Match(String s0, String s1)
{
	bool result = (s0.size == s1.size);

	for (umm i = 0; i < s0.size && result; ++i) result = (s0.data[i] == s1.data[i]);

	return result;
}

/// --------------------------------------------------------------------------------------------

bool Running = false;

String RequiredGLExtensions[] = {
	ISTRING("WGL_ARB_pixel_format"),
	ISTRING("WGL_ARB_create_context"),
	ISTRING("WGL_ARB_create_context_profile"),

	ISTRING("WGL_ARB_pixel_format_float"),
};

bool
CreateWindowAndGLContext(HINSTANCE instance, HWND* window, HDC* dc, HGLRC* context)
{
	bool succeeded = false;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	{ /// Load WGL functions
		HWND dummy_window   = 0;
		HDC dummy_dc        = 0;
		HGLRC dummy_context = 0;

		do
		{
			dummy_window = CreateWindowA("STATIC", "IllumDummyWindow", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
			if (dummy_window == 0)
			{
				//// ERROR: Failed to create dummy window for loading WGL
				break;
			}

			dummy_dc = GetDC(dummy_window);
			if (dummy_dc == 0)
			{
				//// ERROR: Failed to get device context of dummy window for loading WGL
				break;
			}

			PIXELFORMATDESCRIPTOR format = {
				.nSize           = sizeof(PIXELFORMATDESCRIPTOR),
				.nVersion        = 1,
				.dwFlags         = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
				.iPixelType      = PFD_TYPE_RGBA,
				.cColorBits      = 24,
				.cAlphaBits      = 8,
				.cAccumBits      = 0, // NOTE: Probably not used
				.cDepthBits      = 24,
				.cStencilBits    = 8,
				.cAuxBuffers     = 0,
				.bReserved       = 0, // NOTE: Probably not used
				.dwLayerMask     = 0, // NOTE: Probably not used
				.dwVisibleMask   = 0, // NOTE: Probably not used
				.dwDamageMask    = 0, // NOTE: Probably not used
			};

			int format_index = ChoosePixelFormat(dummy_dc, &format);
			if (format_index == 0 || !SetPixelFormat(dummy_dc, format_index, &format))
			{
				//// ERROR: Failed to set pixel format on dummy window for loading WGL
				break;
			}

			dummy_context = wglCreateContext(dummy_dc);
			if (dummy_context == 0 || !wglMakeCurrent(dummy_dc, dummy_context))
			{
				//// ERROR: Failed to create dummy context for loading WGL functions
				break;
			}

			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			if (wglGetExtensionsStringARB == 0 || wglChoosePixelFormatARB == 0 || wglCreateContextAttribsARB == 0)
			{
				//// ERROR: Failed to load necessary WGL functions
				break;
			}

			u8* extensions_string = (u8*)wglGetExtensionsStringARB(dummy_dc);
			if (extensions_string == 0)
			{
				//// ERROR: Failed to query gl extensions
				break;
			}

			u64 found_extensions[(ARRAY_SIZE(RequiredGLExtensions)+63)/64] = {0};
			umm found_extension_count = 0;

			for (u8* scan = extensions_string;;)
			{
				while (*scan == ' ') ++scan;
				if (*scan == 0) break;

				String extension = { .data = (u8*)scan };
				while (*scan != ' ' && *scan != '0') ++scan, ++extension.size;

				for (umm i = 0; i < ARRAY_SIZE(RequiredGLExtensions); ++i)
				{
					if (String_Match(extension, RequiredGLExtensions[i]))
					{
						found_extensions[i/64] |= (1ULL << (i % 64));
						++found_extension_count;
						break;
					}
				}
			}

			if (found_extension_count != ARRAY_SIZE(RequiredGLExtensions))
			{
				//// ERROR: Not all required gl extensions are supported
				break;
			}

			succeeded = true;
		} while (0);

		if (context != 0) wglDeleteContext(*context);
		if (dc != 0)      ReleaseDC(*window, *dc);
		if (window != 0)  DestroyWindow(*window);
	}

	if (succeeded)
	{
		succeeded = false;
		do
		{
			LRESULT WindowProc(HWND window, UINT msg_code, WPARAM wparam, LPARAM lparam);
			WNDCLASSEXW window_class = {
				.cbSize        = sizeof(WNDCLASSEXW),
				.style         = CS_OWNDC,
				.lpfnWndProc   = WindowProc,
				.cbClsExtra    = 0,
				.cbWndExtra    = 0,
				.hInstance     = instance,
				.hIcon         = 0,
				.hCursor       = 0,
				.hbrBackground = 0,
				.lpszMenuName  = 0,
				.lpszClassName = L"IllumWindowClassName",
				.hIconSm       = 0,
			};

			ATOM window_class_atom = RegisterClassExW(&window_class);
			if (window_class_atom == 0)
			{
				//// ERROR: Failed to register window class
				break;
			}

			*window = CreateWindowExW(0, L"IllumWindowClassName", L"Illum", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
			if (*window == 0)
			{
				//// ERROR: Failed to create a window
				break;
			}

			*dc = GetDC(*window);
			if (*dc == 0)
			{
				//// ERROR: Failed to get window device context
				break;
			}

			int format_attribute_list[] = {
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
				WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
				WGL_COLOR_BITS_ARB,     24,
				WGL_DEPTH_BITS_ARB,     24,
				WGL_STENCIL_BITS_ARB,   8,
				0,
			};

			int format_index;
			if (!wglChoosePixelFormatARB(*dc, format_attribute_list, 0, 1, &format_index, &(UINT){0}))
			{
				//// ERROR: Failed to find an appropriate pixel format for the window
				break;
			}

			PIXELFORMATDESCRIPTOR format;
			if (!DescribePixelFormat(*dc, format_index, sizeof(PIXELFORMATDESCRIPTOR), &format) || !SetPixelFormat(*dc, format_index, &format))
			{
				//// ERROR: Failed to set chosen pixel format
				break;
			}

			int context_attribute_list[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
				WGL_CONTEXT_MINOR_VERSION_ARB, 5,
				WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
				0,
			};

			*context = wglCreateContextAttribsARB(*dc, 0, context_attribute_list);
			int a = GetLastError();
			if (*context == 0)
			{
				//// ERROR: Failed to create gl context
				break;
			}

			if (!wglMakeCurrent(*dc, *context))
			{
				//// ERROR: Failed to make gl context current
				break;
			}

			succeeded = true;
		} while (0);
	}

	if (!succeeded)
	{
		if (*context != 0) wglDeleteContext(*context);
		if (*dc != 0)      ReleaseDC(*window, *dc);
		if (*window != 0)  DestroyWindow(*window);
	}

	return succeeded;
}

LRESULT
WindowProc(HWND window, UINT msg_code, WPARAM wparam, LPARAM lparam)
{
	if (msg_code == WM_QUIT || msg_code == WM_CLOSE)
	{
		Running = false;
		return 0;
	}
	else return DefWindowProcW(window, msg_code, wparam, lparam);
}

int
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int show_cmd)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc != 1)
	{
		fprintf(stderr, "ERROR: Invalid number of arguments. Expected: illum\n");
		return 1;
	}

	HWND window;
	HDC dc;
	HGLRC gl_context;
	if (!CreateWindowAndGLContext(instance, &window, &dc, &gl_context))
	{
		//// ERROR
		return 1;
	}

	ShowWindow(window, SW_SHOW);

	Running = true;
	while (Running)
	{
		MSG msg;
		while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
			{
				Running = false;
				break;
			}
			else WindowProc(window, msg.message, msg.wParam, msg.lParam);
		}

		Sleep(10);

		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(dc);
	}

	wglDeleteContext(gl_context);
	ReleaseDC(window, dc);
	DestroyWindow(window);

	return 0;
}
