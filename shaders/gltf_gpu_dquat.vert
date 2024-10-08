#version 460
#extension GL_EXT_shader_explicit_arithmetic_types:enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in u8vec4 aJointNum;
//layout (location = 3) in u16vec4 aJointNum;
//layout (location = 3) in uvec4 aJointNum;
layout (location = 4) in vec4 aJointWeight;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec4 newcolor;
layout (location = 3) out uint txidx;



layout (push_constant) uniform Constants {
  int aModelStride;
  uint txid;
  float t;
  bool decaying;
};

layout (set = 1, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

layout (std430, set = 3, binding = 0) readonly buffer JointDualQuats {
  mat2x4 jointDQs[];
};

mat2x4 getJointTransform(u8vec4 joints, vec4 weights) {
  // read dual quaterions from buffer
  mat2x4 dq0 = jointDQs[aJointNum.x  + gl_InstanceIndex * aModelStride];
  mat2x4 dq1 = jointDQs[aJointNum.y+ gl_InstanceIndex * aModelStride];
  mat2x4 dq2 = jointDQs[aJointNum.w + gl_InstanceIndex * aModelStride];
  mat2x4 dq3 = jointDQs[aJointNum.z + gl_InstanceIndex * aModelStride];

  // shortest rotation
  weights.y *= sign(dot(dq0[0], dq1[0]));
  weights.z *= sign(dot(dq0[0], dq2[0]));
  weights.w *= sign(dot(dq0[0], dq3[0]));

  // blend
  mat2x4 result =
      weights.x * dq0 +
      weights.y * dq1 +
      weights.z * dq2 +
      weights.w * dq3;

  // normalize the dual quaternion
  float norm = length(result[0]);
  return result / norm;
}

mat4 getSkinMat() {
  mat2x4 bone = getJointTransform(aJointNum, aJointWeight);

  vec4 r = bone[0]; // rotation
  vec4 t = bone[1]; // translation

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

void main() {
  mat4 skinMat = getSkinMat();
  gl_Position = projection * view * skinMat * vec4(aPos, 1.0);
  //gl_Position = projection * view * vec4(aPos, 1.0);
  normal = vec3(transpose(inverse(skinMat)) * vec4(aNormal, 1.0));
  texCoord = aTexCoord;
  txidx=txid;
  //newcolor=vec4(((aJointNum) & 0xFF),((aJointNum >> 8) & 0xFF),((aJointNum >> 16) & 0xFF),((aJointNum >> 24) & 0xFF));
  newcolor =aJointNum;
  
}
