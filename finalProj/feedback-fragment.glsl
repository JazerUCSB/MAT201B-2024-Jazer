#version 400

in Fragment {
  vec2 vuv;
}
fragment;


layout(location = 0) out vec4 fragmentColor;

uniform sampler2D tex;

void main() {

  vec4 color = texture(tex, vuv);
  // don't round off the corners
  // float r = dot(fragment.mapping, fragment.mapping);
  // if (r > 1) discard;
  // fragmentColor = vec4(fragment.color.rgb, 1 - r * r);
  fragmentColor = vec4(color.rgb, 1.0);
}