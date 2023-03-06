//===-- webglesutils.cpp - OpenGL ES implementation over WebGL --------------===//
//
//                     Cheerp: The C++ compiler for the Web
//
// This file is distributed under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE.TXT for details.
//
// Copyright 2018-2023 Leaning Technologies
//
//===----------------------------------------------------------------------===//

#include "webgles.h"
#include <cheerpintrin.h>

namespace [[cheerp::genericjs]] client
{
	client::WebGLRenderingContext* webGLES = nullptr;
}
client::OESVertexArrayObject* webGLESExtVAO;

extern client::Array* WebGLBufferArray;
extern client::Array* WebGLRenderbufferArray;
extern client::Array* WebGLShaderArray;
extern client::Array* WebGLTextureArray;

void webGLESInit(const client::String& canvasName, int options)
{
	client::HTMLCanvasElement* canvas = static_cast<client::HTMLCanvasElement*>(client::document.getElementById(canvasName));
	return webGLESInit(canvas, options);
}

void webGLESInit(client::HTMLCanvasElement* canvas, int options)
{
	webGLESLookupArrayInit();
	client::WebGLContextAttributes* optObj = new client::WebGLContextAttributes();
	if(options & WG_NO_ALPHA)
		optObj->set_alpha(0);
	if(options & WG_NO_DEPTH)
		optObj->set_depth(0);
	if(options & WG_NO_ANTIALIAS)
		optObj->set_antialias(0);
	if(options & WG_STENCIL)
		optObj->set_stencil(1);
	if (options & WG_PREMULTIPLIED_ALPHA)
		optObj->set_premultipliedAlpha(1);
	else
		optObj->set_premultipliedAlpha(0);
	webGLES = static_cast<client::WebGLRenderingContext*>(canvas->getContext("experimental-webgl", optObj));
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

void glGetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
	if(pname == GL_INFO_LOG_LENGTH)
	{
		client::String* info = webGLES->getShaderInfoLog(webGLESLookupWebGLShader(shader));
		params[0] = info == nullptr ? 0 : info->get_length() + 1;
	}
	else if(pname == GL_SHADER_SOURCE_LENGTH)
	{
		client::String* info = webGLES->getShaderSource(webGLESLookupWebGLShader(shader));
		params[0] = info == nullptr ? 0 : info->get_length() + 1;
	}
	else
		params[0] = (double)*webGLES->getShaderParameter(webGLESLookupWebGLShader(shader), pname);
}

void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog)
{
	client::String* info = webGLES->getShaderInfoLog(webGLESLookupWebGLShader(shader));
	if(info == nullptr)
	{
		if(length)
			*length = 0;
		infoLog[0] = 0;
		return;
	}
	int strLen = info->get_length() + 1;
	if(strLen > maxLength)
		strLen = maxLength;
	for(int i=0;i<strLen - 1;i++)
		infoLog[i] = info->charCodeAt(i);
	infoLog[strLen - 1] = 0;
	if(length)
		*length = strLen - 1;
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
	if(pname == GL_ACTIVE_UNIFORM_MAX_LENGTH || pname == GL_ACTIVE_ATTRIBUTE_MAX_LENGTH)
		params[0] = 256;
	else if(pname == GL_INFO_LOG_LENGTH)
	{
		client::String* info = webGLES->getProgramInfoLog(webGLESLookupWebGLProgram(program));
		params[0] = info == nullptr ? 0 : info->get_length() + 1;
	}
	else
		params[0] = (double)*webGLES->getProgramParameter(webGLESLookupWebGLProgram(program), pname);
}

void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog)
{
	client::String* info = webGLES->getProgramInfoLog(webGLESLookupWebGLProgram(program));
	if(info == nullptr)
	{
		if(length)
			*length = 0;
		infoLog[0] = 0;
		return;
	}
	int strLen = info->get_length() + 1;
	if(strLen > maxLength)
		strLen = maxLength;
	for(int i=0;i<strLen - 1;i++)
		infoLog[i] = info->charCodeAt(i);
	infoLog[strLen - 1] = 0;
	if(length)
		*length = strLen - 1;
}

void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	client::WebGLActiveInfo* info = webGLES->getActiveAttrib(webGLESLookupWebGLProgram(program), index);
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

void glGetAttachedShaders(GLuint program, GLsizei max, GLsizei* count, GLuint* shaders)
{
	auto shadersArray = webGLES->getAttachedShaders(webGLESLookupWebGLProgram(program));
	int realMax = max > shadersArray->get_length() ? shadersArray->get_length() : max;
	for(int i=0;i<realMax;i++)
	{
		int index=WebGLShaderArray->indexOf((*shadersArray)[i]);
		shaders[i] = index+1;
	}
	if(count)
		*count = realMax;
}

void glGetBufferParameteriv(GLenum target, GLenum value, GLint* data)
{
	data[0] = (int)*webGLES->getBufferParameter(target, value);
}

void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* data)
{
	client::Object* ret = webGLES->getFramebufferAttachmentParameter(target, attachment, pname);
	if(pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME)
	{
		int index = WebGLTextureArray->indexOf(ret);
		if(index < 0)
			index = WebGLRenderbufferArray->indexOf(ret);
		data[0] = index+1;
	}
	else
		data[0] = (int)*ret;
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* data)
{
	data[0] = (int)*webGLES->getRenderbufferParameter(target, pname);
}

void glGetShaderPrecisionFormat(GLenum sType, GLenum pType, GLint* range, GLint* prec)
{
	client::WebGLShaderPrecisionFormat* format = webGLES->getShaderPrecisionFormat(sType, pType);
	range[0] = format->get_rangeMin();
	range[1] = format->get_rangeMax();
	prec[0] = format->get_precision();
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint* data)
{
	data[0] = (int)*webGLES->getTexParameter(target, pname);
}

void glGetUniformfv(GLuint program, GLint location, GLfloat* data)
{
	client::Object* ret = webGLES->getUniform(webGLESLookupWebGLProgram(program), webGLESLookupWebGLUniformLocation(location));
	// We assume the user knows that this uniform is either a float or an array or float
	if(ret->hasOwnProperty("0"))
	{
		// Array or typed array
		client::Array* a = (client::Array*)ret;
		for(int i=0;i<a->get_length();i++)
			data[i] = (double)*((*a)[i]);
	}
	else
	{
		// Value
		data[0] = (double)*ret;
	}
}

void glGetUniformiv(GLuint program, GLint location, GLint* data)
{
	client::Object* ret = webGLES->getUniform(webGLESLookupWebGLProgram(program), webGLESLookupWebGLUniformLocation(location));
	// We assume the user knows that this uniform is either a float or an array or float
	if(ret->hasOwnProperty("0"))
	{
		// Array or typed array
		client::Array* a = (client::Array*)ret;
		for(int i=0;i<a->get_length();i++)
			data[i] = (int)*((*a)[i]);
	}
	else
	{
		// Value
		data[0] = (int)*ret;
	}
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* data)
{
	// We only expect to be here for GL_CURRENT_VERTEX_ATTRIB
	client::Float32Array* ret = (client::Float32Array*)webGLES->getVertexAttrib(index, pname);
	for(int i=0;i<4;i++)
		data[i] = (*ret)[i];
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* data)
{
	client::Object* ret = webGLES->getVertexAttrib(index, pname);
	if(pname == GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING)
		data[0] = WebGLBufferArray->indexOf(ret) + 1;
	else
		data[0] = (int)*ret;
}
