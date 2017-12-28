/*
 * Copyright (c) 2010 VMware, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "piglit-util-gl.h"
#include "piglit-glx-util.h"

static PFNGLXCREATECONTEXTATTRIBSARBPROC
__piglit_glXCreateContextAttribsARB = NULL;
#define glXCreateContextAttribsARB(dpy, config, share, direct, attrib)	\
	(*__piglit_glXCreateContextAttribsARB)(dpy, config, share, direct, \
					       attrib)

int piglit_width = 50, piglit_height = 50;

static const char *TestName = "glx-buffer-sharing";

static Display *dpy;
static Window win;
static XVisualInfo *visinfo;

static const char *vert_shader_text =
	"#version 150 \n"
	"#extension GL_ARB_shading_language_420pack: require \n"
	"#extension GL_ARB_uniform_buffer_object: require \n"
	"in vec4 piglit_vertex; \n"
	"void main() \n"
	"{ \n"
	"   gl_Position = piglit_vertex; \n "
	"} \n";

static const char *frag_shader_text =
	"#version 150 \n"
	"#extension GL_ARB_shading_language_420pack: require \n"
	"#extension GL_ARB_uniform_buffer_object: require \n"
	"layout(binding = 0) uniform globals { \n"
	"   vec3 color; \n"
	"}; \n"
	"out vec4 f_out; \n"
	"void main() \n"
	"{ \n"
	"   f_out = vec4(color, 1); \n"
	"} \n";


static GLuint vert_shader, frag_shader;

static GLuint program;

static void
check_error(int line)
{
	GLenum e = glGetError();
	if (e)
		printf("GL Error 0x%x at line %d\n", e, line);
}

static void GLAPIENTRY debug_callback(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      GLvoid* userParam)
{
	printf("Callback: ");
	fwrite(message, length, 1, stdout);
	printf("\n");
}

GLXContext
create_context(GLXContext share_ctx)
{
	GLXContext ctx;
	int ctx_attribs[7] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 2,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
		0
	};

	GLXFBConfig config =
		piglit_glx_get_fbconfig_for_visinfo(dpy, visinfo);

	ctx = glXCreateContextAttribsARB(dpy, config,
					 share_ctx, True,
					 ctx_attribs);

	assert(ctx);

	return ctx;
}

enum piglit_result
draw(Display *dpy)
{
	const GLfloat red[3] = {1.0F, 0.0F, 0.0F};
	const GLfloat green[3] = {0.0F, 1.0F, 0.0F};
	const GLfloat blue[3] = {0.0F, 0.0F, 1.0F};

	GLXContext ctx1 = create_context(NULL);
	GLXContext ctx2 = create_context(ctx1);
	int ok;

	GLuint ub;

	if (!ctx1 || !ctx2) {
		fprintf(stderr, "%s: create contexts failed\n", TestName);
		piglit_report_result(PIGLIT_FAIL);
	}

	glXMakeCurrent(dpy, win, ctx1);

	piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
	glDebugMessageCallbackARB(debug_callback, 0);

	if (piglit_get_gl_version() < 32) {
		printf("%s: Requires OpenGL 3.2\n", TestName);
		return PIGLIT_SKIP;
	}

	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	vert_shader = piglit_compile_shader_text(GL_VERTEX_SHADER, vert_shader_text);
	frag_shader = piglit_compile_shader_text(GL_FRAGMENT_SHADER, frag_shader_text);
	program = piglit_link_simple_program(vert_shader, frag_shader);
	check_error(__LINE__);
	if (!program) {
		piglit_report_result(PIGLIT_FAIL);
	}

	glUseProgram(program);
	check_error(__LINE__);

	glGenBuffers(1, &ub);
	glBindBuffer(GL_UNIFORM_BUFFER, ub);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(red), red, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ub);

	piglit_draw_rect(-0.5, -0.5, 1, 1);
	check_error(__LINE__);

	ok = piglit_probe_pixel_rgb(piglit_width / 2, piglit_height / 2, red);

	glXSwapBuffers(dpy, win);

	if (!ok) {
		printf("%s: drawing with context 1 failed\n", TestName);
		return PIGLIT_FAIL;
	}

	check_error(__LINE__);
	glXMakeCurrent(dpy, win, ctx2);
	glDebugMessageCallbackARB(debug_callback, 0);

	check_error(__LINE__);
	assert(program);

	glUseProgram(program);
	check_error(__LINE__);

	glBindBuffer(GL_UNIFORM_BUFFER, ub);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(green), green);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ub);

	glClearColor(0.2, 0.2, 0.2, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	piglit_draw_rect(-0.5, -0.5, 1, 1);
	check_error(__LINE__);

	ok = piglit_probe_pixel_rgb(piglit_width / 2, piglit_height / 2, green);

	glXSwapBuffers(dpy, win);

	if (!ok) {
		printf("%s: drawing with context 2 failed\n", TestName);
		return PIGLIT_FAIL;
	}

	check_error(__LINE__);
	glXMakeCurrent(dpy, win, ctx1);

	glClearColor(0.3, 0.3, 0.3, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	piglit_draw_rect(-0.5, -0.5, 1, 1);

	ok = piglit_probe_pixel_rgb(piglit_width / 2, piglit_height / 2, green);

	glXSwapBuffers(dpy, win);

	if (!ok) {
		printf("%s: drawing with context 1 failed\n", TestName);
		return PIGLIT_FAIL;
	}

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(blue), blue);

	glClearColor(0.4, 0.4, 0.4, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	piglit_draw_rect(-0.5, -0.5, 1, 1);

	check_error(__LINE__);

	ok = piglit_probe_pixel_rgb(piglit_width / 2, piglit_height / 2, blue);

	glXSwapBuffers(dpy, win);

	if (!ok) {
		printf("%s: drawing with context 1 failed\n", TestName);
		return PIGLIT_FAIL;
	}

	check_error(__LINE__);
	glXDestroyContext(dpy, ctx1);
	glXDestroyContext(dpy, ctx2);

	return PIGLIT_PASS;
}


int
main(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-auto") == 0)
			piglit_automatic = 1;
		else
			fprintf(stderr, "%s bad option: %s\n", TestName, argv[i]);
	}

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "%s: open display failed\n", TestName);
		piglit_report_result(PIGLIT_FAIL);
	}

	piglit_require_glx_version(dpy, 1, 4);
	piglit_require_glx_extension(dpy, "GLX_ARB_create_context");

	__piglit_glXCreateContextAttribsARB =
		(PFNGLXCREATECONTEXTATTRIBSARBPROC)
		glXGetProcAddress((const GLubyte *)
				  "glXCreateContextAttribsARB");
	assert(__piglit_glXCreateContextAttribsARB != NULL);

	visinfo = piglit_get_glx_visual(dpy);
	win = piglit_get_glx_window(dpy, visinfo);

	XMapWindow(dpy, win);

	piglit_glx_event_loop(dpy, draw);

	return 0;
}
