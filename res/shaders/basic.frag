#version 120

varying vec2 fs_TexCoord;

uniform float u_AudioData[512];
uniform float u_Time;
uniform vec2 u_Resolution;

void main() {
    vec2 uv = fs_TexCoord;
    
    float audioIndex = int(uv.x * 511.0);
    float audioValue = u_AudioData[int(audioIndex)];
    
    vec3 color = vec3(
        0.5 + 0.5 * sin(u_Time + audioValue * 3.0),
        0.5 + 0.5 * sin(u_Time * 0.7 + audioValue * 2.0),
        0.5 + 0.5 * cos(u_Time * 0.5 + audioValue)
    );
    
    float intensity = smoothstep(abs(audioValue), 0.0, uv.y);
    
    gl_FragColor = vec4(color * intensity, 1.0);
} 
