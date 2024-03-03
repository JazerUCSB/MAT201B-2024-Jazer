#version 400

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 uv;


// vertexSize is 2D texture cordinate, but we only use the x

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

out Vertex {
  vec4 color;
  float size;
  vec2 uv;
}
vertex;

void main() {
  vuv = uv;
  gl_Position = al_ModelViewMatrix * vec4(vertexPosition, 1.0);
  vertex.color = vertexColor;
  vertex.size = uv.x;
  vertex.uv = uv;
}