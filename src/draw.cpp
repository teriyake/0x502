#include "plugin.hpp"
#include "context.hpp"
#include "logger.hpp"
#include <widget/FramebufferWidget.hpp>
#include <widget/OpenGlWidget.hpp>

struct Draw : Module {
	enum ParamId {
		PARAM_1X2_PARAM,
		PARAM_GAIN_1_PARAM,
		PARAM_GAIN_2_PARAM,
		PARAM_TRIG_PARAM,
		PARAM_TIME_PARAM,
		PARAM_OFFSET_1_PARAM,
		PARAM_OFFSET_2_PARAM,
		PARAM_THRESHOLD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_1_INPUT,
		IN_2_INPUT,
		OUT_TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_1_OUTPUT,
		OUT_2_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Draw() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PARAM_1X2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_GAIN_1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_GAIN_2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_TRIG_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_TIME_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_OFFSET_1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_OFFSET_2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PARAM_THRESHOLD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(IN_1_INPUT, "");
		configInput(IN_2_INPUT, "");
		configInput(OUT_TRIG_INPUT, "");
		configOutput(OUT_1_OUTPUT, "");
		configOutput(OUT_2_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};

struct GLCanvasWidget : rack::widget::OpenGlWidget {
    void step() override {
        this->box.size = math::Vec(195, 160);
        dirty = true;
        FramebufferWidget::step();
    }

    void drawFramebuffer() override {
        math::Vec fbSize = getFramebufferSize();
        glViewport(0.0, 0.0, fbSize.x, fbSize.y);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, fbSize.x, 0.0, fbSize.y, -1.0, 1.0);

        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(0, 0, 0);
        glColor3f(0, 1, 0);
        glVertex3f(fbSize.x, 0, 0);
        glColor3f(0, 0, 1);
        glVertex3f(0, fbSize.y, 0);
        glEnd();
    }
};

struct DrawWidget : ModuleWidget {
	DrawWidget(Draw* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/draw.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.643, 80.603)), module, Draw::PARAM_1X2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(24.897, 80.551)), module, Draw::PARAM_GAIN_1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(41.147, 80.551)), module, Draw::PARAM_GAIN_2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(57.397, 80.521)), module, Draw::PARAM_TRIG_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.643, 96.819)), module, Draw::PARAM_TIME_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(24.897, 96.789)), module, Draw::PARAM_OFFSET_1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(41.147, 96.815)), module, Draw::PARAM_OFFSET_2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(57.397, 96.815)), module, Draw::PARAM_THRESHOLD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.643, 113.115)), module, Draw::IN_1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(33.023, 113.115)), module, Draw::IN_2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(57.397, 113.115)), module, Draw::OUT_TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.833, 113.115)), module, Draw::OUT_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45.212, 113.115)), module, Draw::OUT_2_OUTPUT));

		// mm2px(Vec(66.04, 55.88))

        GLCanvasWidget* canvas = createWidget<GLCanvasWidget>(mm2px(Vec(0.0, 13.039)));
        addChild(canvas);
	}
};


Model* modelDraw = createModel<Draw, DrawWidget>("draw");
