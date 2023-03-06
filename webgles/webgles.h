//===-- webgles.h - OpenGL ES implementation over WebGL --------------===//
//
//                     Cheerp: The C++ compiler for the Web
//
// This file is distributed under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE.TXT for details.
//
// Copyright 2018-2023 Leaning Technologies
//
//===----------------------------------------------------------------------===//

#ifndef _WEBGLES_H_
#define _WEBGLES_H_

#include <cheerp/client.h>
#include <cheerp/webgl.h>
#include "gl2.h"
namespace [[cheerp::genericjs]] client
{
	extern client::WebGLRenderingContext* webGLES;
}
using client::webGLES;
[[cheerp::genericjs]] extern client::OESVertexArrayObject* webGLESExtVAO;
enum WEBGLES_OPTIONS {
	WG_NO_ALPHA = 1,
	WG_NO_DEPTH = 2,
	WG_STENCIL = 4,
	WG_PREMULTIPLIED_ALPHA = 8,
	WG_NO_ANTIALIAS = 16,
};
[[cheerp::genericjs]] void webGLESInit(const client::String& canvasName, int options = 0);
[[cheerp::genericjs]] void webGLESInit(client::HTMLCanvasElement* canvas, int options = 0);
[[cheerp::genericjs]] bool webGLESInitExtVAO();
[[cheerp::genericjs]] client::WebGLProgram* webGLESLookupWebGLProgram(int objId);
[[cheerp::genericjs]] client::WebGLShader* webGLESLookupWebGLShader(int objId);
[[cheerp::genericjs]] client::WebGLBuffer* webGLESLookupWebGLBuffer(int objId);
[[cheerp::genericjs]] client::WebGLFramebuffer* webGLESLookupWebGLFramebuffer(int objId);
[[cheerp::genericjs]] client::WebGLRenderbuffer* webGLESLookupWebGLRenderbuffer(int objId);
[[cheerp::genericjs]] client::WebGLTexture* webGLESLookupWebGLTexture(int objId);
[[cheerp::genericjs]] client::WebGLUniformLocation* webGLESLookupWebGLUniformLocation(int objId);
[[cheerp::genericjs]] client::WebGLVertexArrayOES* webGLESLookupWebGLVertexArrayOES(int objId);
[[cheerp::genericjs]] void webGLESLookupArrayInit();
#endif
