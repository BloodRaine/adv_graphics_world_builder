#version 330 core
#define SWITCH_TIME 	60.0		// seconds

in vec3 vertex;
out vec4 fragColorOut;

float t = 1.0/60.0;

uniform vec2 resolution;
uniform float r;
uniform float r2;
uniform int terrainType;

float function 			= mod(t,4.0);
bool  multiply_by_F1	= mod(t,8.0)  >= 4.0;
bool  inverse			= mod(t,16.0) >= 8.0;
float distance_type	= mod(t/16.0,4.0);

float random (vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

vec2 hash( vec2 p ){
	p = vec2( dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)));
	return fract(sin(p)*43758.5453);
}

float perlinNoise (vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

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

float billowedNoise(vec2 p)
{
    return abs(perlinNoise(p));
}

float ridgedNoise(vec2 p)
{
    return 1.0f-billowedNoise(p);
}

float fbm (vec2 st, int octaves) {
    // Initial values
    float value = 0.0;
    float amplitude = .5;
    float frequency = 1.0;
    float lac = 1.92;
    float gain = 0.5;
    vec2 dsum = vec2(0.0);
    //
    // Loop of octaves
    for (int i = 0; i < octaves; i++) {
        value += amplitude * ridgedNoise(st*frequency);
        st *= frequency;
        frequency *= lac; 
        amplitude *= gain;
    }
    return value;
}

float pattern (vec2 p, int octaves) {
    vec2 q = vec2( fbm( p + vec2( 0.0, 0.0 ) , octaves), 
                   fbm( p + vec2( 5.2, 1.3 ), octaves ) );
    vec2 r = vec2( fbm( 4.0 * q + vec2( 1.7,9.2) , octaves),
                   fbm( 4.0 * q + vec2(8.3, 2.8) , octaves) );

    return fbm(p + r, octaves);
}

float erosion(vec2 p, vec2 n, int octaves) {
    float T = 0.005;
    float h1 = pattern(p, octaves);
    float h2 = pattern(n, octaves);

    float diff = h2 - h1;

    if (diff > T) {
        if (h2 > h1) {
            h1 += 0.02;
        } else {
            h1 -= 0.04;
        }
    }
    return h1;
}

vec3 permute(vec3 x) {
    return mod((34.0 * x + 1.0) * x, 289.0);
  }

vec3 dist(vec3 x, vec3 y,  bool manhattanDistance) {
  return manhattanDistance ?  abs(x) + abs(y) :  (x * x + y * y);
}

float voronoise( vec2 x ){
	vec2 n = floor( x );
	vec2 f = fract( x );
	
	float F1 = 10.0;
	float F2 = 10.0;
	
	for( int j=-1; j<=1; j++ )
		for( int i=-1; i<=1; i++ ){
			vec2 g = vec2(i,j);
			vec2 o = hash( n + g );

			o = 0.5 + 0.41*sin( 0.5 + 6.2831*o );	
			vec2 r = g - f + o;

		float d = 	distance_type < 1.0 ? dot(r,r)  :				// euclidean^2
				  	distance_type < 2.0 ? sqrt(dot(r,r)) :			// euclidean
					distance_type < 3.0 ? abs(r.x) + abs(r.y) :		// manhattan
					distance_type < 4.0 ? max(abs(r.x), abs(r.y)) :	// chebyshev
					0.0;

		if( d<F1 ) { 
			F2 = F1; 
			F1 = d; 
		} else if( d<F2 ) {
			F2 = d;
		}
    }
	
	float c = function < 1.0 ? F1 : 
			  function < 2.0 ? F2 : 
			  function < 3.0 ? F2-F1 :
			  function < 4.0 ? (F1+F2)/2.0 : 
			  0.0;
		
	if( multiply_by_F1 )	c *= F1;
	if( inverse )			c = 1.0 - c;
	
    return c;
}

vec3 island(vec2 st, vec2 nxt, int octaves) {
    vec3 outNoise = vec3(0.0);

    float rad = 0.38;

    float loc = sqrt((st.x-0.5)*(st.x-0.5) + (st.y-0.5)*(st.y-0.5));

    outNoise += 0.6*voronoise(st/(1.0/((r*r2)*(r*r2))));

    if (loc <= rad) {
        outNoise.y += 0.7-1.2*loc;
        outNoise.y -= 0.1;
        float f1 = fbm(st*r, octaves);
        float f2 = fbm(st*r2, octaves);

        outNoise += 0.4*fbm(vec2(r*f1,f2), octaves);
        outNoise -= erosion(st, nxt, octaves);
        if (outNoise.y < 0.2) {
            outNoise.y = 0.2;
        }
    } else {
        if (outNoise.y < 0.5) {
            outNoise.y = 0.2;
        }
    }

    return outNoise; 
}

vec3 rugged(vec2 st, vec2 nxt, int octaves) {
    float c1 = 0.9 - voronoise(st/(1/(2*r*r2)));
    vec3 outNoise = vec3(0.6*r*c1);
    
    outNoise += 0.7*voronoi(st);
    outNoise -= 1.4*erosion(st*r, nxt*0.5*r2, octaves);
    if (outNoise.y <= 0.1) {
        outNoise.y = 0.0;
    } else {
        outNoise.y -= 0.05;
    }
    outNoise += 0.7*voronoise(st/(1/r));
    outNoise += 0.2*ridgedNoise(st);
    outNoise.y += 0.1;
    return outNoise;
}

vec3 mountain(vec2 st, vec2 nxt, int octaves) {
    vec3 outNoise = vec3(0.0);

    vec2 nv = gl_FragCoord.xy/((1/(r*r2))*resolution.xy);
    nv.x *= resolution.x/resolution.y;

    float c1 = voronoise(nv);

    outNoise += r*c1;
    outNoise += voronoi(st);

    outNoise -= erosion(st, nxt, octaves);

    if (outNoise.y < 0.05) {
        outNoise.y = 0.1*billowedNoise(nv);
    } else if (outNoise.y < 0.075) {
        outNoise.y += 0.05*billowedNoise(nv);
    }

    return outNoise;
}


vec3 canyon(vec2 st, vec2 nxt, int octaves) {

    vec2 nv = gl_FragCoord.xy/(1/(2*r*r2)*resolution.xy);
    nv.x *= resolution.x/resolution.y;

    vec3 outNoise = vec3(0.0);

    float f1 = fbm(st*r, octaves);
    float f2 = fbm(st*r2, octaves);

    outNoise += fbm(vec2(r*f1,f2), octaves);

    float f = 0.3*voronoise(nv);

    outNoise -= vec3(f);

    if (outNoise.y < 0.15) {
        outNoise.y = 0.6;
    } else if (outNoise.y < 0.225) {
        outNoise.y = 0.5;
    } else if (outNoise.y < 0.3) {
        outNoise.y = 0.4;
    }
    outNoise -= 0.2*erosion(st, nxt, octaves);

    return outNoise;
}


void main(void) {
    int octaves = 3;

    vec2 st = gl_FragCoord.xy/resolution.xy;
    st.x *= resolution.x/resolution.y;

    vec2 nxt = vec2(gl_FragCoord.x+0.01, gl_FragCoord.y);
    nxt.s *= resolution.x/resolution.y;

    vec3 outNoise = vec3(0.0);

    if (terrainType < 25) {
        outNoise += island(st, nxt, octaves);
    } else if (terrainType < 50) {
        outNoise += mountain(st, nxt, octaves);
    } else if (terrainType < 75) {
        outNoise += canyon(st, nxt, octaves);
    } else {
        outNoise += rugged(st, nxt, octaves);
    }
    
    fragColorOut = vec4(outNoise,1);
}