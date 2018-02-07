/****************************************************************
 *
 * Copyright (C) 2013 Alessandro Pignotti <alessandro@leaningtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ***************************************************************/

#include "webgles.h"
#include <cheerpintrin.h>

namespace [[cheerp::genericjs]] client
{
	client::WebGLRenderingContext* webGLES = nullptr;
}
client::OESVertexArrayObject* webGLESExtVAO;

void webGLESInit(const client::String& canvasName)
{
	webGLESLookupArrayInit();
	auto canvas = static_cast<client::HTMLCanvasElement*>(client::document.getElementById(canvasName));
	webGLES = static_cast<client::WebGLRenderingContext*>(canvas->getContext("experimental-webgl"));
	if (webGLES == NULL)
		client::console.log("Sorry, we looked hard, but no sign of WebGL has been found :(");
}

bool webGLESInitExtVAO()
{
	webGLESExtVAO = static_cast<client::OESVertexArrayObject*>(webGLES->getExtension("OES_vertex_array_object"));
	return webGLESExtVAO != nullptr;
}

void webGLESShaderSource(GLuint shader, const char* code)
{
	client::WebGLShader* s = webGLESLookupWebGLShader(shader);
	webGLES->shaderSource(s, code);
}

#define shaderSourceImpl() { \
	uint32_t fullLen = 0; \
	for(GLsizei i=0;i<count;i++) \
		fullLen += length[i]; \
	char* buf = (char*)malloc(fullLen+1); \
	buf[fullLen] = 0; \
	uint32_t offset = 0; \
	for(GLsizei i=0;i<count;i++) \
	{ \
		memcpy(buf + offset, string[i], length[i]); \
		offset += length[i]; \
	} \
	webGLESShaderSource(shader, buf); \
	free(buf); \
}

[[cheerp::wasm]] void wgShaderSourceWasm (GLuint shader, GLsizei count, const char* const *string, const GLint* length) shaderSourceImpl();

[[cheerp::genericjs]] void wgShaderSourceGenericjs (GLuint shader, GLsizei count, const char* const *string, const GLint* length) shaderSourceImpl();

void glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
	if(pname == GL_ACTIVE_UNIFORMS)
		params[0] = *webGLES->getProgramParameter(webGLESLookupWebGLProgram(program), pname);
	else if(pname == GL_ACTIVE_UNIFORM_MAX_LENGTH)
		params[0] = 256;
	else
		params[0] = -1;
}

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	client::WebGLActiveInfo* info = webGLES->getActiveUniform(webGLESLookupWebGLProgram(program), index);
	// TODO: bufSize check
	*length = info->get_name()->get_length();
	for(GLsizei i=0;i<*length;i++)
	{
		name[i] = info->get_name()->charCodeAt(i);
	}
	name[*length] = 0;
	*size = info->get_size();
	*type = info->get_type();
}
