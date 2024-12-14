#version 120

attribute vec3 vs_Pos;

void main() {
    gl_Position = vec4(vs_Pos, 1.0);
} 
