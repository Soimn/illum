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

#define GL_FUNC_LIST()                                     \
	X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT)         \
	X(PFNGLUNIFORM2IPROC, glUniform2i)                       \
	X(PFNGLUNIFORM2FPROC, glUniform2f)                       \
	X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)     \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)           \
	X(PFNGLCREATESHADERPROC, glCreateShader)                 \
	X(PFNGLSHADERSOURCEPROC, glShaderSource)                 \
	X(PFNGLCOMPILESHADERPROC, glCompileShader)               \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv)                   \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)         \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram)               \
	X(PFNGLATTACHSHADERPROC, glAttachShader)                 \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram)                   \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                 \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)       \
	X(PFNGLDELETESHADERPROC, glDeleteShader)                 \
	X(PFNGLUSEPROGRAMPROC, glUseProgram)                     \
  X(PFNGLDISPATCHCOMPUTEPROC, glDispatchCompute)           \
  X(PFNGLMEMORYBARRIERPROC, glMemoryBarrier)               \
	X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
	X(PFNGLVALIDATEPROGRAMPROC, glValidateProgram)           \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture)               \
	X(PFNGLBINDIMAGETEXTUREPROC, glBindImageTexture)         \
	X(PFNGLUNIFORM1IPROC, glUniform1i)                       \
	X(PFNGLUNIFORM1UIPROC, glUniform1ui)                     \

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

void
GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
	}
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

	HWND window         = 0;
	HDC dc              = 0;
	HGLRC gl_context    = 0;
	GLuint default_vao  = 0;
	GLuint disp_program = 0;
	GLuint comp_program = 0;

	GLuint display_texture = 0;

	do
	{
		u32 scratch_memory_size = 2*1024*1024;
		u8* scratch_memory = VirtualAlloc(0, scratch_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (scratch_memory == 0)
		{
			//// ERROR
			OutputDebugStringA("ERROR: Failed to allocate memory for startup\n");
			failed_setup = true;
			break;
		}

		{ /// Set working directory
			u32 scratch_memory_size_wchars = scratch_memory_size/sizeof(WCHAR);

			u32 result = GetModuleFileNameW(0, (WCHAR*)scratch_memory, scratch_memory_size_wchars);
			if (result == 0 || result >= scratch_memory_size_wchars-1)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to query current exe path\n");
				failed_setup = true;
				break;
			}

			((WCHAR*)scratch_memory)[result] = 0;

			WCHAR* last_slash = (WCHAR*)scratch_memory;
			for (WCHAR* scan = last_slash; *scan != 0; ++scan)
			{
				if (*scan == '/' || *scan == '\\') last_slash = scan;
			}

			*(last_slash + 1) = 0;

			if (!SetCurrentDirectoryW((WCHAR*)scratch_memory))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to set working directory\n");
				failed_setup = true;
				break;
			}
		}

		if (!CreateWindowAndGLContext(instance, &window, &dc, &gl_context))
		{
			//// ERROR
			OutputDebugStringA("ERROR: Failed to create a gl enabled window\n");
			failed_setup = true;
			break;
		}

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLDebugProc, 0);

		{ /// Upload display program
			u8* vert_shader_code =
				"#version 450 core\n"
				"#line " STRINGIFY(__LINE__) "\n" // NOTE: trick from https://gist.github.com/mmozeiko/6825cb94d393cb4032d250b8e7cc9d14#file-win32_opengl_multi-c-L446
				"\n"
				"out gl_PerVertex { vec4 gl_Position; };\n"
				"out vec2 uv;\n"
				"\n"
				"void\n"
				"main()\n"
				"{\n"
				"  vec2 a = vec2(gl_VertexID%2, gl_VertexID/2);\n"
				"  gl_Position = vec4(a*4 - 1, 0, 1);\n"
				"  uv = a*2;\n"
				"}\n"
			;

			u8* frag_shader_code =
				"#version 450 core\n"
				"#line " STRINGIFY(__LINE__) "\n" // NOTE: trick from https://gist.github.com/mmozeiko/6825cb94d393cb4032d250b8e7cc9d14#file-win32_opengl_multi-c-L446
				"\n"
				"layout(location = 0) uniform vec2 Resolution;\n"
				"\n"
				"layout(binding = 0) uniform sampler2D DisplayTexture;\n"
				"\n"
				"in vec2 uv;\n"
				"\n"
				"out vec4 color;\n"
				"\n"
				"void\n"
				"main()\n"
				"{\n"
				/*"  vec2 uv = gl_FragCoord.xy / Resolution;\n"
				"  vec2 auv = (2*uv - 1) * vec2(1, Resolution.y/Resolution.x);\n"
				"  color = vec4((length(auv) < 0.5 ? uv : vec2(0)), 0, 1);\n"*/
				"  color = vec4(texture(DisplayTexture, uv).rgb, 1);\n"
				"}\n"
			;

			GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vert_shader, 1, &vert_shader_code, 0);
			glCompileShader(vert_shader);

			GLint status;
			glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetShaderInfoLog(vert_shader, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to compile vertex shader. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(frag_shader, 1, &frag_shader_code, 0);
			glCompileShader(frag_shader);

			glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetShaderInfoLog(frag_shader, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to compile fragment shader. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			disp_program = glCreateProgram();
			glAttachShader(disp_program, vert_shader);
			glAttachShader(disp_program, frag_shader);
			glLinkProgram(disp_program);

			glGetProgramiv(disp_program, GL_LINK_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetProgramInfoLog(disp_program, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to link display disp_program. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			glValidateProgram(disp_program);
			glGetProgramiv(disp_program, GL_VALIDATE_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetProgramInfoLog(disp_program, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to validate display disp_program. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			glDeleteShader(vert_shader);
			glDeleteShader(frag_shader);
		}

		{ /// Upload tracing compute program
			// TODO: Decide on where resources should be stored
			HANDLE comp_shader_code_file = CreateFileW(L"..\\src\\tracing.glsl", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (comp_shader_code_file == INVALID_HANDLE_VALUE)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to open tracing compute shader file\n");
				failed_setup = true;
				break;
			}

			LARGE_INTEGER file_size;
			u32 read_bytes;
			if (!GetFileSizeEx(comp_shader_code_file, &file_size))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to query size of tracing compute shader file\n");
				failed_setup = true;
			}
			else if (file_size.HighPart != 0 || file_size.LowPart > scratch_memory_size)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Tracing compute shader file is too large\n");
				failed_setup = true;
			}
			else if (!ReadFile(comp_shader_code_file, scratch_memory, file_size.LowPart, &read_bytes, 0) || read_bytes != file_size.LowPart)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to read tracing compute shader file\n");
				failed_setup = true;
			}

			u32 comp_shader_code_size = read_bytes;

			CloseHandle(comp_shader_code_file);
			if (failed_setup) break;

			HANDLE comp_pcg_code_file = CreateFileW(L"..\\vendor\\pcg32\\pcg32.glsl", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (comp_pcg_code_file == INVALID_HANDLE_VALUE)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to open pcg32 compute shader file\n");
				failed_setup = true;
				break;
			}

			if (!GetFileSizeEx(comp_pcg_code_file, &file_size))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to query size of pcg32 compute shader file\n");
				failed_setup = true;
			}
			else if (file_size.HighPart != 0 || file_size.LowPart + comp_shader_code_size-1 > scratch_memory_size)
			{
				//// ERROR
				OutputDebugStringA("ERROR: pcg32 compute shader file is too large\n");
				failed_setup = true;
			}
			else if (!ReadFile(comp_pcg_code_file, scratch_memory + comp_shader_code_size-1, file_size.LowPart, &read_bytes, 0) || read_bytes != file_size.LowPart)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to read pcg32 compute shader file\n");
				failed_setup = true;
			}

			CloseHandle(comp_pcg_code_file);
			if (failed_setup) break;

			GLint status;
			GLuint comp_shader = glCreateShader(GL_COMPUTE_SHADER);
			glShaderSource(comp_shader, 1, &scratch_memory, 0);
			glCompileShader(comp_shader);

			glGetShaderiv(comp_shader, GL_COMPILE_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetShaderInfoLog(comp_shader, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to compile compute shader. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			comp_program = glCreateProgram();
			glAttachShader(comp_program, comp_shader);
			glLinkProgram(comp_program);

			glGetProgramiv(comp_program, GL_LINK_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetProgramInfoLog(comp_program, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to link tracing compute disp_program. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			glValidateProgram(comp_program);
			glGetProgramiv(comp_program, GL_VALIDATE_STATUS, &status);
			if (status == 0)
			{
				//// ERROR
				u8 buffer[1024];
				glGetProgramInfoLog(comp_program, ARRAY_SIZE(buffer), 0, buffer);
				OutputDebugStringA("ERROR: Failed to validate tracing compute disp_program. OpenGL reports the following error message:\n");
				OutputDebugStringA(buffer);
				OutputDebugStringA("\n");
				failed_setup = true;
				break;
			}

			glDeleteShader(comp_shader);
		}

		{ /// Prepare tracing textures
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &display_texture);
			glBindTexture(GL_TEXTURE_2D, display_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1920, 1080, 0, GL_RGBA, GL_FLOAT, 0); // TODO: dimensions
			glBindImageTexture(0, display_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
		}

		glCreateVertexArrays(1, &default_vao);
	} while (0);

	if (!failed_setup)
	{
		// TODO: Vsync
		wglSwapIntervalEXT(1);

		ShowWindow(window, SW_SHOW);

		u32 frame_index = 0;

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

			RECT client_rect;
			GetClientRect(window, &client_rect);
			u32 width  = client_rect.right - client_rect.left;
			u32 height = client_rect.bottom - client_rect.top;
			glViewport(0, 0, width, height);

			glUseProgram(comp_program);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0); // TODO: probably highly inefficient
			glBindImageTexture(0, display_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
			glUniform2f(0, (f32)width, (f32)height);
			glUniform1ui(1, frame_index);
			glDispatchCompute(width/32 + (width%32 != 0), height/32 + (height%32 != 0), 1); // TODO
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			Sleep(10);

			glBindVertexArray(default_vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, display_texture);
			glUseProgram(disp_program);
			glUniform2f(0, (f32)width, (f32)height);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			SwapBuffers(dc);
			frame_index += 1;
		}
	}

	if (gl_context != 0) wglDeleteContext(gl_context);
	if (dc != 0)         ReleaseDC(window, dc);
	if (window != 0)     DestroyWindow(window);

	return 0;
}
