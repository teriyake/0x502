#version 120

varying vec2 fs_TexCoord;

uniform float u_AudioData[512];
uniform float u_Time;
uniform vec2 u_Resolution;

float getWaveform(vec2 uv) {
    float audioIndex = uv.x * 511.0;
    int index = int(audioIndex);
    int nextIndex = index + 1;
    if (nextIndex > 511) nextIndex = 511;
    float fract = audioIndex - float(index);
    
    float curr = u_AudioData[index];
    float next = u_AudioData[nextIndex];
    float audioValue = mix(curr, next, fract);
    
    float dist = abs(uv.y - 0.5);
    float waveform = smoothstep(abs(audioValue * 0.5), 0.0, dist);
    
    return waveform;
}

vec3 colorGradient(float t) {
    vec3 a = vec3(0.5, 0.0, 0.5);
    vec3 b = vec3(0.0, 0.8, 0.8);
    vec3 c = vec3(1.0, 0.2, 0.3);
    
    float t2 = t * 2.0;
    if (t2 < 1.0) {
        return mix(a, b, t2);
    } else {
        return mix(b, c, t2 - 1.0);
    }
}

void main() {
    vec2 uv = fs_TexCoord;
    
    float waveform = getWaveform(uv);
    
    float flowOffset = sin(uv.x * 10.0 + u_Time) * 0.02;
    float waveform2 = getWaveform(vec2(uv.x, uv.y + flowOffset));
    
    vec3 color1 = colorGradient(uv.x + u_Time * 0.1);
    vec3 color2 = colorGradient(uv.x + u_Time * 0.15 + 0.3);
    
    float glow = waveform * waveform2;
    glow = pow(glow, 2.0);
    
    vec3 finalColor = mix(color1, color2, waveform2) * (0.5 + glow * 2.0);
    
    float energy = 0.0;
    for (int i = 0; i < 512; i += 8) {
        energy += abs(u_AudioData[i]);
    }
    energy = energy / 64.0;
    
    finalColor *= 1.0 + energy * 0.2;
    
    gl_FragColor = vec4(finalColor, 0.15);
} 
