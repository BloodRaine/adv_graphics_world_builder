/*
 *   Fragment Shader
 *
 *   CSCI 444, Adv. Computer Graphics, Colorado School of Mines
 */

#version 330 core

in float texHeight;

out vec4 fragColorOut;

void main() {

    //*****************************************
    //******* Final Color Calculations ********
    //*****************************************

    vec3 color = mix(vec3(1.,0.,0.), vec3(0.,1.,0.), texHeight);

    fragColorOut = vec4(color*1.5*texHeight, 1.0);
}
