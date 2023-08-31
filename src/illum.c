#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
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
#pragma clang diagnostic pop

typedef __INT8_TYPE__  i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;

typedef __UINT8_TYPE__  u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

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

static bool Running = false;

#include "packed_shaders.h"

static String RequiredGLExtensions[] = {
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
	X(PFNGLCLEARTEXIMAGEPROC, glClearTexImage)               \

#define X(T, N) static T N;
GL_FUNC_LIST()
#undef X

bool
CreateWindowAndGLContext(HINSTANCE instance, HWND* window, HDC* dc, HGLRC* context)
{
	bool succeeded = false;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB       = 0;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
	{ /// Load WGL functions
		HWND dummy_window   = 0;
		HDC dummy_dc        = 0;
		HGLRC dummy_context = 0;

		do
		{
			dummy_window = CreateWindowA("STATIC", "IllumDummyWindow", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
			if (dummy_window == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to create dummy window for loading WGL\n");
				break;
			}

			dummy_dc = GetDC(dummy_window);
			if (dummy_dc == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to get device context of dummy window for loading WGL\n");
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
				//// ERROR
				OutputDebugStringA("ERROR: Failed to set pixel format on dummy window for loading WGL\n");
				break;
			}

			dummy_context = wglCreateContext(dummy_dc);
			if (dummy_context == 0 || !wglMakeCurrent(dummy_dc, dummy_context))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to create dummy context for loading WGL functions\n");
				break;
			}

			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			wglChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			if (wglGetExtensionsStringARB == 0 || wglChoosePixelFormatARB == 0 || wglCreateContextAttribsARB == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to load necessary WGL functions\n");
				break;
			}

			u8* extensions_string = (u8*)wglGetExtensionsStringARB(dummy_dc);
			if (extensions_string == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to query gl extensions\n");
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
				OutputDebugStringA("ERROR: Not all required gl extensions are supported\n");
				break;
			}

#define X(T, N)                                                                         \
			N = (T)wglGetProcAddress(STRINGIFY(N));                                           \
			if (!N)                                                                           \
			{                                                                                 \
				OutputDebugStringA("ERROR: Failed to load GL function \"" STRINGIFY(N) "\"\n"); \
				break;                                                                          \
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
				//// ERROR
				OutputDebugStringA("ERROR: Failed to register window class\n");
				break;
			}

			*window = CreateWindowExW(0, L"IllumWindowClassName", L"Illum", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
			if (*window == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to create a window\n");
				break;
			}

			*dc = GetDC(*window);
			if (*dc == 0)
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to get window device context\n");
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
				//// ERROR
				OutputDebugStringA("ERROR: Failed to find an appropriate pixel format for the window\n");
				break;
			}

			PIXELFORMATDESCRIPTOR format;
			if (!DescribePixelFormat(*dc, format_index, sizeof(PIXELFORMATDESCRIPTOR), &format) || !SetPixelFormat(*dc, format_index, &format))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to set chosen pixel format\n");
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
				//// ERROR
				OutputDebugStringA("ERROR: Failed to create gl context\n");
				break;
			}

			if (!wglMakeCurrent(*dc, *context))
			{
				//// ERROR
				OutputDebugStringA("ERROR: Failed to make gl context current\n");
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

	HWND window               = 0;
	HDC dc                    = 0;
	HGLRC gl_context          = 0;
	GLuint default_vao        = 0;
	GLuint disp_program       = 0;
	GLuint tracing_program    = 0;
	GLuint conversion_program = 0;

	GLuint sum_texture          = 0;
	GLuint compensation_texture = 0;
	GLuint display_texture      = 0;

	u32 texture_width  = 1920;
	u32 texture_height = 1080;

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

		{ /// Upload shaders
			struct {
				GLuint* program;
				GLchar** vert_shader_code;
				GLsizei vert_shader_count;
				GLchar** frag_shader_code;
				GLsizei frag_shader_count;
				GLchar** comp_shader_code;
				GLsizei comp_shader_count;
			} shaders[] = {
				{
					.program = &disp_program,
					.vert_shader_code  = &display_vert_shader_code,
					.vert_shader_count = 1,
					.frag_shader_code  = &display_frag_shader_code,
					.frag_shader_count = 1,
				},
				{
					.program = &tracing_program,
					.comp_shader_code  = (GLchar*[]){ tracing_shader_code, pcg32_shader_code },
					.comp_shader_count = 2,
				},
				{
					.program = &conversion_program,
					.comp_shader_code  = &conversion_shader_code,
					.comp_shader_count = 1,
				},
			};

			for (umm i = 0; i < ARRAY_SIZE(shaders); ++i)
			{
				GLuint* program           = shaders[i].program;
				GLchar** vert_shader_code = shaders[i].vert_shader_code;
				GLsizei vert_shader_count = shaders[i].vert_shader_count;
				GLchar** frag_shader_code = shaders[i].frag_shader_code;
				GLsizei frag_shader_count = shaders[i].frag_shader_count;
				GLchar** comp_shader_code = shaders[i].comp_shader_code;
				GLsizei comp_shader_count = shaders[i].comp_shader_count;

				GLint status;
				GLchar buffer[1024];

				GLuint vert_shader = 0;
				GLuint frag_shader = 0;
				GLuint comp_shader = 0;

				if (vert_shader_count != 0)
				{
					vert_shader = glCreateShader(GL_VERTEX_SHADER);
					glShaderSource(vert_shader, vert_shader_count, vert_shader_code, 0);
					glCompileShader(vert_shader);

					glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status);
					if (status == 0)
					{
						//// ERROR
						glGetShaderInfoLog(vert_shader, ARRAY_SIZE(buffer), 0, buffer);
						OutputDebugStringA("ERROR: Failed to compile vertex shader. OpenGL reports the following error message:\n");
						OutputDebugStringA(buffer);
						OutputDebugStringA("\n");
						failed_setup = true;
						break;
					}
				}

				if (frag_shader_count != 0)
				{
					frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
					glShaderSource(frag_shader, frag_shader_count, frag_shader_code, 0);
					glCompileShader(frag_shader);

					glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status);
					if (status == 0)
					{
						//// ERROR
						glGetShaderInfoLog(frag_shader, ARRAY_SIZE(buffer), 0, buffer);
						OutputDebugStringA("ERROR: Failed to compile fragment shader. OpenGL reports the following error message:\n");
						OutputDebugStringA(buffer);
						OutputDebugStringA("\n");
						failed_setup = true;
						break;
					}
				}

				if (comp_shader_count != 0)
				{
					comp_shader = glCreateShader(GL_COMPUTE_SHADER);
					glShaderSource(comp_shader, comp_shader_count, comp_shader_code, 0);
					glCompileShader(comp_shader);

					glGetShaderiv(comp_shader, GL_COMPILE_STATUS, &status);
					if (status == 0)
					{
						//// ERROR
						glGetShaderInfoLog(comp_shader, ARRAY_SIZE(buffer), 0, buffer);
						OutputDebugStringA("ERROR: Failed to compile compute shader. OpenGL reports the following error message:\n");
						OutputDebugStringA(buffer);
						OutputDebugStringA("\n");
						failed_setup = true;
						break;
					}
				}

				*program = glCreateProgram();
				if (vert_shader_code != 0) glAttachShader(*program, vert_shader);
				if (frag_shader_code != 0) glAttachShader(*program, frag_shader);
				if (comp_shader_code != 0) glAttachShader(*program, comp_shader);
				glLinkProgram(*program);

				glGetProgramiv(*program, GL_LINK_STATUS, &status);
				if (status == 0)
				{
					//// ERROR
					glGetProgramInfoLog(*program, ARRAY_SIZE(buffer), 0, buffer);
					OutputDebugStringA("ERROR: Failed to link shader program. OpenGL reports the following error message:\n");
					OutputDebugStringA(buffer);
					OutputDebugStringA("\n");
					failed_setup = true;
					break;
				}

				glValidateProgram(*program);
				glGetProgramiv(*program, GL_VALIDATE_STATUS, &status);
				if (status == 0)
				{
					//// ERROR
					glGetProgramInfoLog(*program, ARRAY_SIZE(buffer), 0, buffer);
					OutputDebugStringA("ERROR: Failed to validate shader program. OpenGL reports the following error message:\n");
					OutputDebugStringA(buffer);
					OutputDebugStringA("\n");
					failed_setup = true;
					break;
				}

				if (vert_shader_code != 0) glDeleteShader(vert_shader);
				if (frag_shader_code != 0) glDeleteShader(frag_shader);
				if (comp_shader_code != 0) glDeleteShader(comp_shader);
			}
		}

		{ /// Prepare tracing textures
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &display_texture);
			glBindTexture(GL_TEXTURE_2D, display_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);
			glBindImageTexture(0, display_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);

			glActiveTexture(GL_TEXTURE1);
			glGenTextures(1, &sum_texture);
			glBindTexture(GL_TEXTURE_2D, sum_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);
			glBindImageTexture(0, sum_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);

			glActiveTexture(GL_TEXTURE2);
			glGenTextures(1, &compensation_texture);
			glBindTexture(GL_TEXTURE_2D, compensation_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);
			glBindImageTexture(0, compensation_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
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
			u32 width  = (u32)(client_rect.right - client_rect.left);
			u32 height = (u32)(client_rect.bottom - client_rect.top);
			glViewport(0, 0, (GLsizei)width, (GLsizei)height);

			if (texture_width != width || texture_height != height)
			{
				texture_width  = width;
				texture_height = height;

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, display_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, sum_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);
				float f[4] = {0, 0, 0, 0};
				glClearTexImage(sum_texture, 0, GL_RGBA, GL_FLOAT, f);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, compensation_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_FLOAT, 0);
				glClearTexImage(compensation_texture, 0, GL_RGBA, GL_FLOAT, f);

				glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
				frame_index = 0;
			}

			glUseProgram(tracing_program);
			glBindImageTexture(0, sum_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindImageTexture(1, compensation_texture, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
			glUniform2f(0, (f32)width, (f32)height);
			glUniform1ui(1, frame_index);
			glDispatchCompute(width/32 + (width%32 != 0), height/32 + (height%32 != 0), 1); // TODO
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			glUseProgram(conversion_program);
			glBindImageTexture(0, sum_texture, 0, 0, 0, GL_READ_ONLY, GL_RGBA32F);
			glBindImageTexture(1, compensation_texture, 0, 0, 0, GL_READ_ONLY, GL_RGBA32F);
			glBindImageTexture(2, display_texture, 0, 0, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glUniform2f(0, (f32)width, (f32)height);
			glDispatchCompute(width/32 + (width%32 != 0), height/32 + (height%32 != 0), 1); // TODO
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			frame_index += 1;

			Sleep(10);

			glBindVertexArray(default_vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, display_texture);
			glUseProgram(disp_program);
			glUniform2f(0, (f32)width, (f32)height);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			SwapBuffers(dc);
		}
	}

	if (gl_context != 0) wglDeleteContext(gl_context);
	if (dc != 0)         ReleaseDC(window, dc);
	if (window != 0)     DestroyWindow(window);

	return 0;
}
