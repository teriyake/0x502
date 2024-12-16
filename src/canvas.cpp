#include "plugin.hpp"
#include "context.hpp"
#include "logger.hpp"
#include "shader_manager.hpp"
#include <widget/FramebufferWidget.hpp>
#include <widget/OpenGlWidget.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include "shader_menu.hpp"
#include "gl_utils.hpp"

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

struct Canvas;

struct GLCanvasWidget : rack::widget::OpenGlWidget {
	GLuint shaderProgram = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;
	GLuint frameBuffer = 0;
	GLuint renderTexture = 0;
	float startTime = 0.f;
	bool dirty = true;
	bool initialized = false;
	
	GLint posAttrib = -1;
	GLint texCoordAttrib = -1;
	GLint timeUniform = -1;
	GLint resolutionUniform = -1;
	GLint audioData1Uniform = -1;
	GLint audioData2Uniform = -1;
	GLint projUniform = -1;
	GLint modelUniform = -1;
	GLint trigger1Uniform = -1;
	GLint trigger2Uniform = -1;
	GLint timeWarp1Uniform = -1;
	GLint timeWarp2Uniform = -1;
	
	Canvas* module = nullptr;
	
	GLCanvasWidget() {
		box.size = math::Vec(487.5, 400.0);
		startTime = rack::system::getTime();
		
		shaderProgram = 0;
		VBO = 0;
		EBO = 0;
		frameBuffer = 0;
		renderTexture = 0;
		dirty = true;
		initialized = false;
		
		posAttrib = -1;
		texCoordAttrib = -1;
		timeUniform = -1;
		resolutionUniform = -1;
		audioData1Uniform = -1;
		audioData2Uniform = -1;
		projUniform = -1;
		modelUniform = -1;
		trigger1Uniform = -1;
		trigger2Uniform = -1;
		timeWarp1Uniform = -1;
		timeWarp2Uniform = -1;
		
		module = nullptr;

		visible = true;
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
	
	void setModule(Canvas* mod);
	void createShaderProgram();
	void setupGeometry();
	void step() override;
	void drawFramebuffer() override;
	~GLCanvasWidget();
};

struct Canvas : Module, ShaderSubscriber {
	enum ParamId {
		PARAM_TIME_1,
		PARAM_TIME_2,
		PARAMS_LEN
	};
	enum InputId {
		INPUT_1,
		INPUT_TRIG_1,
		INPUT_2,
		INPUT_TRIG_2,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_1,
		OUTPUT_2,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	int ch1 = 0;
	int ch2 = 0;
	float audioBuffer1[256] = {0.f};
	float audioBuffer2[256] = {0.f};
	float smoothingFactor = 0.3f;
	float timeWarp1 = 0.0f;
	float timeWarp2 = 0.0f;
	float trig1 = 0.0f;
	float trig2 = 0.0f;

	int subscribedGlibId = -1;
	int subscribedShaderIndex = -1;
	GLCanvasWidget* glCanvas = nullptr;

	Canvas() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INPUT_1, "audio input 1");
		configInput(INPUT_TRIG_1, "trigger input 1");
		configInput(INPUT_2, "audio input 2");
		configInput(INPUT_TRIG_2, "trigger input 2");
		configParam(PARAM_TIME_1, -10.0, 10.0, 0.0, "time warp 1 factor", "");
		configParam(PARAM_TIME_2, -10.0, 10.0, 0.0, "time warp 2 factor", "");
		configOutput(OUTPUT_1, "audio output 1");
		configOutput(OUTPUT_2, "audio output 2");

		ch1 = 0;
		ch2 = 0;
		smoothingFactor = 0.3f;
		timeWarp1 = 0.0f;
		timeWarp2 = 0.0f;
		trig1 = 0.0f;
		trig2 = 0.0f;
		subscribedGlibId = -1;
		subscribedShaderIndex = -1;
		glCanvas = nullptr;

		std::fill(audioBuffer1, audioBuffer1 + 256, 0.f);
		std::fill(audioBuffer2, audioBuffer2 + 256, 0.f);
	}

	void process(const ProcessArgs& args) override {
		float in1 = 0.f;
		float in2 = 0.f;

		int channels1 = inputs[INPUT_1].getChannels();
		if (channels1 != this->ch1) {
			this->ch1 = channels1;
		}
		int channels2 = inputs[INPUT_2].getChannels();
		if (channels2 != this->ch2) {
			this->ch2 = channels2;
		}

		if (inputs[INPUT_TRIG_1].isConnected()) {
			trig1 = inputs[INPUT_TRIG_1].getVoltage() > 1.0f ? 1.0f : 0.0f;
		}
		if (inputs[INPUT_TRIG_2].isConnected()) {
			trig2 = inputs[INPUT_TRIG_2].getVoltage() > 1.0f ? 1.0f : 0.0f;
		}

		timeWarp1 = params[PARAM_TIME_1].getValue();
		timeWarp2 = params[PARAM_TIME_2].getValue();

		if (inputs[INPUT_1].isConnected()) {
			in1 = inputs[INPUT_1].getVoltage() / 5.f; // normalize to roughly [-1, 1] for shaders
			outputs[OUTPUT_1].setChannels(channels1);
			outputs[OUTPUT_1].writeVoltages(inputs[INPUT_1].getVoltages());
		}
		if (inputs[INPUT_2].isConnected()) {
			in2 = inputs[INPUT_2].getVoltage() / 5.f; // normalize to roughly [-1, 1] for shaders
			outputs[OUTPUT_2].setChannels(channels2);
			outputs[OUTPUT_2].writeVoltages(inputs[INPUT_2].getVoltages());
		}

		// shift buffer and apply smoothing
		for (int i = 255; i > 0; i--) {
			audioBuffer1[i] = audioBuffer1[i-1] * (1.f - smoothingFactor) + audioBuffer1[i] * smoothingFactor;
			audioBuffer2[i] = audioBuffer2[i-1] * (1.f - smoothingFactor) + audioBuffer2[i] * smoothingFactor;
		}
		audioBuffer1[0] = in1;
		audioBuffer2[0] = in2;
	}

	void onShaderSubscribe(int64_t glibId, int shaderIndex) override {
		//INFO("Canvas module %lld subscribing to Glib %lld, shader %d", (long long)id, (long long)glibId, shaderIndex);
		
		subscribedGlibId = glibId;
		subscribedShaderIndex = shaderIndex;
		auto& shaderLib = SharedShaderLibrary::getInstance();
		const ShaderSubscription* sub = shaderLib.getSubscription(id);
		if (sub) {
			//INFO("Verified subscription: Glib %lld, Shader %d, Valid: %d", (long long)sub->glibId, sub->shaderIndex, sub->isValid);
			
			if (sub->isValid && glCanvas) {
				glCanvas->dirty = true;
				//INFO("Marked canvas as dirty for shader update");
			} else {
				WARN("Invalid subscription for Canvas module %lld", (long long)id);
			}
		} else {
			WARN("Failed to verify subscription for Canvas module %lld", (long long)id);
		}
	}

	void onReset() override {
		subscribedGlibId = -1;
		subscribedShaderIndex = -1;
		if (glCanvas) {
			glCanvas->dirty = true;
		}
	}
};

void GLCanvasWidget::setModule(Canvas* mod) {
	module = mod;
	if (module) {
		module->glCanvas = this;
		//INFO("GLCanvasWidget: Set module %lld", (long long)module->id);
	}
}

void GLCanvasWidget::createShaderProgram() {
	if (!initialized) {
		WARN("Cannot create shader program - OpenGL context not initialized");
		return;
	}

	INFO("OpenGL Version: %s", glGetString(GL_VERSION));
	
	if (!module) {
		WARN("No module attached to GLCanvasWidget");
		return;
	}
	
	int64_t moduleId = static_cast<int64_t>(module->id);
	//INFO("Creating shader program for Canvas module %lld", (long long)moduleId);
	
	//INFO("Module subscription state: Glib %llu, Shader %d", static_cast<unsigned long long>(module->subscribedGlibId), module->subscribedShaderIndex);
	
	auto& shaderLib = SharedShaderLibrary::getInstance();
	//INFO("Attempting to get subscription for module %lld", (long long)moduleId);
	const ShaderSubscription* sub = shaderLib.getSubscription(moduleId);
	if (!sub) {
		WARN("No subscription found for module %lld", (long long)moduleId);
		dirty = false;
		return;
	}
	
	//INFO("Found subscription: Glib %llu, Shader %d, Valid: %d", static_cast<unsigned long long>(sub->glibId), sub->shaderIndex, sub->isValid);
	
	uint64_t moduleGlibId = static_cast<uint64_t>(module->subscribedGlibId);
	uint64_t subGlibId = static_cast<uint64_t>(sub->glibId);
	
	if (moduleGlibId != subGlibId || sub->shaderIndex != module->subscribedShaderIndex) {
		WARN("Subscription mismatch: Module expects Glib %llu, Shader %d but got Glib %llu, Shader %d",
			static_cast<unsigned long long>(moduleGlibId), module->subscribedShaderIndex,
			static_cast<unsigned long long>(subGlibId), sub->shaderIndex);
		
		INFO("Updating module's subscription to match shader library");
		module->subscribedGlibId = sub->glibId;
		module->subscribedShaderIndex = sub->shaderIndex;
	}
	
	if (!sub->isValid) {
		WARN("Invalid subscription for module %lld", (long long)moduleId);
		dirty = false;
        return;
	}
	
	//INFO("Attempting to get shader for module %lld", (long long)moduleId);
	const ShaderPair* shaderPair = shaderLib.getShaderForModule(moduleId);
	if (!shaderPair) {
		WARN("No shader pair found for module %lld", (long long)moduleId);
		dirty = false;
		return;
	}
	
	if (!shaderPair->isValid) {
		WARN("Invalid shader pair for module %lld: %s", 
			moduleId, shaderPair->errorLog.c_str());
		dirty = false;
		return;
	}

	//INFO("Shader pair found and valid. Vertex shader length: %zu, Fragment shader length: %zu", shaderPair->vertexSource.length(), shaderPair->fragmentSource.length());
	
	if (shaderProgram) {
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
		//INFO("Shader program linked successfully");
		
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		
		//INFO("Getting uniform and attribute locations...");
		glUseProgram(shaderProgram);
		posAttrib = glGetAttribLocation(shaderProgram, "vs_Pos");
		texCoordAttrib = glGetAttribLocation(shaderProgram, "vs_TexCoord");
		timeUniform = glGetUniformLocation(shaderProgram, "u_Time");
		resolutionUniform = glGetUniformLocation(shaderProgram, "u_Resolution");
		audioData1Uniform = glGetUniformLocation(shaderProgram, "u_AudioData1");
		audioData2Uniform = glGetUniformLocation(shaderProgram, "u_AudioData2");
		projUniform = glGetUniformLocation(shaderProgram, "u_Proj");
		modelUniform = glGetUniformLocation(shaderProgram, "u_Model");
		trigger1Uniform = glGetUniformLocation(shaderProgram, "u_Trigger1");
		trigger2Uniform = glGetUniformLocation(shaderProgram, "u_Trigger2");
		timeWarp1Uniform = glGetUniformLocation(shaderProgram, "u_TimeWarp1");
		timeWarp2Uniform = glGetUniformLocation(shaderProgram, "u_TimeWarp2");
		
		//INFO("Setting up framebuffer...");
		setupFramebuffer();
		
		//INFO("Setting up geometry...");
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

void GLCanvasWidget::setupGeometry() {
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
	
	checkGLError("setupGeometry");
}

void GLCanvasWidget::step() {
	if (!initialized) {
		INFO("GLCanvasWidget: Initializing OpenGL context");
		OpenGlWidget::step();
		initialized = true;
		return;
	}
	
	if (dirty) {
		//INFO("GLCanvasWidget: Creating shader program due to dirty flag");
		createShaderProgram();
	}
	
	OpenGlWidget::step();
}

void GLCanvasWidget::drawFramebuffer() {
	if (!initialized) {
		INFO("GLCanvasWidget: OpenGL context not initialized yet");
		return;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	math::Vec fbSize = getFramebufferSize();
	glViewport(0.0, 0.0, fbSize.x, fbSize.y);

	if (!shaderProgram && dirty) {
			createShaderProgram();
	}

	if (!shaderProgram) {
		return;
	}

	GLboolean blend_enabled;
	glGetBooleanv(GL_BLEND, &blend_enabled);
	GLint blend_src, blend_dst;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
	
	float currentTime = rack::system::getTime() - startTime;
	if (timeUniform >= 0) glUniform1f(timeUniform, currentTime);
	if (resolutionUniform >= 0) glUniform2f(resolutionUniform, fbSize.x, fbSize.y);
	
	if (module) {
		if (audioData1Uniform >= 0) glUniform1fv(audioData1Uniform, 256, module->audioBuffer1);
		if (audioData2Uniform >= 0) glUniform1fv(audioData2Uniform, 256, module->audioBuffer2);
		if (trigger1Uniform >= 0) glUniform1f(trigger1Uniform, module->trig1);
		if (trigger2Uniform >= 0) glUniform1f(trigger2Uniform, module->trig2);
		if (timeWarp1Uniform >= 0) glUniform1f(timeWarp1Uniform, module->timeWarp1);
		if (timeWarp2Uniform >= 0) glUniform1f(timeWarp2Uniform, module->timeWarp2);
	}
	
	float aspect = fbSize.x / fbSize.y;
	float projection[16] = {
		1.0f/aspect, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	
	float model[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	
	if (projUniform >= 0) glUniformMatrix4fv(projUniform, 1, GL_FALSE, projection);
	if (modelUniform >= 0) glUniformMatrix4fv(modelUniform, 1, GL_FALSE, model);
	
	checkGLError("uniforms");
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	
	if (posAttrib >= 0) glDisableVertexAttribArray(posAttrib);
	if (texCoordAttrib >= 0) glDisableVertexAttribArray(texCoordAttrib);
	
	if (!blend_enabled) {
		glDisable(GL_BLEND);
	}
	glBlendFunc(blend_src, blend_dst);
	
	checkGLError("draw");
}

GLCanvasWidget::~GLCanvasWidget() {
	if (shaderProgram) glDeleteProgram(shaderProgram);
	if (VBO) glDeleteBuffers(1, &VBO);
	if (EBO) glDeleteBuffers(1, &EBO);
	if (frameBuffer) glDeleteFramebuffers(1, &frameBuffer);
	if (renderTexture) glDeleteTextures(1, &renderTexture);
}

struct CanvasWidget : ModuleWidget {
	Canvas* module;
	
	CanvasWidget(Canvas* module) {
		this->module = module;
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/canvas.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.427, 14.552)), module, Canvas::INPUT_1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.773, 14.552)), module, Canvas::INPUT_2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.427, 30.916)), module, Canvas::INPUT_TRIG_1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(193.773, 30.916)), module, Canvas::INPUT_TRIG_2));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.427, 81.756)), module, Canvas::PARAM_TIME_1));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(193.773, 81.756)), module, Canvas::PARAM_TIME_2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.427, 115.864)), module, Canvas::OUTPUT_1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(193.773, 115.864)), module, Canvas::OUTPUT_2));
		
		// mm2px(Vec(165.49, 128.5))
		GLCanvasWidget* glCanvas = createWidget<GLCanvasWidget>(mm2px(Vec(18.855, 0.025)));
		glCanvas->setModule(module);
		addChild(glCanvas);
	}

	void appendContextMenu(Menu* menu) override {
		if (module) {
			menu->addChild(new MenuSeparator);
			menu->addChild(createMenuLabel("Shader Source"));
			addShaderMenuItems(menu, module);
		}
	}
};

Model* modelCanvas = createModel<Canvas, CanvasWidget>("canvas");
