#version 330 core
#define NUM_OCTAVES 16

in vec3 vertex;
out vec4 fragColorOut;

uniform vec2 resolution;

float random (vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

float hash(vec3 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec3(0.71,0.113,0.419));
    return -1.0+2.0*fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
}

float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

vec3 voronoi(vec2 st) {

    vec3 color = vec3(.0);

    // Cell positions
    vec2 point[8];
    point[0] = vec2(0.83,0.75);
    point[1] = vec2(0.60,0.07);
    point[2] = vec2(0.28,0.64);
    point[3] =  vec2(0.31,0.26);
    point[4] =  vec2(0.1,0.16);
    point[5] =  vec2(0.2,0.08);
    point[6] = vec2(0.055, 0.2);
    point[7] =  vec2(0.93, 0.95);
    
    float m_dist = 1.;  // minimun distance
    vec2 m_point;        // minimum position

    // Iterate through the points positions
    for (int i = 0; i < 8; i++) {
        float dist = distance(st, point[i]);
        if ( dist < m_dist ) {
            // Keep the closer distance
            m_dist = dist;

            // Kepp the position of the closer point
            m_point = point[i];
        }
    }

    // Add distance field to closest point center
    color += m_dist*2.;
    return color;
}

float fbm ( vec2 _st ) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = rot * _st * 1.5 + shift;
        a *= 0.5;
    }
    return v;
}

float pattern (vec2 p) {
    vec2 q = vec2( fbm( p + vec2( 0.0, 0.0 ) ), 
                   fbm( p + vec2( 5.2, 1.3 ) ) );
    vec2 r = vec2( fbm( 4.0 * q + vec2( 1.7,9.2) ),
                   fbm( 4.0 * q + vec2(8.3, 2.8) ) );

    return fbm(p + 4.0*r);
}

// returns 3D value noise and its 3 derivatives
// vec4 noised( in vec3 x )
// {
//     vec3 p = floor(x);
//     vec3 w = fract(x);
    
//     vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
//     vec3 du = 30.0*w*w*(w*(w-2.0)+1.0);

//     float a = hash( p+vec3(0,0,0) );
//     float b = hash( p+vec3(1,0,0) );
//     float c = hash( p+vec3(0,1,0) );
//     float d = hash( p+vec3(1,1,0) );
//     float e = hash( p+vec3(0,0,1) );
//     float f = hash( p+vec3(1,0,1) );
//     float g = hash( p+vec3(0,1,1) );
//     float h = hash( p+vec3(1,1,1) );

//     float k0 =   a;
//     float k1 =   b - a;
//     float k2 =   c - a;
//     float k3 =   e - a;
//     float k4 =   a - b - c + d;
//     float k5 =   a - c - e + g;
//     float k6 =   a - b - e + f;
//     float k7 = - a + b + c - d + e - f - g + h;

//     return vec4( -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z), 
//                       2.0* du * vec3( k1 + k4*u.y + k6*u.z + k7*u.y*u.z,
//                                       k2 + k5*u.z + k4*u.x + k7*u.z*u.x,
//                                       k3 + k6*u.x + k5*u.y + k7*u.x*u.y ) );
// }

 // returns 3D fbm and its 3 derivatives
// vec4 fbm( in vec3 x, int octaves ) 
// {
//     float f = 1.98;  // could be 2.0
//     float s = 0.49;  // could be 0.5
//     float a = 0.0;
//     float b = 0.5;
//     vec3  d = vec3(0.0);
//     mat3  m = mat3(1.0,0.0,0.0,
//                    0.0,1.0,0.0,
//                    0.0,0.0,1.0);
//     for( int i=0; i < octaves; i++ )
//     {
//         vec4 n = noised(x);
//         a += b*n.x;          // accumulate values		
//         d += b*m*n.yzw;      // accumulate derivatives
//         b *= s;
//         x = f*m3*x;
//         m = f*m3i*m;
//     }
//     return vec4( a, d );
// }


// float uberNoise (vec3 lPosition, int octaves, float perterb, float sharpness, float ampFeatures, 
//                  float altErosion, float ridgeErosion, flost slopeErosion, float lacunarity, float gain) {

//     // amplify features
//     float lfCurrentGain  = gain + ampFeatures;

//     // Sharpness
//     float lfRidgeNoise = (1.0f - abs(featureNoise));
//     float billowNoise = featureNoise * featureNoise;

//     featureNoise = lerp(featureNoise, billowNoise, max(0.0,sharpness));
//     featureNoise = lerp(featureNoise, lfRidgeNoise, abs(mine(0.0,sharpness)));

//     // slope erosion
//     slopeErosionDerivativeSum = lDerivative + slopeErosion;

//     lfsum += lfAmplitude, * featureNoise* (1.0f / (1.0 + dot(slopeErosionDerivativeSum, slopeErosionDerivativeSum)));

//     // Amplitude Damping
//     lfsum += dampedAmplitude * featureNoise * (1.0 / (1.0 + dot(slopeErosionDerivativeSum, slopeErosionDerivativeSum)));
//     lfAmplitude *= lerp(currentGain, currentGain * smoothstep(0.0f, 1.0f, lfsum), altErosion);
//     lfDampedAmplitude = Amplitude * (1.0 - (ridgeErosion / (1.0 + dot(ridgeErosionDerivativeSum, ridgeErosionDerivativeSum))));

//     // domain perturb
//     lOctavePosition = (lPosition * lFrequency) + perterbDerivativeSum;
//     perterbDerivativeSum += lDerivative * perturbFeatures;

// }





void main(void) {
    int octaves = 32;
    float persistence = 0.7;

    vec2 st = gl_FragCoord.xy/resolution.xy;
    st.x *= resolution.x/resolution.y;

    vec3 color = voronoi(st);

    // vec2 q = vec2(0.);
    // q.x = fbm( st + 0.00*2, octaves);
    // q.y = fbm( st + vec2(1.0), octaves);

    // vec2 r = vec2(0.0);
    // r.x = fbm( st + 1.3*q + vec2(1.7,9.2)+ 0.15*1.5 , octaves);
    // r.y = fbm( st + 1.5*q + vec2(8.3,2.8)+ 0.126*1.5, octaves);

    // float f = fbm(st+r, octaves);

    

    float o = pattern(st);
    vec3 fragColor = 1.5* color * o;
    
    fragColorOut = vec4(fragColor,1);
}