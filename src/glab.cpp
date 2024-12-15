#include "plugin.hpp"
#include "shader_manager.hpp"
#include <string>
#include "gl_utils.hpp"

struct GlabTextEditor : OpaqueWidget {
	std::string text;
	std::vector<std::string> lines;
	float fontSize = 12.0f;
	float lineSpacing = 1.2f;
	Vec textOffset;
	bool isFocused = false;
	NVGcolor textColor = nvgRGB(0xee, 0xee, 0xee);
	NVGcolor backgroundColor = nvgRGB(0x20, 0x20, 0x20);

	GlabTextEditor() {
		box.size = Vec(200, 200);
		textOffset = Vec(4, fontSize);
	}

	void setText(const std::string& newText) {
		text = newText;
		updateLines();
	}

	void updateLines() {
		lines.clear();
		std::string currentLine;
		for (char c : text) {
			if (c == '\n') {
				lines.push_back(currentLine);
				currentLine.clear();
			} else {
				currentLine += c;
			}
		}
		lines.push_back(currentLine);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);

		if (!text.empty()) {
			nvgFontSize(args.vg, fontSize);
			nvgFillColor(args.vg, textColor);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			
			float y = textOffset.y;
			float lineHeight = fontSize * lineSpacing;
			
			for (const std::string& line : lines) {
				nvgText(args.vg, textOffset.x, y, line.c_str(), NULL);
				y += lineHeight;
			}
		}

		if (isFocused) {
			float cursorX = textOffset.x;
			float cursorY = textOffset.y + (lines.size() - 1) * (fontSize * lineSpacing);
			if (!lines.empty()) {
				cursorX += nvgTextBounds(args.vg, 0, 0, lines.back().c_str(), NULL, NULL);
			}
			
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, cursorX, cursorY);
			nvgLineTo(args.vg, cursorX, cursorY + fontSize);
			nvgStrokeColor(args.vg, textColor);
			nvgStrokeWidth(args.vg, 1.0);
			nvgStroke(args.vg);
		}
	}

	void onButton(const event::Button& e) override {
		e.consume(this);
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			APP->event->setSelectedWidget(this);
		}
	}

	void onSelect(const event::Select& e) override {
		isFocused = true;
	}

	void onDeselect(const event::Deselect& e) override {
		isFocused = false;
	}

	void step() override {
		OpaqueWidget::step();
		if (isFocused) {
			APP->event->setSelectedWidget(this);
		}
	}

	void onHover(const event::Hover& e) override {
		e.consume(this);
	}

	void onLeave(const event::Leave& e) override {
		e.consume(this);
	}

	void onDragStart(const event::DragStart& e) override {
		e.consume(this);
	}

	void onDragEnd(const event::DragEnd& e) override {
		e.consume(this);
	}

	void onDragMove(const event::DragMove& e) override {
		e.consume(this);
	}

	void onDragEnter(const event::DragEnter& e) override {
		e.consume(this);
	}

	void onDragLeave(const event::DragLeave& e) override {
		e.consume(this);
	}

	void onHoverKey(const event::HoverKey& e) override {
		if (!isFocused) return;

		e.consume(this);

		if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
			switch (e.key) {
				case GLFW_KEY_BACKSPACE:
					if (!text.empty()) {
						text.pop_back();
						updateLines();
					}
					return;

				case GLFW_KEY_ENTER:
				case GLFW_KEY_KP_ENTER:
					text += '\n';
					updateLines();
					return;

				case GLFW_KEY_TAB:
					text += "    ";
					updateLines();
					return;

				case GLFW_KEY_ESCAPE:
					APP->event->setSelectedWidget(NULL);
					return;

				default:
					return;
			}
		}
	}

	void onHoverText(const event::HoverText& e) override {
		if (!isFocused) return;
		e.consume(this);
		text += e.codepoint;
		updateLines();
	}

	void onAction(const event::Action& e) override {
		if (isFocused) {
			e.consume(this);
		}
	}

	void onSelectKey(const event::SelectKey& e) override {
		if (isFocused) {
			e.consume(this);
			
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				switch (e.key) {
					case GLFW_KEY_BACKSPACE:
						if (!text.empty()) {
							text.pop_back();
							updateLines();
						}
						break;

					case GLFW_KEY_ENTER:
					case GLFW_KEY_KP_ENTER:
						text += '\n';
						updateLines();
						break;

					case GLFW_KEY_TAB:
						text += "    ";
						updateLines();
						break;

					case GLFW_KEY_ESCAPE:
						APP->event->setSelectedWidget(NULL);
						break;
				}
			}
		}
	}
};

struct Glab : Module {
	enum ParamId {
		PARAM_COMP_V,
		PARAM_COMP_F,
		PARAMS_LEN
	};
	enum InputId {
		INPUT_L,
		INPUT_R,
		INPUT_CV_1,
		INPUT_CV_2,
		INPUT_CV_3,
		INPUT_CV_4,
		INPUT_COMP_V,
		INPUT_COMP_F,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_VERT,
		OUTPUT_FRAG,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHT_COMP_V,
		LIGHT_COMP_F,
		LIGHTS_LEN
	};

	std::string vertexShaderText;
	std::string fragmentShaderText;
	std::string errorText;
	bool compilationNeeded = false;
	dsp::SchmittTrigger compileVertexTrigger;
	dsp::SchmittTrigger compileFragTrigger;

	Glab() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_L, "left audio");
		configInput(INPUT_R, "right audio");
		configInput(INPUT_CV_1, "cv 1");
		configInput(INPUT_CV_2, "cv 2");
		configInput(INPUT_CV_3, "cv 3");
		configInput(INPUT_CV_4, "cv 4");
		configInput(INPUT_COMP_V, "compile vertex shader trigger");
		configInput(INPUT_COMP_F, "compile fragment shader trigger");
		configButton(PARAM_COMP_V, "compile vertex shader");
		configButton(PARAM_COMP_F, "compile fragment shader");
		configOutput(OUTPUT_VERT, "vertex shader");
		configOutput(OUTPUT_FRAG, "fragment shader");

		vertexShaderText = 
R"(#version 120

attribute vec3 vs_Pos;

void main() {
    gl_Position = vec4(vs_Pos, 1.0);
}
)";

		fragmentShaderText = 
R"(#version 120

void main() {
    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";
	}

	void process(const ProcessArgs& args) override {
		if (inputs[INPUT_COMP_V].isConnected() && compileVertexTrigger.process(inputs[INPUT_COMP_V].getVoltage())) {
			// TODO: compile vertex shader
			lights[LIGHT_COMP_V].setBrightness(1.0f);
		}
		
		if (inputs[INPUT_COMP_F].isConnected() && compileFragTrigger.process(inputs[INPUT_COMP_F].getVoltage())) {
			// TODO: compile fragment shader
			lights[LIGHT_COMP_F].setBrightness(1.0f);
		}

		if (inputs[INPUT_L].isConnected() || inputs[INPUT_R].isConnected() ||
			inputs[INPUT_CV_1].isConnected() || inputs[INPUT_CV_2].isConnected() ||
			inputs[INPUT_CV_3].isConnected() || inputs[INPUT_CV_4].isConnected()) {
			// TODO: use input values to modify shaders
		}
	}
};

struct GlabWidget : ModuleWidget {
	GlabTextEditor* vertexEditor;
	GlabTextEditor* fragmentEditor;
	GlabTextEditor* errorEditor;

	GlabWidget(Glab* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glab.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.427, 14.952)), module, Glab::INPUT_L));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.773, 14.952)), module, Glab::INPUT_R));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.427, 48.375)), module, Glab::INPUT_CV_1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.875, 48.375)), module, Glab::INPUT_CV_3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.427, 65.087)), module, Glab::INPUT_CV_2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.875, 65.087)), module, Glab::INPUT_CV_4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.325, 81.799)), module, Glab::INPUT_COMP_V));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.773, 81.799)), module, Glab::INPUT_COMP_F));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(9.325, 92.34)), module, Glab::PARAM_COMP_V));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(193.773, 92.34)), module, Glab::PARAM_COMP_F));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.427, 115.864)), module, Glab::OUTPUT_VERT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(193.773, 115.864)), module, Glab::OUTPUT_FRAG));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(9.325, 100.013)), module, Glab::LIGHT_COMP_V));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(193.773, 100.012)), module, Glab::LIGHT_COMP_F));

		vertexEditor = new GlabTextEditor;
		vertexEditor->box.pos = mm2px(Vec(18.855, 5.49));
		vertexEditor->box.size = mm2px(Vec(80.0, 97.607));
		if (module) vertexEditor->setText(module->vertexShaderText);
		addChild(vertexEditor);

		fragmentEditor = new GlabTextEditor;
		fragmentEditor->box.pos = mm2px(Vec(104.345, 25.403));
		fragmentEditor->box.size = mm2px(Vec(80.0, 97.607));
		if (module) fragmentEditor->setText(module->fragmentShaderText);
		addChild(fragmentEditor);

		errorEditor = new GlabTextEditor;
		errorEditor->box.pos = mm2px(Vec(104.345, 5.49));
		errorEditor->box.size = mm2px(Vec(80.0, 14.423));
		errorEditor->backgroundColor = nvgRGB(0x30, 0x20, 0x20);
		if (module) errorEditor->setText(module->errorText);
		addChild(errorEditor);
	}
};

Model* modelGlab = createModel<Glab, GlabWidget>("glab");
