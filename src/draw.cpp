#include "plugin.hpp"
#include "context.hpp"
#include "logger.hpp"
#include <widget/FramebufferWidget.hpp>
#include <widget/OpenGlWidget.hpp>
#include <string>
#include <fstream>
#include <sstream>

std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        WARN("Shader compilation failed: %s", infoLog);
    }
    INFO("Shader compiled.");
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
	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;
	float startTime = 0.f;
	
	Draw* module;
	
    GLCanvasWidget() {
        startTime = rack::system::getTime();
    }
	
	void createShaderProgram() {
		std::string vertSource = readShaderFile(asset::plugin(pluginInstance, "res/shaders/basic.vert"));
		std::string fragSource = readShaderFile(asset::plugin(pluginInstance, "res/shaders/basic.frag"));
		
		GLuint vertShader = compileShader(vertSource, GL_VERTEX_SHADER);
		GLuint fragShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
		
		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertShader);
		glAttachShader(shaderProgram, fragShader);
		glLinkProgram(shaderProgram);
		
		GLint success;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			GLchar infoLog[512];
			glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
			WARN("Shader program linking failed: %s", infoLog);
        } else {
            INFO("Shader program linked.");
        }
		
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
	}
	
	void setupGeometry() {
		float vertices[] = {
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
		};
		unsigned int indices[] = {
			0, 1, 3,
			1, 2, 3
		};
		
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		
		glBindVertexArray(VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		
		GLint posAttrib = glGetAttribLocation(shaderProgram, "vs_Pos");
		GLint texCoordAttrib = glGetAttribLocation(shaderProgram, "vs_TexCoord");
		
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(posAttrib);
		glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(texCoordAttrib);
	}
	
	void step() override {
		box.size = math::Vec(195, 160);
		dirty = true;
		OpenGlWidget::step();
	}
	
	void drawFramebuffer() override {
		if (!shaderProgram) {
			createShaderProgram();
			setupGeometry();
		}
		
		math::Vec fbSize = getFramebufferSize();
		glViewport(0.0, 0.0, fbSize.x, fbSize.y);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		glUseProgram(shaderProgram);
		
		float currentTime = rack::system::getTime() - startTime;
		glUniform1f(glGetUniformLocation(shaderProgram, "u_Time"), currentTime);
		glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), fbSize.x, fbSize.y);
		
		if (module) {
			glUniform1fv(glGetUniformLocation(shaderProgram, "u_AudioData"), 512, module->audioBuffer);
		}
		
		float projection[16] = {
			2.0f/fbSize.x, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f/fbSize.y, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 1.0f
		};
		
		float model[16] = {
			fbSize.x/2, 0.0f, 0.0f, 0.0f,
			0.0f, fbSize.y/2, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			fbSize.x/2, fbSize.y/2, 0.0f, 1.0f
		};
		
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Proj"), 1, GL_FALSE, projection);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, model);
		
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
	
	~GLCanvasWidget() {
		if (shaderProgram) glDeleteProgram(shaderProgram);
		if (VAO) glDeleteVertexArrays(1, &VAO);
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

		// mm2px(Vec(66.04, 55.88))

		GLCanvasWidget* canvas = createWidget<GLCanvasWidget>(mm2px(Vec(0.0, 13.039)));
		canvas->module = module;
		addChild(canvas);
	}
};

Model* modelDraw = createModel<Draw, DrawWidget>("draw");
