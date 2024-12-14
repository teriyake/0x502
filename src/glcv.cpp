#include "plugin.hpp"
#include "gl_utils.hpp"
#include <widget/OpenGlWidget.hpp>
#include <string>
#include <fstream>
#include <sstream>

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
    float chaos = 0.f;
    float scale = 1.f;
    float timeSpace = 0.f;
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

        chaos = params[PARAM_CHAOS].getValue();
        scale = params[PARAM_SCALE].getValue();
        timeSpace = inputs[Glcv::INPUT_TS].getVoltage() > 1.0f ? 1.0f : 0.0f;
	}
};

struct GLCVProcessor : rack::widget::OpenGlWidget {
    GLuint shaderProgram = 0;
    GLuint VBO = 0;
	GLuint EBO = 0;
    GLuint frameBuffer = 0;
    GLuint renderTexture = 0;
    float startTime = 0.f;
    
	GLint posAttrib = -1;
    GLint timeUniform = -1;
    GLint chaosUniform = -1;
    GLint scaleUniform = -1;
    GLint clockTimeUniform = -1;
    GLint timeSpaceUniform = -1;
    
    Glcv* module = nullptr;
    
    GLCVProcessor() {
        box.size = math::Vec(1, 1);
        startTime = rack::system::getTime();
    }
    
    void setModule(Glcv* mod) {
        module = mod;
    }
    
    void createShaderProgram() {
        std::string vertSource = gl::readShaderFile(asset::plugin(pluginInstance, "res/shaders/cv.vert"));
        std::string fragSource = gl::readShaderFile(asset::plugin(pluginInstance, "res/shaders/cv.frag"));
        
        GLuint vertShader = gl::compileShader(vertSource, GL_VERTEX_SHADER);
        GLuint fragShader = gl::compileShader(fragSource, GL_FRAGMENT_SHADER);
        
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
        timeUniform = glGetUniformLocation(shaderProgram, "u_Time");
        chaosUniform = glGetUniformLocation(shaderProgram, "u_Chaos");
        scaleUniform = glGetUniformLocation(shaderProgram, "u_Scale");
        clockTimeUniform = glGetUniformLocation(shaderProgram, "u_ClockTime");
        timeSpaceUniform = glGetUniformLocation(shaderProgram, "u_TimeSpace");
        
        glGenFramebuffers(1, &frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        
        glGenTextures(1, &renderTexture);
        glBindTexture(GL_TEXTURE_2D, renderTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);
        
        gl::checkError("createShaderProgram");
    }

	void setupGeometry() {
		float vertices[] = {
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f
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
		
        gl::checkError("setupGeometry");
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

        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glViewport(0, 0, 1, 1);
        
        glUseProgram(shaderProgram);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		if (posAttrib >= 0) {
			glEnableVertexAttribArray(posAttrib);
			glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		}
        
        float currentTime = rack::system::getTime() - startTime;
		if (timeUniform >= 0) glUniform1f(timeUniform, currentTime);
		if (module) {
			if (chaosUniform >= 0) glUniform1f(chaosUniform, module->chaos);
			if (scaleUniform >= 0) glUniform1f(scaleUniform, module->scale);
			if (clockTimeUniform >= 0) glUniform1f(clockTimeUniform, module->clockTime);
			if (timeSpaceUniform >= 0) glUniform1f(timeSpaceUniform, module->timeSpace);
		}
        
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		if (posAttrib >= 0) glDisableVertexAttribArray(posAttrib);
        
        float result[4];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);
        
        // map [0,1] to [-10V, 10V]
        module->outputs[Glcv::OUTPUT_1].setVoltage((result[0] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_2].setVoltage((result[1] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_3].setVoltage((result[2] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_4].setVoltage((result[3] * 2.0f - 1.0f) * 10.f);

        gl::checkError("drawFramebuffer");
	}
	
	~GLCVProcessor() {
		if (shaderProgram) glDeleteProgram(shaderProgram);
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);
	}
};


struct GlcvWidget : ModuleWidget {
	GlcvWidget(Glcv* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glcv.svg")));

		GLCVProcessor* processor = new GLCVProcessor();
		processor->setModule(module);
		addChild(processor);

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
