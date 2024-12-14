#version 120

varying vec2 fs_TexCoord;

uniform float u_AudioData1[256];
uniform float u_AudioData2[256];
uniform float u_Time;
uniform vec2 u_Resolution;
uniform float u_Trigger1;
uniform float u_Trigger2;
uniform float u_TimeWarp1;
uniform float u_TimeWarp2;

float getWaveform1(vec2 uv, float timeOffset) {
    float audioIndex = uv.x * 255.0;
    int index = int(audioIndex);
    int nextIndex = index + 1;
    if (nextIndex > 255) nextIndex = 255;
    float fract = audioIndex - float(index);
    
    float curr = u_AudioData1[index];
    float next = u_AudioData1[nextIndex];
    float audioValue = mix(curr, next, fract);
    
    float dist = abs(uv.y - 0.5 + sin(u_Time * timeOffset + uv.x * 6.28) * 0.1);
    float waveform = smoothstep(abs(audioValue * 0.5), 0.0, dist);
    
    return waveform;
}

float getWaveform2(vec2 uv, float timeOffset) {
    float audioIndex = uv.x * 255.0;
    int index = int(audioIndex);
    int nextIndex = index + 1;
    if (nextIndex > 255) nextIndex = 255;
    float fract = audioIndex - float(index);
    
    float curr = u_AudioData2[index];
    float next = u_AudioData2[nextIndex];
    float audioValue = mix(curr, next, fract);
    
    float dist = abs(uv.y - 0.5 + sin(u_Time * timeOffset + uv.x * 6.28) * 0.1);
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

float getEnergy1() {
    float energy = 0.0;
    for (int i = 0; i < 256; i += 4) {
        energy += abs(u_AudioData1[i]);
    }
    return energy / 64.0;
}

float getEnergy2() {
    float energy = 0.0;
    for (int i = 0; i < 256; i += 4) {
        energy += abs(u_AudioData2[i]);
    }
    return energy / 64.0;
}

void main() {
    vec2 uv = fs_TexCoord;
    
    float warpedTime1 = u_Time * (1.0 + u_TimeWarp1 * 0.2);
    float warpedTime2 = u_Time * (1.0 + u_TimeWarp2 * 0.2);
    
    float waveform1 = getWaveform1(uv, warpedTime1);
    float waveform2 = getWaveform2(uv, warpedTime2);
    
    float flowOffset1 = sin(uv.x * 10.0 + warpedTime1) * 0.02;
    float flowOffset2 = cos(uv.y * 8.0 + warpedTime2) * 0.02;
    
    float waveform1_warped = getWaveform1(vec2(uv.x + flowOffset1, uv.y + flowOffset2), warpedTime1);
    float waveform2_warped = getWaveform2(vec2(uv.x - flowOffset2, uv.y - flowOffset1), warpedTime2);
    
    float energy1 = getEnergy1();
    float energy2 = getEnergy2();
    
    vec3 color1 = colorGradient(uv.x + warpedTime1 * 0.1);
    vec3 color2 = colorGradient(uv.x + warpedTime2 * 0.15 + 0.3);
    
    float triggerPulse1 = u_Trigger1 * sin(warpedTime1 * 10.0) * 0.5 + 0.5;
    float triggerPulse2 = u_Trigger2 * sin(warpedTime2 * 8.0) * 0.5 + 0.5;
    
    float glow = (waveform1 * waveform2_warped * (1.0 + triggerPulse1)) + 
                 (waveform2 * waveform1_warped * (1.0 + triggerPulse2));
    glow = pow(glow, 2.0);
    
    vec3 finalColor = mix(color1, color2, (waveform1_warped + waveform2_warped) * 0.5);
    finalColor = mix(finalColor, vec3(1.0), glow * 0.5);
    
    float combinedEnergy = (energy1 + energy2) * 0.5;
    finalColor *= 1.0 + combinedEnergy * 0.3;
    
    finalColor += vec3(triggerPulse1 * 0.2, triggerPulse2 * 0.2, (triggerPulse1 + triggerPulse2) * 0.1);
    
    gl_FragColor = vec4(finalColor, 1.0);
} 
