#version 330 core
layout(location = 0) out vec4 iris_FragData0;
layout(location = 1) out vec4 iris_FragData1;
float clamp01(float x) { return clamp(x, 0.0f, 1.0f); }
vec3 fireSpecialLightColor = vec3(0.6f, 0.3f, 2.4f) * 3.8f;
vec3 torchBlockSpecialLightColor = mix(vec3(2.0f, 0.87f, 0.27f), fireSpecialLightColor, clamp01(0.75f));
void main() {
    iris_FragData0 = vec4(torchBlockSpecialLightColor, 1.0f);
    iris_FragData1 = vec4(0.0f);
}
