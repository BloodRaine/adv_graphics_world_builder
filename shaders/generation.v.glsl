/*
 *   Vertex Shader
 *
 *   CSCI 444, Adv. Computer Graphics, Colorado School of Mines
 */

#version 330 core

in vec3 vPos;
in vec3 normal;
in vec2 texCoord;

out float texHeight;

uniform sampler2D textureMap;

uniform mat4 PMV;

void main() {
    //*****************************************
    //********* Vertex Calculations  **********
    //*****************************************
    vec4 texel = texture(textureMap, texCoord);
    int scale = 5;

    vec3 translation = vec3(vPos.x, vPos.y + scale * (texel.y-1), vPos.z);

    texHeight = texel.y;
    
    gl_Position = PMV * vec4(translation, 1.0);
}