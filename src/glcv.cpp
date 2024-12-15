#include "plugin.hpp"
#include "gl_utils.hpp"
#include "shader_manager.hpp"
#include <widget/OpenGlWidget.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include "shader_menu.hpp"

struct GLCVProcessor;

class Glcv;
void updateGlcvProcessor(Glcv* module);

struct Glcv : Module, ShaderSubscriber {
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

    int64_t subscribedGlibId = -1;
    int subscribedShaderIndex = -1;
    GLCVProcessor* processor = nullptr;

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
        timeSpace = inputs[INPUT_TS].getVoltage() > 1.0f ? 1.0f : 0.0f;
	}

	void onReset() override {
        subscribedGlibId = -1;
        subscribedShaderIndex = -1;
        updateGlcvProcessor(this);
    }

    void onShaderSubscribe(int64_t glibId, int shaderIndex) override {
        //INFO("GLCV module %lld subscribing to Glib %lld, shader %d", (long long)id, (long long)glibId, shaderIndex);
        
        subscribedGlibId = glibId;
        subscribedShaderIndex = shaderIndex;
        
        auto& shaderLib = SharedShaderLibrary::getInstance();
        const ShaderSubscription* sub = shaderLib.getSubscription(id);
        if (sub) {
            //INFO("Verified subscription: Glib %lld, Shader %d, Valid: %d", (long long)sub->glibId, sub->shaderIndex, sub->isValid);
            
            if (sub->isValid) {
                updateGlcvProcessor(this);
            } else {
                WARN("Invalid subscription for GLCV module %lld", (long long)id);
            }
        } else {
            WARN("Failed to verify subscription for GLCV module %lld", (long long)id);
        }
    }
};

struct GLCVProcessor : rack::widget::OpenGlWidget {
    GLuint shaderProgram = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint frameBuffer = 0;
    GLuint renderTexture = 0;
    float startTime = 0.f;
    bool dirty = true;
    bool initialized = false;
    
    GLint posAttrib = -1;
    GLint timeUniform = -1;
    GLint chaosUniform = -1;
    GLint scaleUniform = -1;
    GLint clockTimeUniform = -1;
    GLint timeSpaceUniform = -1;
    
    Glcv* module = nullptr;
    
    GLCVProcessor() {
        box.size = math::Vec(1, 1);
        visible = true;
        startTime = rack::system::getTime();
    }
    
    void setModule(Glcv* mod) {
        module = mod;
        if (module) {
            //INFO("GLCVProcessor: Setting module with ID %lld", (long long)module->id);
            module->processor = this;
            dirty = true;
        }
    }

    void createShaderProgram() {
        if (!initialized) {
            WARN("Cannot create shader program - OpenGL context not initialized");
            return;
        }

        if (!module) {
            WARN("No module attached to GLCVProcessor");
            return;
        }
        
        int64_t moduleId = module->id;
        //INFO("Creating shader program for GLCV module %lld", (long long)moduleId);
        
        //INFO("Module subscription state: Glib %lld, Shader %d", (long long)module->subscribedGlibId, module->subscribedShaderIndex);
        
        auto& shaderLib = SharedShaderLibrary::getInstance();
        //INFO("Attempting to get subscription for module %lld", moduleId);
        const ShaderSubscription* sub = shaderLib.getSubscription(moduleId);
        if (!sub) {
            //INFO("No subscription found for module %lld", moduleId);
            dirty = false;
            return;
        }
        
        //INFO("Found subscription: Glib %lld, Shader %d, Valid: %d", (long long)sub->glibId, sub->shaderIndex, sub->isValid);
        
        if (sub->glibId != module->subscribedGlibId || sub->shaderIndex != module->subscribedShaderIndex) {
            WARN("Subscription mismatch: Module expects Glib %lld, Shader %d but got Glib %lld, Shader %d",
                (long long)module->subscribedGlibId, module->subscribedShaderIndex,
                (long long)sub->glibId, sub->shaderIndex);
            dirty = false;
            return;
        }
        
        if (!sub->isValid) {
            WARN("Invalid subscription for module %lld", moduleId);
            dirty = false;
            return;
        }
        
        //INFO("Attempting to get shader for module %lld", moduleId);
        const ShaderPair* shaderPair = shaderLib.getShaderForModule(moduleId);
        if (!shaderPair) {
            WARN("No shader pair found for module %lld", moduleId);
            dirty = false;
            return;
        }
        
        if (!shaderPair->isValid) {
            WARN("Invalid shader pair for module %lld: %s", 
                moduleId, shaderPair->errorLog.c_str());
            dirty = false;
            return;
        }
        
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
        
        GLuint vertShader = 0, fragShader = 0;
        try {
            vertShader = gl::compileShader(shaderPair->vertexSource, GL_VERTEX_SHADER);
            if (!vertShader) {
                WARN("Failed to compile vertex shader");
                dirty = false;
                return;
            }
            
            fragShader = gl::compileShader(shaderPair->fragmentSource, GL_FRAGMENT_SHADER);
            if (!fragShader) {
                WARN("Failed to compile fragment shader");
                glDeleteShader(vertShader);
                dirty = false;
                return;
            }
            
            shaderProgram = glCreateProgram();
            if (!shaderProgram) {
                WARN("Failed to create shader program");
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                dirty = false;
                return;
            }
            
            glAttachShader(shaderProgram, vertShader);
            glAttachShader(shaderProgram, fragShader);
            glLinkProgram(shaderProgram);
            
            GLint ok;
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
            if (!ok) {
                GLchar infoLog[512];
                glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
                WARN("Shader program linking failed: %s", infoLog);
                glDeleteProgram(shaderProgram);
                shaderProgram = 0;
                dirty = false;
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
            
            setupFramebuffer();
            
            setupGeometry();
            
            gl::checkError("createShaderProgram");
            //INFO("Successfully created shader program for module %lld", (long long)moduleId);
            dirty = false;
        }
        catch (const std::exception& e) {
            WARN("Exception during shader program creation: %s", e.what());
            if (vertShader) glDeleteShader(vertShader);
            if (fragShader) glDeleteShader(fragShader);
            if (shaderProgram) {
                glDeleteProgram(shaderProgram);
                shaderProgram = 0;
            }
            dirty = false;
        }
    }

    void setupFramebuffer() {
        if (frameBuffer) {
            glDeleteFramebuffers(1, &frameBuffer);
            frameBuffer = 0;
        }
        if (renderTexture) {
            glDeleteTextures(1, &renderTexture);
            renderTexture = 0;
        }
        
        glGenFramebuffers(1, &frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        
        glGenTextures(1, &renderTexture);
        glBindTexture(GL_TEXTURE_2D, renderTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            WARN("Framebuffer is not complete!");
            throw std::runtime_error("Failed to create framebuffer");
        }
        
        gl::checkError("setupFramebuffer");
    }

    void setupGeometry();
    void step() override;
    void drawFramebuffer() override;
    ~GLCVProcessor();
};

void GLCVProcessor::setupGeometry() {
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

void GLCVProcessor::step() {
    if (!initialized) {
        INFO("GLCVProcessor: Initializing OpenGL context");
        OpenGlWidget::step();
        initialized = true;
        return;
    }
    
    if (dirty) {
        //INFO("GLCVProcessor: Creating shader program due to dirty flag");
        createShaderProgram();
    }
    
    OpenGlWidget::step();
}

void GLCVProcessor::drawFramebuffer() {
    if (!initialized) {
        INFO("GLCVProcessor: OpenGL context not initialized yet");
        return;
    }

    if (!shaderProgram) {
        if (dirty) {
            //INFO("GLCV: Creating shader program in drawFramebuffer");
            createShaderProgram();
            if (!shaderProgram) {
                //INFO("GLCV: No shader program available, clearing to black");
                glClearColor(0.0, 0.0, 0.0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                return;
            }
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
    } else {
        WARN("GLCV: Position attribute not found in shader");
    }
    
    float currentTime = rack::system::getTime() - startTime;
    if (timeUniform >= 0) {
        glUniform1f(timeUniform, currentTime);
    } else {
        WARN("GLCV: Time uniform not found in shader");
    }

    if (module) {
        if (chaosUniform >= 0) glUniform1f(chaosUniform, module->chaos);
        if (scaleUniform >= 0) glUniform1f(scaleUniform, module->scale);
        if (clockTimeUniform >= 0) glUniform1f(clockTimeUniform, module->clockTime);
        if (timeSpaceUniform >= 0) glUniform1f(timeSpaceUniform, module->timeSpace);
    } else {
        WARN("GLCV: No module attached in drawFramebuffer");
    }
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    if (posAttrib >= 0) glDisableVertexAttribArray(posAttrib);
    
    float result[4];
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);
    
    // map [0,1] to [-10V, 10V]
    if (module) {
        module->outputs[Glcv::OUTPUT_1].setVoltage((result[0] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_2].setVoltage((result[1] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_3].setVoltage((result[2] * 2.0f - 1.0f) * 10.f);
        module->outputs[Glcv::OUTPUT_4].setVoltage((result[3] * 2.0f - 1.0f) * 10.f);
        /*
        INFO("GLCV: Output voltages: %.2f, %.2f, %.2f, %.2f", 
            module->outputs[Glcv::OUTPUT_1].getVoltage(),
            module->outputs[Glcv::OUTPUT_2].getVoltage(),
            module->outputs[Glcv::OUTPUT_3].getVoltage(),
            module->outputs[Glcv::OUTPUT_4].getVoltage());
        */
    }

    gl::checkError("drawFramebuffer");
}

GLCVProcessor::~GLCVProcessor() {
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (frameBuffer) glDeleteFramebuffers(1, &frameBuffer);
    if (renderTexture) glDeleteTextures(1, &renderTexture);
}

void updateGlcvProcessor(Glcv* module) {
    if (!module) {
        WARN("GLCV: Null module in updateGlcvProcessor");
        return;
    }
    
    if (!module->processor) {
        WARN("GLCV: No processor attached to module %lld", (long long)module->id);
        return;
    }
    
    auto& shaderLib = SharedShaderLibrary::getInstance();
    const ShaderSubscription* sub = shaderLib.getSubscription(module->id);
    if (sub && sub->isValid) {
        //INFO("GLCV: Marking processor as dirty for shader update (Module %lld, Glib %lld, Shader %d)", (long long)module->id, (long long)sub->glibId, sub->shaderIndex);
        module->processor->dirty = true;
    } else {
        WARN("GLCV: Cannot update processor - invalid subscription state for module %lld", 
            (long long)module->id);
    }
}

struct GlcvWidget : ModuleWidget {
	Glcv* module;

	GlcvWidget(Glcv* module) {
		this->module = module;
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

	void appendContextMenu(Menu* menu) override {
		if (module) {
			menu->addChild(new MenuSeparator);
			menu->addChild(createMenuLabel("Shader Source"));
			
			addShaderMenuItems(menu, module);
		}
	}
};

Model* modelGlcv = createModel<Glcv, GlcvWidget>("glcv");
