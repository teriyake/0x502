#version 120

attribute vec3 vs_Pos;
attribute vec2 vs_TexCoord;

varying vec2 fs_TexCoord;

void main() {
    gl_Position = vec4(vs_Pos, 1.0);
    fs_TexCoord = vs_TexCoord;
} 
