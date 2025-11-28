//
//  Shader.vsh
//  doctordonna
//
//  Created by Eric Herrmann on 8/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

varying lowp vec2 v_texcoord;

attribute lowp vec4 a_position;
attribute lowp vec2 a_texcoord;

uniform lowp mat4 u_projection;

void main()
{
    v_texcoord = a_texcoord.st;
    gl_Position = a_position * u_projection;
}