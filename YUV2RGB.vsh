//
//  Shader.vsh
//  doctordonna
//
//  Created by Eric Herrmann on 8/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

varying highp vec2 v_texcoord;

attribute highp vec4 a_position;
attribute highp vec2 a_texcoord;

uniform highp mat4 u_projection;

void main()
{
    v_texcoord = a_texcoord.st;
    gl_Position = a_position * u_projection;
}