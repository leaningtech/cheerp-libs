/****************************************************************
 *
 * Copyright (C) 2013-2017 Alessandro Pignotti <alessandro@leaningtech.com>
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
	WG_PREMULTIPLIED_ALPHA = 8
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
