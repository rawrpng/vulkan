#version 460 core

layout (location=0) in vec3 pos;
layout (location=1) in vec2 tex;
layout (location=2) in uvec4 joi;
layout (location=3) in vec4 wei;
layout (location=4) in vec3 normal0;
layout (location=5) in vec4 color;


//uniform mat4 jointmats[256];

layout (location=0) out vec4 colorz;
layout (location=1) out vec2 otex;
layout(push_constant) uniform Push{
      float outline0;
      float time;
      mat2x4 jointdualquats[256];
      mat4 world;
      mat4 view;
      mat4 proj;
}push;







mat2x4 getjointtransform(uvec4 jj,vec4 ww){
      mat2x4 dq0 = push.jointdualquats[jj.x];
      mat2x4 dq1 = push.jointdualquats[jj.y];
      mat2x4 dq2 = push.jointdualquats[jj.z];
      mat2x4 dq3 = push.jointdualquats[jj.w];
      ww.y *=sign(dot(dq0[0],dq1[0]));
      ww.z *=sign(dot(dq0[0],dq2[0]));
      ww.w *=sign(dot(dq0[0],dq3[0]));
      mat2x4 res = ww.x*dq0+ww.y*dq1+ww.z*dq2+ww.w*dq3;
      float norm = length(res[0]);
      return res/norm;
}

//uniform mat4 jointspace[256];

mat4 getskinmat(){
 mat2x4 bone = getjointtransform(joi,wei);
 vec4 r=bone[0];
 vec4 t=bone[1];
 return mat4(
      1.0 - (2.0 * r.y * r.y) - (2.0 * r.z * r.z),
            (2.0 * r.x * r.y) + (2.0 * r.w * r.z),
            (2.0 * r.x * r.z) - (2.0 * r.w * r.y),
      0.0,

            (2.0 * r.x * r.y) - (2.0 * r.w * r.z),
      1.0 - (2.0 * r.x * r.x) - (2.0 * r.z * r.z),
            (2.0 * r.y * r.z) + (2.0 * r.w * r.x),
      0.0,

            (2.0 * r.x * r.z) + (2.0 * r.w * r.y),
            (2.0 * r.y * r.z) - (2.0 * r.w * r.x),
      1.0 - (2.0 * r.x * r.x) - (2.0 * r.y * r.y),
      0.0,

      2.0 * (-t.w * r.x + t.x * r.w - t.y * r.z + t.z * r.y),
      2.0 * (-t.w * r.y + t.x * r.z + t.y * r.w - t.z * r.x),
      2.0 * (-t.w * r.z - t.x * r.y + t.y * r.x + t.z * r.w),
      1);
}

void main(){
	mat4 skinmat = getskinmat();
	//mat4 skinmat = wei.x * jointmats[joi.x] + wei.y * jointmats[joi.y] + wei.z * jointmats[joi.z] + wei.w * jointmats[joi.w];
	//mat4 skintime = wei.x * jointspace[int(joi.x)] + wei.y * jointspace[int(joi.y)] + wei.z * jointspace[int(joi.z)] + wei.w * jointspace[int(joi.w)];
	gl_Position=push.proj*push.view*push.world*skinmat*vec4(pos+normal0*uniformz.outline0,1.0);
	colorz = color;
	otex=tex;
}