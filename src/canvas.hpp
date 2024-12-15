#pragma once
#include "plugin.hpp"
#include <widget/OpenGlWidget.hpp>
#include "shader_menu.hpp"

struct GLCanvasWidget;

struct Canvas : rack::Module, ShaderSubscriber {
    enum ParamId {
        PARAM_TIME_1,
        PARAM_TIME_2,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_1,
        INPUT_TRIG_1,
        INPUT_2,
        INPUT_TRIG_2,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUT_1,
        OUTPUT_2,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    int ch1 = 0;
    int ch2 = 0;
    float audioBuffer1[256] = {0.f};
    float audioBuffer2[256] = {0.f};
    float smoothingFactor = 0.3f;
    float timeWarp1 = 0.0f;
    float timeWarp2 = 0.0f;
    float trig1 = 0.0f;
    float trig2 = 0.0f;

    int subscribedGlibId = -1;
    int subscribedShaderIndex = -1;
    GLCanvasWidget* glCanvas = nullptr;

    Canvas();
    void process(const ProcessArgs& args) override;
    void onReset() override;
    void onShaderSubscribe(int64_t glibId, int shaderIndex) override;
}; 
