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
