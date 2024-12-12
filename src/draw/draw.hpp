struct Draw : Module {
    enum ParamIds {
        MODE_PARAM,
        FILE_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        NUM_INPUTS
    };

    enum OutputIds {
        NUM_OUTPUTS
    };

    enum LightIds {
        NUM_LIGHTS
    };

    Draw() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        // ======= CONFIG PARAMS =======
    }

    void onReset() override {}

    void process(const ProcessArgs& args) override {}
};
