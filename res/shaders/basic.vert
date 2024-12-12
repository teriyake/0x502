#version 120

attribute vec3 vs_Pos;
attribute vec2 vs_TexCoord;

varying vec2 fs_TexCoord;

uniform mat4 u_Proj;
uniform mat4 u_Model;

void main() {
    gl_Position = u_Proj * u_Model * vec4(vs_Pos, 1.0);
    fs_TexCoord = vs_TexCoord;
} 
