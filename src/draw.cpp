#include "plugin.hpp"
#include "context.hpp"
#include "logger.hpp"
#include <widget/FramebufferWidget.hpp>
#include <widget/OpenGlWidget.hpp>
#include <string>
#include <fstream>
#include <sstream>

void checkGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        WARN("OpenGL error at %s: 0x%x", location, err);
    }
}

std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        WARN("Failed to open shader file: %s", filePath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        WARN("Shader compilation failed: %s", infoLog);
        return 0;
    }
    return shader;
}

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

	float audioBuffer[512] = {0.f};

	void process(const ProcessArgs& args) override {
		if (inputs[IN_1_INPUT].isConnected()) {
			float in = inputs[IN_1_INPUT].getVoltage() / 5.f;

			for (int i = 511; i > 0; i--) {
				audioBuffer[i] = audioBuffer[i-1];
			}
			audioBuffer[0] = in;
		}
	}
};


struct GLCanvasWidget : rack::widget::OpenGlWidget {
	GLuint shaderProgram = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;
	
	GLint posAttrib = -1;
	GLint texCoordAttrib = -1;
	
	Draw* module = nullptr;
	
	GLCanvasWidget() {
		box.size = math::Vec(195, 160);
	}
	
	void setModule(Draw* mod) {
		module = mod;
	}
	
	void createShaderProgram() {
		INFO("OpenGL Version: %s", glGetString(GL_VERSION));
		
		std::string vertSource = readShaderFile(asset::plugin(pluginInstance, "res/shaders/basic.vert"));
		std::string fragSource = readShaderFile(asset::plugin(pluginInstance, "res/shaders/basic.frag"));
		
		if (vertSource.empty() || fragSource.empty()) {
			WARN("Failed to load shader sources");
			return;
		}
		
		GLuint vertShader = compileShader(vertSource, GL_VERTEX_SHADER);
		GLuint fragShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
		
		if (!vertShader) {
			WARN("Failed to compile vertex shader!");
			return;
		}

		if (!fragShader) {
			WARN("Failed to compile fragment shader!");
			return;
		}
		
		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertShader);
		glAttachShader(shaderProgram, fragShader);
		glLinkProgram(shaderProgram);
		
		GLint ok;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
		if (!ok) {
			GLchar infoLog[512];
			glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
			WARN("Shader program linking failed: %s", infoLog);
			return;
		}
		
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		
		glUseProgram(shaderProgram);
		posAttrib = glGetAttribLocation(shaderProgram, "vs_Pos");
		texCoordAttrib = glGetAttribLocation(shaderProgram, "vs_TexCoord");
		INFO("Attribute locations - vs_Pos: %d, vs_TexCoord: %d", posAttrib, texCoordAttrib);
		
		checkGLError("createShaderProgram");
	}
	
	void setupGeometry() {
		float vertices[] = {
			-0.8f,  0.8f, 0.0f, 0.0f, 1.0f,
            0.8f,  0.8f, 0.0f, 1.0f, 1.0f,
			-0.8f, -0.8f, 0.0f, 0.0f, 0.0f,
			 0.8f, -0.8f, 0.0f, 1.0f, 0.0f
		};
		unsigned int indices[] = {
			0, 1, 2,
			1, 3, 2
		};
		
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		checkGLError("setupGeometry");
	}
	
	void step() override {
		dirty = true;
		OpenGlWidget::step();
	}
	
	void drawFramebuffer() override {
		if (!shaderProgram) {
			createShaderProgram();
			if (shaderProgram) {
				setupGeometry();
			} else {
				return;
			}
		}
		
		math::Vec fbSize = getFramebufferSize();
		glViewport(0.0, 0.0, fbSize.x, fbSize.y);
		glClearColor(0.2, 0.2, 0.2, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		glUseProgram(shaderProgram);
		checkGLError("glUseProgram");
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		
		if (posAttrib >= 0) {
			glEnableVertexAttribArray(posAttrib);
			glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		}
		
		if (texCoordAttrib >= 0) {
			glEnableVertexAttribArray(texCoordAttrib);
			glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		if (posAttrib >= 0) glDisableVertexAttribArray(posAttrib);
		if (texCoordAttrib >= 0) glDisableVertexAttribArray(texCoordAttrib);
		
		checkGLError("draw");
	}
	
	~GLCanvasWidget() {
		if (shaderProgram) glDeleteProgram(shaderProgram);
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);
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

		GLCanvasWidget* canvas = createWidget<GLCanvasWidget>(mm2px(Vec(0.0, 13.039)));
		canvas->setModule(module);
		addChild(canvas);
	}
};

Model* modelDraw = createModel<Draw, DrawWidget>("draw");
