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
extern client::WebGLRenderingContext* webGLES;
extern client::OESVertexArrayObject* webGLESExtVAO;
void webGLESInit(const client::String& canvasName);
bool webGLESInitExtVAO();
client::WebGLProgram* webGLESLookupWebGLProgram(int objId);
client::WebGLShader* webGLESLookupWebGLShader(int objId);
client::WebGLBuffer* webGLESLookupWebGLBuffer(int objId);
client::WebGLFramebuffer* webGLESLookupWebGLFramebuffer(int objId);
client::WebGLRenderbuffer* webGLESLookupWebGLRenderbuffer(int objId);
client::WebGLTexture* webGLESLookupWebGLTexture(int objId);
client::WebGLUniformLocation* webGLESLookupWebGLUniformLocation(int objId);
client::WebGLVertexArrayOES* webGLESLookupWebGLVertexArrayOES(int objId);
void webGLESLookupArrayInit();
#endif