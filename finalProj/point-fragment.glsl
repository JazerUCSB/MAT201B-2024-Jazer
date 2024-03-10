#version 400



in Fragment {
  vec4 color;
  vec2 mapping;
  vec2 uv;
  
}
fragment;

layout(location = 0) out vec4 fragmentColor;


void main() {
  // don't round off the corners
   float r = dot(fragment.mapping, fragment.mapping);
   if (r > 1) discard;
   //fragmentColor = vec4(fragment.uv, 0.0, 1.0);
   fragmentColor = vec4(fragment.color.rgb, 1 - r * r);
  //fragmentColor = vec4(fragment.color.rgb, 1.0);
}