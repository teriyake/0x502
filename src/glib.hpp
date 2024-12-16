#pragma once
#include "plugin.hpp"
#include "shader_manager.hpp"

struct Glib : Module {
    enum ParamId {
        PARAM_LOAD,
        PARAM_PREV,
        PARAM_NEXT,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_PREV,
        INPUT_NEXT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUT_VERT,
        OUTPUT_FRAG,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHT_LOADED,
        LIGHT_VERT_VALID,
        LIGHT_FRAG_VALID,
        LIGHTS_LEN
    };

    std::vector<ShaderPair> shaderLibrary;
}; 