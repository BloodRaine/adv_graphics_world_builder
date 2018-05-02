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

    fragColorOut = vec4(texHeight, texHeight, texHeight, 1.0);
}
