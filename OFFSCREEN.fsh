//
//  Shader.fsh
//  doctordonna
//
//  Created by Eric Herrmann on 8/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

varying lowp vec2 v_texcoord;
uniform sampler2D u_ytexture;
uniform sampler2D u_uvtexture;

void main()
{
    lowp float y = texture2D(u_ytexture, v_texcoord).r;
    lowp vec2 uv = texture2D(u_uvtexture, v_texcoord).ar;
    gl_FragColor = vec4(y, uv, 1.0);
}