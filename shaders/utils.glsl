layout(binding = 0, rgba8) uniform image2D storageTexture;

layout (binding = 1) uniform ParameterUBO {
    float deltaTime;
    float totalTime;
    vec3 sunPosition;
    vec3 cameraPosition;
    vec3 windDirection;
    int frame;
    int particleBasedFluid;
    float rotationAngle;
} ubo;

struct Particle {
    vec4 position;
    vec3 velocity;
    vec4 color;
};

layout(std140, binding = 2) buffer ParticleSSBO {
    Particle particles[];
};

layout(binding = 3) uniform sampler2D noiseTexture;
layout(binding = 4) uniform sampler2D blueNoiseTexture;
layout (binding = 5, rgba8) uniform image2D causticTexture;

#define PI 3.14159265359

float sdSphere(vec3 p, float radius) {
    return length(p) - radius;
}

float BeersLaw (float dist, float absorption) {
    return exp(-dist * absorption);
}

float HenyeyGreenstein(float g, float mu) {
    float gg = g * g;
    return (1.0 / (4.0 * PI))  * ((1.0 - gg) / pow(1.0 + gg - 2.0 * g * mu, 1.5));
}

float sdRoundBox(vec3 p, vec3 b, float r)
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
}

float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sdPlane(vec3 p, vec3 n, float h)
{
    // n must be normalized
    return dot(p, n) + h;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float hash1(float n)
{
    return fract(n*17.0*fract(n*0.3183099));
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float noise(in vec3 x)
{
    vec3 p = floor(x);
    vec3 w = fract(x);

    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);

    float n = p.x + 317.0*p.y + 157.0*p.z;

    float a = hash1(n+0.0);
    float b = hash1(n+1.0);
    float c = hash1(n+317.0);
    float d = hash1(n+318.0);
    float e = hash1(n+157.0);
    float f = hash1(n+158.0);
    float g = hash1(n+474.0);
    float h = hash1(n+475.0);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}

// smooth minimum - https://iquilezles.org/articles/smin/
// returns: vec2 (sdf value, mix factor)
vec2 sminN( float a, float b, float k, float n )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    float m = pow(h, n)*0.5;
    float s = m*k/n; 
    return (a<b) ? vec2(a-s,m) : vec2(b-s,1.0-m);
}

float smoothVoronoi( in vec2 x )
{
    ivec2 p = ivec2(floor( x ));
    vec2  f = fract( x );

    float res = 0.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        ivec2 b = ivec2( i, j );
        vec2  r = vec2( b ) - f + noise( vec3(p + b, 0.0) );
        float d = length( r );

        res += exp( -32.0*d );
    }
    return -(1.0/32.0)*log( res );
}

const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );
                      
float fbm(vec3 p, in int iterations) {
    vec3 q = p + ubo.totalTime * 0.5 * ubo.windDirection;
    float g = noise(q);

    float f = 0.0;
    float scale = 0.5;
    float factor = 2.02;

    for (int i = 0; i < iterations; i++) {
        f += scale * noise(q);
        q *= factor;
        factor += 0.21;
        scale *= 0.5;
    }

    return f;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm_4( in vec3 x )
{
    return fbm(x, 4);
}

float hash21(vec2 uv) {
    return fract(hash1(uv.x * uv.y));
}

// Return smooth noise 
float smoothNoise(vec2 uv) {
    // Create 1D random value
    vec2 index = uv;
    vec2 localUV = fract(index); 
    vec2 cellID = floor(index); 
    
    localUV = localUV*localUV*(3.0 - 2.0 * localUV); // Hermite Curve
    
    // Get noise values for corners of each cell (bottom/top right + left, then mix it)
    float bl = hash21(cellID);
    float br = hash21(cellID + vec2(1, 0));
    float b = mix(bl, br, localUV.x);
    float tl = hash21(cellID + vec2(0, 1));
    float tr = hash21(cellID + vec2(1, 1));
    float t = mix(tl, tr, localUV.x);
    float noiseCol = mix(b, t, localUV.y);
        
    return noiseCol;
}

// Fractal Brownian Motion 
float fbm(vec2 pos, int iterations) {
    // Declare return value, amplitude of motion, freq of motion
    float val = 0.0;
    float amp = 0.05;
    float freq = 4.0;
    
    // Now loop through layers and return the combined value
    for (int i=0;i<iterations;i++) {
        val += amp * smoothNoise(freq * pos); 
        amp *= 0.5;
        freq * 2.0;
    }
    
    return val;
}

vec3 rotateVector(vec3 direction, vec3 axis, float angle) {
    // Normalize the axis
    axis = normalize(axis);
    
    // Create a quaternion representing the rotation
    float s = sin(angle * 0.5);
    vec4 quaternion = vec4(axis * s, cos(angle * 0.5));
    
    // Convert the quaternion to a rotation matrix
    mat3 rotationMatrix = mat3(
        1.0 - 2.0 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z),
        2.0 * (quaternion.x * quaternion.y - quaternion.z * quaternion.w),
        2.0 * (quaternion.x * quaternion.z + quaternion.y * quaternion.w),
        
        2.0 * (quaternion.x * quaternion.y + quaternion.z * quaternion.w),
        1.0 - 2.0 * (quaternion.x * quaternion.x + quaternion.z * quaternion.z),
        2.0 * (quaternion.y * quaternion.z - quaternion.x * quaternion.w),
        
        2.0 * (quaternion.x * quaternion.z - quaternion.y * quaternion.w),
        2.0 * (quaternion.y * quaternion.z + quaternion.x * quaternion.w),
        1.0 - 2.0 * (quaternion.x * quaternion.x + quaternion.y * quaternion.y)
    );
    
    // Rotate the direction vector
    return rotationMatrix * direction;
}
