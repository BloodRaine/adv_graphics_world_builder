/*
 *   Vertex Shader
 *
 *   CSCI 441, Computer Graphics, Colorado School of Mines
 */

#version 410 core

// varying inputs
in vec3 vPos;

void main() {
    /*****************************************/
    /********* Vertex Calculations  **********/
    /*****************************************/

    gl_Position = vec4(vPos, 1.0);
}
