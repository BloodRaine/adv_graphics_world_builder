/*
 *   Vertex Shader
 *
 *   CSCI 441, Computer Graphics, Colorado School of Mines
 */

#version 410 core

// varying inputs
in vec3 vPos;
out vec3 vertex;

uniform mat4 mvp;

void main() {
    /*****************************************/
    /********* Vertex Calculations  **********/
    /*****************************************/

    vertex = vec3(vec4(vPos, 1.0));
    gl_Position = vec4(vPos, 1.0); //mvp *
}