#version 400

#define PI 3.141592653
// take in a point and output a triangle strip with 4 vertices (aka a "quad")
//
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
// XXX ~ what about winding? should use triangles?

//uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform float pointSize;

in Vertex {
  vec4 color;
  float size;
  vec2 uv;
  vec3 pos;
}
vertex[];

out Fragment {
  vec4 color;
  vec2 mapping;
  vec2 uv;
}
fragment;

void main() {
  mat4 m = al_ProjectionMatrix;   // rename to make lines shorter
  vec4 v = gl_in[0].gl_Position;  // al_ModelViewMatrix * gl_Position
  
  vec3 vv = normalize(v.xyz);

  float hex = cos(PI*(vv.x));
  float why = cos(PI*(vv.y));
  float zee = sin(PI*(vv.z));



   float r = pointSize;
   hex *= vertex[0].size*r;
   why *= vertex[0].size*r;
   zee *= vertex[0].size*r;

  gl_Position = m * (v + vec4(-hex, -why, zee, 0.0));
  fragment.color = vertex[0].color;
  fragment.mapping = vec2(-1.0, -1.0);
  EmitVertex();

  gl_Position = m * (v + vec4(hex, -why, -zee, 0.0));
  fragment.color = vertex[0].color;
  fragment.mapping = vec2(1.0, -1.0);
  EmitVertex();

  gl_Position = m * (v + vec4(-hex, why, -zee, 0.0));
  fragment.color = vertex[0].color;
  fragment.mapping = vec2(-1.0, 1.0);
  EmitVertex();

  gl_Position = m * (v + vec4(hex, why, zee, 0.0));
  fragment.color = vertex[0].color;
  fragment.mapping = vec2(1.0, 1.0);
  EmitVertex();

  fragment.uv = vertex[0].uv;
  EndPrimitive();
  
}