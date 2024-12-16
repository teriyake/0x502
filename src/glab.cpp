#include "plugin.hpp"
#include "shader_manager.hpp"
#include <string>
#include "gl_utils.hpp"
#include "glib.hpp"
#include "shader_menu.hpp"

struct GlabTextEditor : OpaqueWidget {
	std::string text;
	std::vector<std::string> lines;
	float fontSize = 12.0f;
	float lineSpacing = 1.2f;
	Vec textOffset;
	bool isFocused = false;
	NVGcolor textColor = nvgRGB(0xee, 0xee, 0xee);
	NVGcolor backgroundColor = nvgRGB(0x20, 0x20, 0x20);
	std::function<void(const std::string&)> changeCallback;
	
	int cursorLine = 0;
	int cursorPos = 0;
	float viewportOffset = 0.f;

	GlabTextEditor() {
		box.size = Vec(200, 200);
		textOffset = Vec(4, fontSize);
	}

	void setText(const std::string& newText, bool notify = true) {
		if (text == newText) return;
		text = newText;
		updateLines();
		if (notify && changeCallback) {
			changeCallback(text);
		}
		cursorLine = lines.size() - 1;
		cursorPos = lines.empty() ? 0 : lines.back().length();
	}

	std::vector<std::string> wrapText(const std::string& line, float maxWidth, NVGcontext* vg) {
		std::vector<std::string> wrappedLines;
		if (line.empty()) {
			wrappedLines.push_back("");
			return wrappedLines;
		}

		std::string currentLine;
		std::string word;
		float currentWidth = 0.f;
		float spaceWidth = nvgTextBounds(vg, 0, 0, " ", NULL, NULL);

		for (size_t i = 0; i < line.length(); i++) {
			char c = line[i];
			if (c == ' ' || c == '\t' || i == line.length() - 1) {
				if (i == line.length() - 1 && c != ' ' && c != '\t') {
					word += c;
				}
				
				float wordWidth = nvgTextBounds(vg, 0, 0, word.c_str(), NULL, NULL);
				
				if (currentWidth + wordWidth > maxWidth) {
					if (!currentLine.empty()) {
						wrappedLines.push_back(currentLine);
						currentLine = "";
						currentWidth = 0.f;
					}
					if (wordWidth > maxWidth) {
						std::string partialWord;
						for (char wc : word) {
							float charWidth = nvgTextBounds(vg, 0, 0, std::string(1, wc).c_str(), NULL, NULL);
							if (currentWidth + charWidth > maxWidth) {
								wrappedLines.push_back(partialWord);
								partialWord = std::string(1, wc);
								currentWidth = charWidth;
							} else {
								partialWord += wc;
								currentWidth += charWidth;
							}
						}
						if (!partialWord.empty()) {
							currentLine = partialWord;
						}
					} else {
						currentLine = word;
						currentWidth = wordWidth;
					}
				} else {
					if (!currentLine.empty()) {
						currentLine += " ";
						currentWidth += spaceWidth;
					}
					currentLine += word;
					currentWidth += wordWidth;
				}
				word = "";
			} else {
				word += c;
			}
		}

		if (!currentLine.empty()) {
			wrappedLines.push_back(currentLine);
		}

		return wrappedLines;
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

		nvgSave(args.vg);
		nvgIntersectScissor(args.vg, 0, 0, box.size.x, box.size.y);

		if (!text.empty()) {
			nvgFontSize(args.vg, fontSize);
			nvgFillColor(args.vg, textColor);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			
			float y = textOffset.y - viewportOffset;
			float lineHeight = fontSize * lineSpacing;
			float maxWidth = box.size.x - 2 * textOffset.x;
			
			int totalLines = 0;
			for (size_t i = 0; i < lines.size(); i++) {
				auto wrappedLines = wrapText(lines[i], maxWidth, args.vg);
				for (const auto& wrappedLine : wrappedLines) {
					if (y + lineHeight > 0 && y < box.size.y) {
						nvgText(args.vg, textOffset.x, y, wrappedLine.c_str(), NULL);
					}
					y += lineHeight;
					totalLines++;
				}
			}

			if (isFocused) {
				float cursorX = textOffset.x;
				float cursorY = textOffset.y + (cursorLine * lineHeight) - viewportOffset;
				
				if (cursorLine >= 0 && cursorLine < (int)lines.size()) {
					std::string textBeforeCursor = lines[cursorLine].substr(0, cursorPos);
					cursorX += nvgTextBounds(args.vg, 0, 0, textBeforeCursor.c_str(), NULL, NULL);
				}
				
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, cursorX, cursorY);
				nvgLineTo(args.vg, cursorX, cursorY + fontSize);
				nvgStrokeColor(args.vg, textColor);
				nvgStrokeWidth(args.vg, 1.0);
				nvgStroke(args.vg);
			}
		}
		
		nvgRestore(args.vg);
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

	void onHoverText(const event::HoverText& e) override {
		if (!isFocused) return;
		e.consume(this);
		
		if (cursorLine >= 0 && cursorLine < (int)lines.size()) {
			lines[cursorLine].insert(cursorPos, 1, e.codepoint);
			cursorPos++;
			
			text.clear();
			for (size_t i = 0; i < lines.size(); i++) {
				text += lines[i];
				if (i < lines.size() - 1) text += '\n';
			}
			
			if (changeCallback) {
				changeCallback(text);
			}
		}
	}

	void onAction(const event::Action& e) override {
		if (isFocused) {
			e.consume(this);
		}
	}

	void onSelectKey(const event::SelectKey& e) override {
		if (!isFocused) return;
		e.consume(this);
		
		if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
			bool textChanged = false;
			switch (e.key) {
				case GLFW_KEY_BACKSPACE: {
					if (!text.empty()) {
						if (cursorPos > 0 || cursorLine > 0) {
							if (cursorPos > 0) {
								lines[cursorLine].erase(cursorPos - 1, 1);
								cursorPos--;
							} else {
								cursorPos = lines[cursorLine - 1].length();
								lines[cursorLine - 1] += lines[cursorLine];
								lines.erase(lines.begin() + cursorLine);
								cursorLine--;
							}
							text.clear();
							for (size_t i = 0; i < lines.size(); i++) {
								text += lines[i];
								if (i < lines.size() - 1) text += '\n';
							}
							textChanged = true;
						}
					}
				} break;

				case GLFW_KEY_ENTER:
				case GLFW_KEY_KP_ENTER: {
					std::string beforeCursor = lines[cursorLine].substr(0, cursorPos);
					std::string afterCursor = lines[cursorLine].substr(cursorPos);
					lines[cursorLine] = beforeCursor;
					lines.insert(lines.begin() + cursorLine + 1, afterCursor);
					cursorLine++;
					cursorPos = 0;
					text.clear();
					for (size_t i = 0; i < lines.size(); i++) {
						text += lines[i];
						if (i < lines.size() - 1) text += '\n';
					}
					textChanged = true;
				} break;

				case GLFW_KEY_LEFT:
					if (cursorPos > 0) {
						cursorPos--;
					} else if (cursorLine > 0) {
						cursorLine--;
						cursorPos = lines[cursorLine].length();
					}
					break;

				case GLFW_KEY_RIGHT:
					if (cursorPos < (int)lines[cursorLine].length()) {
						cursorPos++;
					} else if (cursorLine < (int)lines.size() - 1) {
						cursorLine++;
						cursorPos = 0;
					}
					break;

				case GLFW_KEY_UP:
					if (cursorLine > 0) {
						cursorLine--;
						cursorPos = std::min(cursorPos, (int)lines[cursorLine].length());
					}
					break;

				case GLFW_KEY_DOWN:
					if (cursorLine < (int)lines.size() - 1) {
						cursorLine++;
						cursorPos = std::min(cursorPos, (int)lines[cursorLine].length());
					}
					break;

				case GLFW_KEY_TAB:
					lines[cursorLine].insert(cursorPos, "    ");
					cursorPos += 4;
					text.clear();
					for (size_t i = 0; i < lines.size(); i++) {
						text += lines[i];
						if (i < lines.size() - 1) text += '\n';
					}
					textChanged = true;
					break;

				case GLFW_KEY_ESCAPE:
					APP->event->setSelectedWidget(NULL);
					break;
			}

			float lineHeight = fontSize * lineSpacing;
			float cursorY = cursorLine * lineHeight;
			
			if (cursorY < viewportOffset) {
				viewportOffset = cursorY;
			}
			else if (cursorY > viewportOffset + box.size.y - lineHeight) {
				viewportOffset = cursorY - box.size.y + lineHeight;
			}
			
			if (textChanged && changeCallback) {
				changeCallback(text);
			}
		}
	}
};

struct Glab : Module, ShaderSubscriber {
	enum ParamId {
		PARAM_COMP_V,
		PARAM_COMP_F,
		PARAM_PUBLISH,
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
		ENUMS(LIGHT_PUBLISH, 3),
		LIGHTS_LEN
	};

	std::string vertexShaderText;
	std::string fragmentShaderText;
	std::string errorText;
	bool vertexShaderValid = false;
	bool fragmentShaderValid = false;
	bool isDirty = false;
	ShaderSubscription subscription;
	dsp::SchmittTrigger compileVertexTrigger;
	dsp::SchmittTrigger compileFragTrigger;
	dsp::SchmittTrigger publishTrigger;
	
	float audioL = 0.f;
	float audioR = 0.f;
	float cv[4] = {0.f, 0.f, 0.f, 0.f};
	
	bool requestVertexCompile = false;
	bool requestFragmentCompile = false;
	
	bool wasPublishPressed = false;
	
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
		configButton(PARAM_PUBLISH, "publish shaders");
		configOutput(OUTPUT_VERT, "vertex shader");
		configOutput(OUTPUT_FRAG, "fragment shader");

		publishTrigger.reset();
		compileVertexTrigger.reset();
		compileFragTrigger.reset();
		
		errorText = "Ready to publish shaders";
		
		vertexShaderText = 
R"(#version 120

attribute vec3 vs_Pos;

void main() {
    gl_Position = vec4(vs_Pos, 1.0);
}
)";

		fragmentShaderText = R"(#version 120
uniform float audioL;
uniform float audioR;
uniform float cv1;
uniform float cv2;
uniform float cv3;
uniform float cv4;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(800, 600);
    float color = sin(uv.x * 10.0 + cv1 * 6.28) * cos(uv.y * 10.0 + cv2 * 6.28);
    color = color * 0.5 + 0.5; // normalize to [0,1]
    color = mix(color, (audioL + audioR) * 0.5, cv3);
    gl_FragColor = vec4(color, color * cv4, 1.0 - color, 1.0);
}
)";

		requestVertexCompile = true;
		requestFragmentCompile = true;
	}

	void process(const ProcessArgs& args) override {
		audioL = inputs[INPUT_L].getVoltage() / 5.f;
		audioR = inputs[INPUT_R].getVoltage() / 5.f;
		
		for (int i = 0; i < 4; i++) {
			cv[i] = inputs[INPUT_CV_1 + i].getVoltage() / 10.f;
		}

		if (params[PARAM_COMP_V].getValue() > 0 || 
			(inputs[INPUT_COMP_V].isConnected() && compileVertexTrigger.process(inputs[INPUT_COMP_V].getVoltage()))) {
			requestVertexCompile = true;
		}
		
		if (params[PARAM_COMP_F].getValue() > 0 || 
			(inputs[INPUT_COMP_F].isConnected() && compileFragTrigger.process(inputs[INPUT_COMP_F].getVoltage()))) {
			requestFragmentCompile = true;
		}

		outputs[OUTPUT_VERT].setVoltage(vertexShaderValid ? 10.f : 0.f);
		outputs[OUTPUT_FRAG].setVoltage(fragmentShaderValid ? 10.f : 0.f);

		float publishValue = params[PARAM_PUBLISH].getValue();
		if (publishValue > 0.f) {
			if (!wasPublishPressed) {
				//INFO("Publish button pressed!");
				publishToGlib();
				wasPublishPressed = true;
			}
		} else {
			wasPublishPressed = false;
		}

		updatePublishLight();
	}

	void onReset() override {
		vertexShaderValid = false;
		fragmentShaderValid = false;
		isDirty = false;
		subscription.isValid = false;
		errorText = "";
		publishTrigger.reset();
		updatePublishLight();
	}

	void updatePublishLight() {
		lights[LIGHT_PUBLISH + 0].setBrightness(!vertexShaderValid || !fragmentShaderValid ? 1.f : 0.f);
		lights[LIGHT_PUBLISH + 1].setBrightness(!isDirty && vertexShaderValid && fragmentShaderValid ? 1.f : 0.f);
		lights[LIGHT_PUBLISH + 2].setBrightness(isDirty ? 1.f : 0.f);
		
        /*
		INFO("Publish light states - Red: %f, Green: %f, Blue: %f", 
			lights[LIGHT_PUBLISH + 0].getBrightness(),
			lights[LIGHT_PUBLISH + 1].getBrightness(),
			lights[LIGHT_PUBLISH + 2].getBrightness());
        */
	}

	void publishToGlib() {
		//INFO("Starting publishToGlib...");
		
		if (!vertexShaderValid || !fragmentShaderValid) {
			errorText = "Cannot publish: Shaders must be valid first!";
			//INFO("Publish failed: Invalid shaders");
			return;
		}

		auto& shaderLib = SharedShaderLibrary::getInstance();
		
		ShaderPair newShader;
		bool isUpdating = false;
		
		if (subscription.isValid) {
			const std::vector<ShaderPair>* shaders = shaderLib.getShadersForGlib(subscription.glibId);
			if (shaders && static_cast<size_t>(subscription.shaderIndex) < shaders->size()) {
				newShader.name = (*shaders)[subscription.shaderIndex].name;
				isUpdating = true;
				//INFO("Updating existing shader '%s' in Glib %lld", newShader.name.c_str(), subscription.glibId);
			}
		}
		
		int64_t targetGlibId;
		if (subscription.isValid) {
			targetGlibId = subscription.glibId;
		} else {
			targetGlibId = id;
			INFO("Using module ID as Glib ID: %lld", targetGlibId);
		}
		
		shaderLib.registerGlib(targetGlibId);
		
		if (!isUpdating) {
			time_t now = time(nullptr);
			char timestamp[32];
			strftime(timestamp, sizeof(timestamp), "%H%M%S", localtime(&now));
			
			const std::vector<ShaderPair>* shaders = shaderLib.getShadersForGlib(targetGlibId);
			int shaderCount = shaders ? shaders->size() : 0;
			newShader.name = string::f("glab_%d_%s", shaderCount + 1, timestamp);
			//INFO("Creating new shader '%s' in Glib %lld", newShader.name.c_str(), targetGlibId);
		}
		
		newShader.vertexSource = vertexShaderText;
		newShader.fragmentSource = fragmentShaderText;
		newShader.isValid = true;
		
		try {
			//INFO("Adding shader to SharedShaderLibrary...");
			shaderLib.addShader(targetGlibId, newShader);
			
			isDirty = false;
			errorText = string::f("Successfully published shader as '%s'", newShader.name.c_str());
			//INFO("Successfully added shader to library");
			
			const std::vector<ShaderPair>* shaders = shaderLib.getShadersForGlib(targetGlibId);
			if (shaders) {
				int newIndex = shaders->size() - 1;
				//INFO("Subscribing to shader at index %d", newIndex);
				shaderLib.subscribeToShader(id, targetGlibId, newIndex);
				subscription = *shaderLib.getSubscription(id);
				//INFO("Successfully subscribed to shader at index %d in Glib %lld", newIndex, targetGlibId);
			} else {
				errorText = "Failed to get shader list after publishing";
				WARN("Failed to get shader list after publishing");
			}
		} catch (const std::exception& e) {
			errorText = string::f("Error publishing shader: %s", e.what());
			WARN("Error publishing shader: %s", e.what());
		} catch (...) {
			errorText = "Unknown error occurred while publishing shader";
			WARN("Unknown error occurred while publishing shader");
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "vertexShader", json_string(vertexShaderText.c_str()));
		json_object_set_new(rootJ, "fragmentShader", json_string(fragmentShaderText.c_str()));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* vertexShaderJ = json_object_get(rootJ, "vertexShader");
		json_t* fragmentShaderJ = json_object_get(rootJ, "fragmentShader");
		if (vertexShaderJ) vertexShaderText = json_string_value(vertexShaderJ);
		if (fragmentShaderJ) fragmentShaderText = json_string_value(fragmentShaderJ);
	}

	void onShaderSubscribe(int64_t glibId, int shaderIndex) override {
		auto& shaderLib = SharedShaderLibrary::getInstance();
		const ShaderPair* shader = shaderLib.getShaderForModule(id);
		if (shader) {
			vertexShaderText = shader->vertexSource;
			fragmentShaderText = shader->fragmentSource;
			isDirty = false;
			errorText = string::f("Subscribed to shader %d from Glib %lld", shaderIndex, glibId);
		} else {
			errorText = string::f("Failed to load shader %d from Glib %lld", shaderIndex, glibId);
		}
	}
};

struct GlabWidget : ModuleWidget {
	GlabTextEditor* vertexEditor;
	GlabTextEditor* fragmentEditor;
	GlabTextEditor* errorEditor;

	void step() override {
		ModuleWidget::step();
		
		Glab* module = dynamic_cast<Glab*>(this->module);
		if (!module) return;

		if (module->requestVertexCompile) {
			module->requestVertexCompile = false;
			GLuint shader = gl::compileShader(module->vertexShaderText, GL_VERTEX_SHADER);
			module->vertexShaderValid = shader != 0;
			if (shader) glDeleteShader(shader);
			module->lights[Glab::LIGHT_COMP_V].setBrightness(module->vertexShaderValid ? 1.f : 0.f);
			module->isDirty = true;
		}
		
		if (module->requestFragmentCompile) {
			module->requestFragmentCompile = false;
			GLuint shader = gl::compileShader(module->fragmentShaderText, GL_FRAGMENT_SHADER);
			module->fragmentShaderValid = shader != 0;
			if (shader) glDeleteShader(shader);
			module->lights[Glab::LIGHT_COMP_F].setBrightness(module->fragmentShaderValid ? 1.f : 0.f);
			module->isDirty = true;
		}

		if (errorEditor->text != module->errorText) {
			//INFO("Updating error text: %s", module->errorText.c_str());
			errorEditor->setText(module->errorText, false);
		}
	}

	void appendContextMenu(Menu* menu) override {
		Glab* module = dynamic_cast<Glab*>(this->module);
		if (!module) return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Shader Source"));
		addShaderMenuItems(menu, module);
	}

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
		
		auto publishButton = createParamCentered<VCVButton>(mm2px(Vec(109.009, 14.952)), module, Glab::PARAM_PUBLISH);
		if (module) {
			INFO("Configuring publish button for module %lld", module->id);
		}
		addParam(publishButton);
		
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.427, 115.864)), module, Glab::OUTPUT_VERT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(193.773, 115.864)), module, Glab::OUTPUT_FRAG));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(9.325, 100.013)), module, Glab::LIGHT_COMP_V));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(193.773, 100.012)), module, Glab::LIGHT_COMP_F));

		vertexEditor = new GlabTextEditor;
		vertexEditor->box.pos = mm2px(Vec(18.855, 5.49));
		vertexEditor->box.size = mm2px(Vec(80.0, 97.607));
		if (module) {
			vertexEditor->setText(module->vertexShaderText, false);
			vertexEditor->changeCallback = [module](const std::string& text) {
				module->vertexShaderText = text;
				module->isDirty = true;
				module->requestVertexCompile = true;
			};
		}
		addChild(vertexEditor);

		fragmentEditor = new GlabTextEditor;
		fragmentEditor->box.pos = mm2px(Vec(104.345, 25.403));
		fragmentEditor->box.size = mm2px(Vec(80.0, 97.607));
		if (module) {
			fragmentEditor->setText(module->fragmentShaderText, false);
			fragmentEditor->changeCallback = [module](const std::string& text) {
				module->fragmentShaderText = text;
				module->isDirty = true;
				module->requestFragmentCompile = true;
			};
		}
		addChild(fragmentEditor);

		errorEditor = new GlabTextEditor;
		errorEditor->box.pos = mm2px(Vec(115.345, 5.49));
		errorEditor->box.size = mm2px(Vec(69.0, 14.423));
		errorEditor->backgroundColor = nvgRGB(0x30, 0x20, 0x20);
		if (module) errorEditor->setText(module->errorText, false);
		addChild(errorEditor);

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(109.009, 22.625)), module, Glab::LIGHT_PUBLISH));
	}
};

Model* modelGlab = createModel<Glab, GlabWidget>("glab");
