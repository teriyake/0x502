#pragma once
#include "plugin.hpp"
#include "shader_manager.hpp"
#include "shader_menu.hpp"
#include <widget/OpenGlWidget.hpp>

struct GLProcessor;

struct Glaze : Module, ShaderSubscriber {
    enum ParamId {
        PARAM_LAYER,
        PARAM_BLEND,
        PARAM_MODE,
        PARAM_MIX,
        PARAMS_LEN
    };
    enum InputId {
        INPUT_L,
        INPUT_R,
        INPUT_U1,
        INPUT_U2,
        INPUT_U3,
        INPUT_LAYER,
        INPUT_BLEND,
        INPUT_MIX,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUT_L,
        OUTPUT_R,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHT_REV,
        LIGHT_DLY,
        LIGHT_FZZ,
        LIGHT_GLD,
        LIGHT_GRN,
        LIGHT_FLD,
        LIGHT_WRP,
        LIGHT_SPC,
        LIGHTS_LEN
    };

    enum Mode {
        MODE_REV,
        MODE_DLY,
        MODE_FZZ,
        MODE_GLD,
        MODE_GRN,
        MODE_FLD,
        MODE_WRP,
        MODE_SPC,
        NUM_MODES
    };

    Mode currentMode = MODE_REV;
    dsp::SchmittTrigger modeTrigger;
    ShaderSubscription shaderSub;
    std::vector<float> layer1Buffer;
    std::vector<float> layer2Buffer;
    int bufferSize = 4096;
    float sampleRate = 44100.f;
    bool shaderDirty = false;

    float fuzzLastL = 0.f;
    float fuzzLastR = 0.f;
    float glidePhaseL = 0.f;
    float glidePhaseR = 0.f;
    float glideLastFreqL = 1.f;
    float glideLastFreqR = 1.f;
    float warpPhaseL = 0.f;
    float warpPhaseR = 0.f;
    float warpLastL = 0.f;
    float warpLastR = 0.f;

    struct Grain {
        bool active = false;
        size_t position = 0;
        size_t length = 0;
        float pan = 0.5f;
        float pitch = 1.f;
        float amp = 0.f;
        float phase = 0.f;
    };
    static const size_t MAX_GRAINS = 32;
    std::vector<Grain> grains{MAX_GRAINS};
    size_t writePos = 0;
    float grainTrigPhase = 0.f;

    static const size_t SPEC_SIZE = 16;
    std::vector<float> specBufL{SPEC_SIZE, 0.f};
    std::vector<float> specBufR{SPEC_SIZE, 0.f};
    std::vector<float> specWindowL{SPEC_SIZE, 0.f};
    std::vector<float> specWindowR{SPEC_SIZE, 0.f};
    size_t specPos = 0;
    float specPhase = 0.f;

    float* shaderBuffer = nullptr;
    static const int SHADER_BUFFER_SIZE = 1024;
    bool useShaderWaveshaping = false;
    GLProcessor* processor = nullptr;
    bool shaderEnabled = false;

    Glaze();
    ~Glaze();

    void onSampleRateChange(const SampleRateChangeEvent& e) override;
    void onReset() override;
    void process(const ProcessArgs& args) override;
    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;

    void onShaderSubscribe(int64_t glibId, int shaderIndex) override;

    void processMode();
    void processShader();
    float processShaderWaveshaping(float input);
    void processReverb(float input, std::vector<float>& buffer, float decay, float diffusion);
    void processDelay(float input, std::vector<float>& buffer, float delayTime, float feedback, float modulation, const ProcessArgs& args);
    float processFuzz(float input, float drive, float shape, float tone, float& lastSample);
    float processGlide(float input, float& phase, float& lastFreq, float targetFreq, float glideSpeed, float waveform, const ProcessArgs& args);
    void processGrain(float input, std::vector<float>& buffer, float& outL, float& outR, float density, float size, float pitch, const ProcessArgs& args);
    float processFold(float input, float folds, float symmetry, float bias);
    float processWarp(float input, float& phase, float& lastSample, float amount, float shape, float skew, const ProcessArgs& args);
    float processSpectral(float input, std::vector<float>& specBuf, std::vector<float>& window, float spread, float shift, float smear);
};

struct GLProcessor : rack::widget::OpenGlWidget {
    GLuint shaderProgram = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint frameBuffer = 0;
    GLuint renderTexture = 0;
    bool dirty = true;
    bool initialized = false;

    GLint posAttrib = -1;
    GLint audioInLUniform = -1;
    GLint audioInRUniform = -1;
    GLint u1Uniform = -1;
    GLint u2Uniform = -1;
    GLint u3Uniform = -1;
    GLint modeUniform = -1;

    // thread-safe communication buffers
    struct AudioFrame {
        float inL = 0.f;
        float inR = 0.f;
        float u1 = 0.f;
        float u2 = 0.f;
        float u3 = 0.f;
        int mode = 0;
        float outL = 0.f;
        float outR = 0.f;
    };
    
    AudioFrame currentFrame;
    AudioFrame processedFrame;
    
    Glaze* module = nullptr;

    GLProcessor();
    ~GLProcessor();
    void setModule(Glaze* mod);
    void createShaderProgram();
    void setupFramebuffer();
    void setupGeometry();
    void step() override;
    void processAudio(float& outL, float& outR, float inL, float inR, float u1, float u2, float u3, int mode);
    void processShader(); // process shaders in render frame ONLY!!!!!!!
}; 
