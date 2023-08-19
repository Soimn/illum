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

typedef float f32;

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

#define CONCAT_(X, Y) X##Y
#define CONCAT__(X, Y) CONCAT_(X, Y)
#define CONCAT(X, Y) CONCAT__(X, Y)

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

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
	ISTRING("WGL_EXT_swap_control"),

	ISTRING("WGL_ARB_pixel_format_float"),
};

#define GL_FUNC_LIST()                                 \
	X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT)     \
	X(PFNGLUNIFORM2IPROC, glUniform2i)                   \
	X(PFNGLUNIFORM2FPROC, glUniform2f)                   \
	X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays) \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)       \
	X(PFNGLCREATESHADERPROC, glCreateShader)             \
	X(PFNGLSHADERSOURCEPROC, glShaderSource)             \
	X(PFNGLCOMPILESHADERPROC, glCompileShader)           \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv)               \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)     \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram)           \
	X(PFNGLATTACHSHADERPROC, glAttachShader)             \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram)               \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)             \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)   \
	X(PFNGLDELETESHADERPROC, glDeleteShader)             \
	X(PFNGLUSEPROGRAMPROC, glUseProgram)                 \

#define X(T, N) T N;
GL_FUNC_LIST()
#undef X

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

#define X(T, N)                               \
			N = (T)wglGetProcAddress(STRINGIFY(N)); \
			if (!N)                                 \
			{                                       \
				/*Failed to load*/                    \
				break;                                \
			}

			GL_FUNC_LIST()
#undef X

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

	bool failed_setup = false;

	HWND window        = 0;
	HDC dc             = 0;
	HGLRC gl_context   = 0;
	GLuint default_vao = 0;
	GLuint program     = 0;

	do
	{
		if (!CreateWindowAndGLContext(instance, &window, &dc, &gl_context))
		{
			//// ERROR
			OutputDebugStringA("ERROR: Failed to ");
			failed_setup = true;
		}

		u8* vert_shader_code =
			"#version 450 core\n"
			"#line " STRINGIFY(__LINE__) "\n" // NOTE: trick from https://gist.github.com/mmozeiko/6825cb94d393cb4032d250b8e7cc9d14#file-win32_opengl_multi-c-L446
			"\n"
			"out gl_PerVertex { vec4 gl_Position; };\n"
			"\n"
			"void\n"
			"main()\n"
			"{\n"
			"  gl_Position = vec4(vec2(gl_VertexID%2, gl_VertexID/2)*4 - 1, 0, 1);\n"
			"}\n"
		;

		u8* frag_shader_code =
			"#version 450 core\n"
			"#line " STRINGIFY(__LINE__) "\n" // NOTE: trick from https://gist.github.com/mmozeiko/6825cb94d393cb4032d250b8e7cc9d14#file-win32_opengl_multi-c-L446
			"\n"
			"layout(location = 0) uniform vec2 Resolution;\n"
			"\n"
			"out vec4 color;\n"
			"\n"
			"void\n"
			"main()\n"
			"{\n"
			"  vec2 uv = gl_FragCoord.xy / Resolution;\n"
			"  vec2 auv = (2*uv - 1) * vec2(1, Resolution.y/Resolution.x);\n"
			"  color = vec4((length(auv) < 0.5 ? uv : vec2(0)), 0, 1);\n"
			"}\n"
		;

		GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert_shader, 1, &vert_shader_code, 0);
		glCompileShader(vert_shader);

		GLint compile_status;
		glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &compile_status);
		if (compile_status == 0)
		{
			//// ERROR
			u8 buffer[1024];
			glGetShaderInfoLog(vert_shader, ARRAY_SIZE(buffer), 0, buffer);
			OutputDebugStringA(buffer);
			failed_setup = true;
		}

		GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_shader, 1, &frag_shader_code, 0);
		glCompileShader(frag_shader);

		glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &compile_status);
		if (compile_status == 0)
		{
			//// ERROR
			u8 buffer[1024];
			glGetShaderInfoLog(frag_shader, ARRAY_SIZE(buffer), 0, buffer);
			OutputDebugStringA(buffer);
			failed_setup = true;
		}

		program = glCreateProgram();
		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);
		glLinkProgram(program);

		GLint link_status;
		glGetProgramiv(program, GL_LINK_STATUS, &link_status);
		if (link_status == 0)
		{
			//// ERROR
			u8 buffer[1024];
			glGetProgramInfoLog(program, ARRAY_SIZE(buffer), 0, buffer);
			OutputDebugStringA(buffer);
			failed_setup = true;
		}

		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);

		glCreateVertexArrays(1, &default_vao);
	} while (0);

	// TODO: Vsync
	wglSwapIntervalEXT(1);

	ShowWindow(window, SW_SHOW);

	Running = !failed_setup;
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

		RECT client_rect;
		GetClientRect(window, &client_rect);
		u32 width  = client_rect.right - client_rect.left;
		u32 height = client_rect.bottom - client_rect.top;
		glViewport(0, 0, width, height);

		Sleep(10);

		glBindVertexArray(default_vao);
		glUseProgram(program);
		glUniform2f(0, (f32)width, (f32)height);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		SwapBuffers(dc);
	}

	if (gl_context != 0) wglDeleteContext(gl_context);
	if (dc != 0)         ReleaseDC(window, dc);
	if (window != 0)     DestroyWindow(window);

	return 0;
}
