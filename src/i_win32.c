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

String RequiredGLExtensions[] = {
	ISTRING("WGL_ARB_pixel_format"),
};

bool
LoadGL()
{
	bool succeeded = false;

	HWND window   = 0;
	HDC dc        = 0;
	HGLRC context = 0;

	do
	{
		window = CreateWindowA("STATIC", "IllumDummyWindow", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
		if (window == 0)
		{
			//// ERROR
			break;
		}

		dc = GetDC(window);
		if (dc == 0)
		{
			//// ERROR
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

		int format_index = ChoosePixelFormat(dc, &format);
		if (format_index == 0)
		{
			//// ERROR
			break;
		}

		if (!SetPixelFormat(dc, format_index, &format))
		{
			//// ERROR
			break;
		}

		context = wglCreateContext(dc);
		if (context == 0)
		{
			//// ERROR
			break;
		}

		if (!wglMakeCurrent(dc, context))
		{
			//// ERROR
			break;
		}

		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtesionsStringARB");
		if (wglGetExtensionsStringARB == 0)
		{
			//// ERROR
			break;
		}

		u8* extensions_string = (u8*)wglGetExtensionsStringARB(dc);
		if (extensions_string == 0)
		{
			//// ERROR
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
			//// ERROR
			break;
		}

		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		if (wglChoosePixelFormatARB == 0)
		{
			//// ERROR
			break;
		}

		// TODO:

	} while (0);

	if (context != 0) wglDeleteContext(context);
	if (dc != 0)      ReleaseDC(window, dc);
	if (window != 0)  DestroyWindow(window);

	return succeeded;
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

	if (!LoadGL())
	{
		//// ERROR: Failed to load Open GL
		return 1;
	}

	return 0;
}
