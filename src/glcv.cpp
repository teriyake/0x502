#include "plugin.hpp"


struct Glcv : Module {
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
    bool clockTriggered = false;
    float lastClockValue = 0.f;

	Glcv() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_CLK, "clock");
		configInput(INPUT_RST, "reset");
		configInput(INPUT_TS, "time/space toggle");
        configParam(PARAM_SCALE, 0.f, 1.f, 0.5f, "output scale", "%", 0.f, 100.f);
        configParam(PARAM_CHAOS, 0.f, 1.f, 0.f, "chaos amount", "%", 0.f, 100.f);
		configOutput(OUTPUT_1, "cv #1");
		configOutput(OUTPUT_2, "cv #2");
		configOutput(OUTPUT_3, "cv #3");
		configOutput(OUTPUT_4, "cv #4");
	}

	void process(const ProcessArgs& args) override {
        float clockValue = inputs[INPUT_CLK].getVoltage();
        bool clockHigh = clockValue >= 1.f;
        bool clockTriggered = clockHigh && !lastClockValue;
        lastClockValue = clockHigh;

        if (inputs[INPUT_RST].getVoltage() >= 1.f) {
            phase = 0.f;
            clockTime = 0.f;
        }

        if (clockTriggered) {
            clockTime += 1.f;
        }

        float t = inputs[INPUT_TS].getVoltage() > 1.0f ? 0.0f : 1.0f;
        float s = 1.0f - t;
        float outputScale = params[PARAM_SCALE].getValue();
        float chaos = params[PARAM_CHAOS].getValue();
        float timeWarp = t > 0.0f ? chaos / outputScale : 0.0f;
        float timeScale = s > 0.0f ? chaos  * outputScale : 1.0f;

        // TODO: Update this with actual shader-based CV generation
        float time = clockTime * timeScale;
        outputs[OUTPUT_1].setVoltage(5.f * outputScale * std::sin(2.f * M_PI * time));
        outputs[OUTPUT_2].setVoltage(5.f * outputScale * std::sin(4.f * M_PI * time + timeWarp));
        outputs[OUTPUT_3].setVoltage(5.f * outputScale * std::sin(8.f * M_PI * time + chaos));
        outputs[OUTPUT_4].setVoltage(5.f * outputScale * std::sin(16.f * M_PI * time + timeWarp + chaos));
	}
};


struct GlcvWidget : ModuleWidget {
	GlcvWidget(Glcv* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glcv.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 14.552)), module, Glcv::INPUT_CLK));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.162, 14.552)), module, Glcv::INPUT_RST));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 32.686)), module, Glcv::INPUT_TS));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.478, 79.2)), module, Glcv::PARAM_CHAOS));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.162, 116.965)), module, Glcv::PARAM_SCALE));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.162, 32.686)), module, Glcv::OUTPUT_1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.478, 53.49)), module, Glcv::OUTPUT_2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.162, 95.06)), module, Glcv::OUTPUT_3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.478, 115.864)), module, Glcv::OUTPUT_4));
	}
};


Model* modelGlcv = createModel<Glcv, GlcvWidget>("glcv");
