#version 460 core
in vec2 xx;
out vec4 f;

uniform float time;

float hash1( float n )
{
    return fract(sin(n)*43758.5453123);
}


const float PI = 3.14;
const float PHI = 1.61;

vec3 forwardSF( float i, float n) 
{
    float phi = 2.0*PI*fract(i/PHI);
    float zi = 1.0 - (2.0*i+1.0)/n;
    float sinTheta = sqrt( 1.0 - zi*zi);
    return vec3( cos(phi)*sinTheta, sin(phi)*sinTheta, zi);
}

float almostIdentity( float x, float m, float n )
{
    if( x>m ) return x;
    float a = 2.0*n - m;
    float b = 2.0*m - 3.0*n;
    float t = x/m;
    return (a*t + b)*t*t + n;
}


vec2 map( vec3 q )
{
    q *= 100.0;

    vec2 res = vec2( q.y, 2.0 );


    float r = 15.0;
    q.y -= r;
    float ani = pow( 0.5+0.5*sin(6.28318*time + q.y/25.0), 4.0 );
    q *= 1.0 - 0.2*vec3(1.0,0.5,1.0)*ani;
    q.y -= 1.5*ani;
    float x = abs(q.x);

        
    float y = q.y;
    float z = q.z;
    y = 4.0 + y*1.2 - x*sqrt(max((20.0-x)/15.0,0.0));
    z *= 2.0 - y/15.0;
    float d = sqrt(x*x+y*y+z*z) - r;
    d = d/3.0;
    if( d<res.x ) res = vec2( d, 1.0 );
    
    res.x /= 100.0;
    //return vec2(max(res.x,time-80.0*q.y),res.y);
    return res;
}

vec2 intersect( in vec3 ro, in vec3 rd )
{
	const float maxd = 1.0;

    vec2 res = vec2(0.0);
    float t = 0.2;
    for( int i=0; i<300; i++ )
    {
	    vec2 h = map( ro+rd*t );
        if( (h.x<0.0) || (t>maxd) ) break;
        t += h.x;
        res = vec2( t, h.y );
    }

    if( t>maxd ) res=vec2(-1.0);
	return res;
}

vec3 calcNormal( in vec3 pos )
{
    vec3 eps = vec3(0.005,0.0,0.0);
	return normalize( vec3(
           map(pos+eps.xyy).x - map(pos-eps.xyy).x,
           map(pos+eps.yxy).x - map(pos-eps.yxy).x,
           map(pos+eps.yyx).x - map(pos-eps.yyx).x ) );
}


vec3 render( in vec2 p )
{
	float an = 6.2*time;
	vec3 ro = vec3(1.0*sin(an),0.26,1.0*cos(an));
    vec3 ta = vec3(0.0,0.16,0.0);
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	vec3 rd = normalize( p.x*uu + p.y*vv + 1.7*ww );

    
	vec3 col = vec3(0.2,0.0,0.1);

    vec3 uvw;
    vec2 res = intersect(ro,rd);
    float t = res.x;

    if( t>0.0 )
    {
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal(pos);
		vec3 ref = reflect( rd, nor );
        float fre = clamp( 1.0 + dot(nor,rd), 0.0, 1.0 );
        
        
        if( res.y<1.5 ) // heart
        {
            col = vec3(0.88,0.02,0.66);
            col = col*0.72 + 0.2*fre*vec3(1.0,0.8,0.2);
            
            vec3 lin = 0.8*fre*vec3(1.0,1.0,1.0)*(0.6);
            col = col * lin;

            col = pow(col,vec3(0.4545));
        }
        else{ // ground
            discard;
        }
    }else{ //background
        discard;
    }

    col = clamp(col,0.0,1.0);
	return col;
}

void main(){
    vec3 uvw= render(xx*2.2+1.8);
    f=vec4(uvw,0.9);
}