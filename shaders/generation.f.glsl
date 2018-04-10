/*
 *   Fragment Shader
 *
 *   CSCI 441, Computer Graphics, Colorado School of Mines
 */

#version 410 core

in vec3 ghalfwayVec;
in vec3 GLight;
in vec3 GNormal;
in vec3 GPosition;

noperspective in vec3 GEdgeDistance;

layout( location = 0 ) out vec4 FragColor;

uniform vec3 mDiff, mAmb, mSpec;
uniform float shininess;

uniform light {
    vec3 lAmb, lDiff, lSpec, lPos;
};

// The mesh line settings
uniform struct LineInfo {
    float Width;
    vec4 Color;
} Line;

vec3 phongModel( vec3 pos, vec3 norm ) {
    vec3 lightVec2 = normalize(GLight);
    vec3 normalVec2 = normalize(GNormal);
    vec3 halfwayVec2 = normalize(ghalfwayVec);

    if (!gl_FrontFacing)
        normalVec2 = -normalVec2;
    
    float sDotN = max( dot(lightVec2, normalVec2), 0.0 );
    vec4 diffuse = vec4(lDiff * mDiff * sDotN, 1);
    
    vec4 specular = vec4(0.0);
    if( sDotN > 0.0 ) {
        specular = vec4(lSpec * mSpec * pow( max( 0.0, dot( halfwayVec2, normalVec2 ) ), shininess ),1);
    }
    
    vec4 ambient = vec4(lAmb * mAmb, 1);
    
    vec3 fragColorOut = vec3(diffuse + specular + ambient);
    // vec4 fragColorOut = vec4(0.0,0.0,0.0,0.0);
    return fragColorOut;
}


void main() {
    //     /*****************************************/
    //     /******* Final Color Calculations ********/
    //     /*****************************************/

    // The shaded surface color.
    vec4 color=vec4(phongModel(GPosition, GNormal), 1.0);

    // Find the smallest distance
    float d = min( GEdgeDistance.x, GEdgeDistance.y );
    d = min( d, GEdgeDistance.z );

    // Determine the mix factor with the line color
    float mixVal = smoothstep( Line.Width - 1, Line.Width + 1, d );
    // float mixVal = 1;

    // Mix the surface color with the line color
    FragColor = vec4(mix( Line.Color, color, mixVal ));
    FragColor.a = 1;
}
