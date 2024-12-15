#pragma once
#include "plugin.hpp"
#include <widget/OpenGlWidget.hpp>
#include "shader_menu.hpp"

struct GLCVProcessor;

struct Glcv : rack::Module, ShaderSubscriber {
    enum ParamId {
        PARAM_CHAOS,
        PARAM_SCALE,
        PARAMS_LEN
    };
    enum InputId {
         INPUT_CLK,
         INPUT_RST,
         INPUT_TS,
         INPUTS_LEN
    };
    enum OutputId {
        OUTPUT_1,
        OUTPUT_2,
        OUTPUT_3,
        OUTPUT_4,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    float phase = 0.f;
    float clockTime = 0.f;
    float chaos = 0.f;
    float scale = 1.f;
    float timeSpace = 0.f;
    bool clockTriggered = false;
    float lastClockValue = 0.f;

    int subscribedGlibId = -1;
    int subscribedShaderIndex = -1;
    GLCVProcessor* processor = nullptr;

    Glcv();
    void process(const ProcessArgs& args) override;
    void onReset() override;
    void onShaderSubscribe(int64_t glibId, int shaderIndex) override;
}; 
