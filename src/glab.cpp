#include "plugin.hpp"


struct Glab : Module {
	enum ParamId {
		PARAM_FRAG,
		PARAM_VERT,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
        OUTPUT_VERT,
        OUTPUT_FRAG,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHT_VERT_1,
		LIGHT_VERT_2,
		LIGHT_VERT_3,
		LIGHT_FRAG_1,
		LIGHT_FRAG_2,
		LIGHTS_LEN
	};

	Glab() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PARAM_VERT, 0.0f, 10.0f, 0.0f, "activate loaded vertex shader at index", "");
		configParam(PARAM_FRAG, 0.0f, 10.0f, 0.0f, "activate loaded fragment shader at index", "");
		configOutput(OUTPUT_VERT, "vertex shader");
		configOutput(OUTPUT_FRAG, "fragment shader");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct GlabWidget : ModuleWidget {
	GlabWidget(Glab* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glab.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(193.773, 15.807)), module, Glab::PARAM_FRAG));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.427, 57.355)), module, Glab::PARAM_VERT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.427, 115.864)), module, Glab::OUTPUT_VERT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(193.773, 115.864)), module, Glab::OUTPUT_FRAG));
        addChild(createLightCentered<MediumLight<TYellowLight<>>>(mm2px(Vec(5.25, 16.311)), module, Glab::LIGHT_VERT_1));
		addChild(createLightCentered<TinyLight<TYellowLight<>>>(mm2px(Vec(13.604, 30.334)), module, Glab::LIGHT_VERT_2));
		addChild(createLightCentered<MediumLight<TYellowLight<>>>(mm2px(Vec(5.25, 83.547)), module, Glab::LIGHT_VERT_3));
		addChild(createLightCentered<MediumLight<TYellowLight<>>>(mm2px(Vec(197.95, 57.239)), module, Glab::LIGHT_FRAG_1));
		addChild(createLightCentered<TinyLight<TYellowLight<>>>(mm2px(Vec(189.596, 71.261)), module, Glab::LIGHT_FRAG_2));

		// mm2px(Vec(80.0, 14.423))
		addChild(createWidget<Widget>(mm2px(Vec(104.345, 5.49))));
		// mm2px(Vec(80.0, 97.607))
		addChild(createWidget<Widget>(mm2px(Vec(18.855, 5.49))));
		// mm2px(Vec(80.0, 97.607))
		addChild(createWidget<Widget>(mm2px(Vec(104.345, 25.403))));
	}
};


Model* modelGlab = createModel<Glab, GlabWidget>("glab");
