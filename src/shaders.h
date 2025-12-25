#pragma once

static const char* kGlowFragmentShader = R"(
uniform float uIntensity;
void main() {
    vec2 uv = gl_TexCoord[0].xy * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    float a = exp(-r2 * 2.6) * uIntensity;
    vec4 c = gl_Color;
    c.a *= a;
    gl_FragColor = c;
}
)";

static const char* kBlurFragmentShader = R"(
uniform sampler2D uTexture;
uniform vec2 uDirection;
uniform float uRadius;
uniform float uSigma;

float gaussian(float x, float sigma) {
    return exp(-(x*x) / (2.0*sigma*sigma));
}

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 sum = vec4(0.0);
    float wsum = 0.0;

    float radius = uRadius;
    float sigma = uSigma;

    for (float i = -radius; i <= radius; i += 1.0) {
        float w = gaussian(i, sigma);
        vec2 offset = uDirection * i;
        sum += texture2D(uTexture, uv + offset) * w;
        wsum += w;
    }

    gl_FragColor = sum / wsum;
}
)";