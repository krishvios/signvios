//
//  Shader.fsh
//  doctordonna
//
//  Created by Eric Herrmann on 8/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

varying highp vec2 v_texcoord;

uniform sampler2D u_ytexture;
uniform sampler2D u_utexture;
uniform sampler2D u_vtexture;
uniform highp mat3 u_yuv2rgb;

void main()
{
    highp float y = texture2D(u_ytexture, v_texcoord).r;
    highp float u = texture2D(u_utexture, v_texcoord).r;
    highp float v = texture2D(u_vtexture, v_texcoord).r;

    highp vec3 rgb = u_yuv2rgb * vec3(y - 0.0625, u - 0.5, v - 0.5);
    gl_FragColor = vec4(rgb, 1.0);
}