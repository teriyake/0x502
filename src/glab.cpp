#include "glab.hpp"

void GlabTextField::draw(const DrawArgs& args) {
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3.0);
    nvgFillColor(args.vg, bgColor);
    nvgFill(args.vg);

    Widget::draw(args);
}

int GlabTextField::getTextPosition(Vec position) {
    position.y += scrollPos;
    
    int line = (int)(position.y - 3) / lineHeight;
    if (line < 0) line = 0;
    
    std::string text = getText();
    int currentLine = 0;
    size_t lineStart = 0;
    size_t pos = 0;
    
    while (currentLine < line && (pos = text.find('\n', lineStart)) != std::string::npos) {
        lineStart = pos + 1;
        currentLine++;
    }
    
    if (currentLine < line) {
        return text.length();
    }
    
    size_t lineEnd = text.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = text.length();
    }
    
    std::string lineText = text.substr(lineStart, lineEnd - lineStart);
    
    float x = position.x + 3;
    if (x <= 0) return lineStart;
    
    int left = 0;
    int right = lineText.length();
    
    while (left < right) {
        int mid = (left + right + 1) / 2;
        float width = nvgTextBounds(APP->window->vg, 0, 0, lineText.substr(0, mid).c_str(), NULL, NULL);
        
        if (width < x) {
            left = mid;
        } else {
            right = mid - 1;
        }
    }
    
    return lineStart + left;
}

void GlabTextField::onButton(const event::Button& e) {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        cursor = getTextPosition(e.pos);
        selectionStart = cursor;
        selectionEnd = cursor;
        dragPos = e.pos;
        isDragging = true;
    } else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        isDragging = false;
    }
    LedDisplayTextField::onButton(e);
}

void GlabTextField::onDragMove(const event::DragMove& e) {
    if (isDragging) {
        dragPos += e.mouseDelta;
        cursor = getTextPosition(dragPos);
        selectionEnd = cursor;
    }
    LedDisplayTextField::onDragMove(e);
}

void GlabTextField::drawLayer(const DrawArgs& args, int layer) {
    if (layer == 1) {
        std::shared_ptr<window::Font> font = APP->window->loadFont(fontPath);
		if (font && font->handle >= 0) {
			bndSetFont(font->handle);

			NVGcolor highlightColor = nvgRGB(0xB8, 0xBB, 0x26);
			highlightColor.a = 0.5;

			nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);
			nvgFontSize(args.vg, 12);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgFillColor(args.vg, textColor);

			std::string text = getText();
			std::vector<std::string> wrappedLines = wrapText(text, box.size.x - 6);
			
			float y = -scrollPos;
			for (const std::string& line : wrappedLines) {
				if (y + lineHeight >= 0 && y <= box.size.y) {
					nvgText(args.vg, 3, y + 3, line.c_str(), NULL);
				}
				y += lineHeight;
			}

			nvgResetScissor(args.vg);

			if (isFocused) {
				int cursorPos = cursor;
				std::string preText = text.substr(0, cursorPos);
				
				int cursorLine = 0;
				size_t lastNewline = 0;
				size_t pos = 0;
				
				while ((pos = preText.find('\n', lastNewline)) != std::string::npos) {
					cursorLine++;
					lastNewline = pos + 1;
				}
				
				std::string currentLineText;
				if (lastNewline > 0) {
					currentLineText = preText.substr(lastNewline);
				} else {
					currentLineText = preText;
				}
				
				float cursorX = 3 + nvgTextBounds(args.vg, 0, 0, currentLineText.c_str(), NULL, NULL);
				float cursorY = cursorLine * lineHeight - scrollPos;
				
                NVGcolor highlightColor = color;
                highlightColor.a = 0.5;
				if (cursorY + lineHeight >= 0 && cursorY <= box.size.y) {
					nvgBeginPath(args.vg);
					nvgMoveTo(args.vg, cursorX, cursorY + 3);
					nvgLineTo(args.vg, cursorX, cursorY + lineHeight + 3);
					nvgStrokeColor(args.vg, highlightColor);
					nvgStrokeWidth(args.vg, 2.0);
					nvgStroke(args.vg);
				}
			}

			if (selectionStart != selectionEnd) {
				int start = std::min(selectionStart, selectionEnd);
				int end = std::max(selectionStart, selectionEnd);
				std::string text = getText();
				std::vector<std::string> wrappedLines = wrapText(text, box.size.x - 6);

				int startLine = 0, endLine = 0;
				int currentCharCount = 0;

				for (int i = 0; i < wrappedLines.size(); i++) {
					currentCharCount += wrappedLines[i].length() + 1;
					if (currentCharCount > start && startLine == 0) {
						startLine = i;
					}
					if (currentCharCount >= end) {
						endLine = i;
						break;
					}
				}

				float y = -scrollPos;
				for (int i = startLine; i <= endLine; i++) {
					float startX = (i == startLine) ? nvgTextBounds(args.vg, 0, 0, text.substr(0, start).c_str(), NULL, NULL) : 0;
					float width = (i == endLine) ? nvgTextBounds(args.vg, 0, 0, text.substr(start, end - start).c_str(), NULL, NULL) : nvgTextBounds(args.vg, 0, 0, wrappedLines[i].c_str(), NULL, NULL);

					nvgBeginPath(args.vg);
					nvgFillColor(args.vg, nvgRGBA(150, 150, 150, 100));
					nvgRect(args.vg, startX, y + 3, width, lineHeight);
					nvgFill(args.vg);
					y += lineHeight;
				}
			}

			bndSetFont(APP->window->uiFont->handle);
		}
 
    }
}

void GlabTextField::onSelectKey(const event::SelectKey& e) {
    if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
        if (e.key == GLFW_KEY_TAB && (e.mods & RACK_MOD_MASK) == 0) {
            e.consume(this);
            insertSpaces(4);
            return;
        }
        
        if (e.key == GLFW_KEY_UP || e.key == GLFW_KEY_DOWN) {
            float contentHeight = getContentHeight();
            float viewportHeight = box.size.y;
            
            if (contentHeight > viewportHeight) {
                if (e.key == GLFW_KEY_UP) {
                    scrollPos = std::max(0.f, scrollPos - lineHeight);
                } else {
                    float maxScroll = contentHeight - viewportHeight;
                    scrollPos = std::min(maxScroll, scrollPos + lineHeight);
                }
            }
        }
    }
    LedDisplayTextField::onSelectKey(e);
}

void GlabTextField::onHoverScroll(const event::HoverScroll& e) {
    float contentHeight = getContentHeight();
    float viewportHeight = box.size.y;
    
    if (contentHeight > viewportHeight) {
        float maxScroll = contentHeight - viewportHeight;
        scrollPos = clamp(scrollPos - e.scrollDelta.y, 0.f, maxScroll);
        e.consume(this);
    }
}

void GlabTextField::insertSpaces(int numSpaces) {
    std::string spaces(numSpaces, ' ');
    insertText(spaces);
}

float GlabTextField::getContentHeight() {
    int numLines = 1;
    for (char c : getText()) {
        if (c == '\n') numLines++;
    }
    return numLines * lineHeight;
}

std::vector<std::string> GlabTextField::wrapText(const std::string& text, float maxWidth) {
    std::vector<std::string> lines;
    std::string currentLine;
    std::string word;
    float currentWidth = 0;
    
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == ' ' || text[i] == '\n' || i == text.length() - 1) {
            if (i == text.length() - 1 && text[i] != ' ' && text[i] != '\n') {
                word += text[i];
            }
            
            float wordWidth = nvgTextBounds(APP->window->vg, 0, 0, word.c_str(), NULL, NULL);
            
            if (currentWidth + wordWidth > maxWidth) {
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                    currentLine = "";
                    currentWidth = 0;
                }
                
                if (wordWidth > maxWidth) {
                    std::string partialWord;
                    for (char c : word) {
                        std::string testStr = partialWord + c;
                        float testWidth = nvgTextBounds(APP->window->vg, 0, 0, testStr.c_str(), NULL, NULL);
                        if (testWidth > maxWidth) {
                            if (!partialWord.empty()) {
                                lines.push_back(partialWord);
                            }
                            partialWord = c;
                        } else {
                            partialWord += c;
                        }
                    }
                    if (!partialWord.empty()) {
                        currentLine = partialWord;
                        currentWidth = nvgTextBounds(APP->window->vg, 0, 0, partialWord.c_str(), NULL, NULL);
                    }
                } else {
                    currentLine = word;
                    currentWidth = wordWidth;
                }
            } else {
                if (!currentLine.empty()) {
                    currentLine += " ";
                    currentWidth += nvgTextBounds(APP->window->vg, 0, 0, " ", NULL, NULL);
                }
                currentLine += word;
                currentWidth += wordWidth;
            }
            
            if (text[i] == '\n') {
                lines.push_back(currentLine);
                currentLine = "";
                currentWidth = 0;
            }
            
            word = "";
        } else {
            word += text[i];
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

void GlabVertexField::step() {
	LedDisplayTextField::step();
	if (module && module->requestVertexCompile) {
		if (getText() != module->vertexShaderText) {
			setText(module->vertexShaderText);
		}
		module->requestVertexCompile = false;
	}
}

void GlabVertexField::onChange(const ChangeEvent& e) {
	if (module) {
		module->vertexShaderText = getText();
		module->isDirty = true;
		module->requestVertexCompile = true;
	}
}

void GlabFragmentField::step() {
	LedDisplayTextField::step();
	if (module && module->requestFragmentCompile) {
		if (getText() != module->fragmentShaderText) {
			setText(module->fragmentShaderText);
		}
		module->requestFragmentCompile = false;
	}
}

void GlabFragmentField::onChange(const ChangeEvent& e) {
	if (module) {
		module->fragmentShaderText = getText();
		module->isDirty = true;
		module->requestFragmentCompile = true;
	}
}

void GlabErrorField::step() {
	LedDisplayTextField::step();
	if (module && getText() != module->errorText) {
		setText(module->errorText);
	}
}

void GlabDisplay::setModule(Glab* module, bool isVertex) {
	if (isVertex) {
		GlabVertexField* textField = createWidget<GlabVertexField>(Vec(0, 0));
		textField->box.size = box.size;
		textField->multiline = true;
		textField->module = module;
		if (module) {
			textField->setText(module->vertexShaderText);
		}
		addChild(textField);
	}
	else {
		GlabFragmentField* textField = createWidget<GlabFragmentField>(Vec(0, 0));
		textField->box.size = box.size;
		textField->multiline = true;
		textField->module = module;
		if (module) {
			textField->setText(module->fragmentShaderText);
		}
		addChild(textField);
	}
}

void GlabErrorDisplay::setModule(Glab* module) {
	GlabErrorField* textField = createWidget<GlabErrorField>(Vec(0, 0));
	textField->box.size = box.size;
	textField->multiline = true;
	textField->module = module;
	if (module) {
		textField->setText(module->errorText);
	}
	addChild(textField);
}

Glab::Glab() {
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

void Glab::process(const ProcessArgs& args) {
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
			publishToGlib();
			wasPublishPressed = true;
		}
	} else {
		wasPublishPressed = false;
	}

	updatePublishLight();
}

void Glab::onReset() {
	vertexShaderValid = false;
	fragmentShaderValid = false;
	isDirty = false;
	subscription.isValid = false;
	errorText = "";
	publishTrigger.reset();
	updatePublishLight();
}

void Glab::updatePublishLight() {
	lights[LIGHT_PUBLISH + 0].setBrightness(!vertexShaderValid || !fragmentShaderValid ? 1.f : 0.f);
	lights[LIGHT_PUBLISH + 1].setBrightness(!isDirty && vertexShaderValid && fragmentShaderValid ? 1.f : 0.f);
	lights[LIGHT_PUBLISH + 2].setBrightness(isDirty ? 1.f : 0.f);
}

void Glab::publishToGlib() {
	if (!vertexShaderValid || !fragmentShaderValid) {
		errorText = "Cannot publish: Shaders must be valid first!";
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
		}
	}
	
	int64_t targetGlibId;
	if (subscription.isValid) {
		targetGlibId = subscription.glibId;
	} else {
		targetGlibId = id;
	}
	
	shaderLib.registerGlib(targetGlibId);
	
	if (!isUpdating) {
		time_t now = time(nullptr);
		char timestamp[32];
		strftime(timestamp, sizeof(timestamp), "%H%M%S", localtime(&now));
		
		const std::vector<ShaderPair>* shaders = shaderLib.getShadersForGlib(targetGlibId);
		int shaderCount = shaders ? shaders->size() : 0;
		newShader.name = string::f("glab_%d_%s", shaderCount + 1, timestamp);
	}
	
	newShader.vertexSource = vertexShaderText;
	newShader.fragmentSource = fragmentShaderText;
	newShader.isValid = true;
	
	try {
		shaderLib.addShader(targetGlibId, newShader);
		
		isDirty = false;
		errorText = string::f("Successfully published shader as '%s'", newShader.name.c_str());
		
		const std::vector<ShaderPair>* shaders = shaderLib.getShadersForGlib(targetGlibId);
		if (shaders) {
			int newIndex = shaders->size() - 1;
			shaderLib.subscribeToShader(id, targetGlibId, newIndex);
			subscription = *shaderLib.getSubscription(id);
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

json_t* Glab::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "vertexShader", json_string(vertexShaderText.c_str()));
	json_object_set_new(rootJ, "fragmentShader", json_string(fragmentShaderText.c_str()));
	return rootJ;
}

void Glab::dataFromJson(json_t* rootJ) {
	json_t* vertexShaderJ = json_object_get(rootJ, "vertexShader");
	json_t* fragmentShaderJ = json_object_get(rootJ, "fragmentShader");
	if (vertexShaderJ) vertexShaderText = json_string_value(vertexShaderJ);
	if (fragmentShaderJ) fragmentShaderText = json_string_value(fragmentShaderJ);
}

void Glab::onShaderSubscribe(int64_t glibId, int shaderIndex) {
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

void GlabWidget::step() {
	ModuleWidget::step();
	
	Glab* module = dynamic_cast<Glab*>(this->module);
	if (!module) return;

	if (module->requestVertexCompile) {
		GLuint shader = gl::compileShader(module->vertexShaderText, GL_VERTEX_SHADER);
		module->vertexShaderValid = shader != 0;
		if (shader) glDeleteShader(shader);
		module->lights[Glab::LIGHT_COMP_V].setBrightness(module->vertexShaderValid ? 1.f : 0.f);
		module->isDirty = true;
	}
	
	if (module->requestFragmentCompile) {
		GLuint shader = gl::compileShader(module->fragmentShaderText, GL_FRAGMENT_SHADER);
		module->fragmentShaderValid = shader != 0;
		if (shader) glDeleteShader(shader);
		module->lights[Glab::LIGHT_COMP_F].setBrightness(module->fragmentShaderValid ? 1.f : 0.f);
		module->isDirty = true;
	}
}

void GlabWidget::appendContextMenu(Menu* menu) {
	Glab* module = dynamic_cast<Glab*>(this->module);
	if (!module) return;

	menu->addChild(new MenuSeparator);
	menu->addChild(createMenuLabel("Shader Source"));
	addShaderMenuItems(menu, module);
}

GlabWidget::GlabWidget(Glab* module) {
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
		//INFO("Configuring publish button for module %lld", module->id);
	}
	addParam(publishButton);
	
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.427, 115.864)), module, Glab::OUTPUT_VERT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(193.773, 115.864)), module, Glab::OUTPUT_FRAG));
	addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(9.325, 100.013)), module, Glab::LIGHT_COMP_V));
	addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(193.773, 100.012)), module, Glab::LIGHT_COMP_F));

	GlabDisplay* vertexDisplay = createWidget<GlabDisplay>(mm2px(Vec(18.855, 5.49)));
	vertexDisplay->box.size = mm2px(Vec(80.0, 97.607));
	vertexDisplay->setModule(module, true);
	addChild(vertexDisplay);

	GlabDisplay* fragmentDisplay = createWidget<GlabDisplay>(mm2px(Vec(104.345, 25.403)));
	fragmentDisplay->box.size = mm2px(Vec(80.0, 97.607));
	fragmentDisplay->setModule(module, false);
	addChild(fragmentDisplay);

	GlabErrorDisplay* errorDisplay = createWidget<GlabErrorDisplay>(mm2px(Vec(115.345, 5.49)));
	errorDisplay->box.size = mm2px(Vec(69.0, 14.423));
	errorDisplay->setModule(module);
	addChild(errorDisplay);

	addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(109.009, 22.625)), module, Glab::LIGHT_PUBLISH));
}

Model* modelGlab = createModel<Glab, GlabWidget>("glab");
