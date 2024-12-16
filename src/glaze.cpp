#include "glaze.hpp"
#include "gl_utils.hpp"

GLProcessor::GLProcessor() {
	box.size = math::Vec(1, 1);
	visible = true;
}

void GLProcessor::setModule(Glaze* mod) {
	module = mod;
	if (module) {
		//INFO("GLProcessor: Module set, ID: %lld", (long long)module->id);
		module->processor = this;
		dirty = true;
	}
}

void GLProcessor::createShaderProgram() {
	if (!initialized || !module) {
		WARN("GLProcessor: Not initialized or no module");
		dirty = false;
		return;
	}

	//INFO("GLProcessor: Creating shader program for module %lld", (long long)module->id);

	if (shaderProgram) {
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
	}

	auto& shaderLib = SharedShaderLibrary::getInstance();
	const ShaderSubscription* sub = shaderLib.getSubscription(module->id);
	if (!sub || !sub->isValid) {
		WARN("GLProcessor: No valid shader subscription for module %lld", (long long)module->id);
		dirty = false;
		return;
	}
	//INFO("GLProcessor: Found subscription - Glib: %lld, Shader: %d", (long long)sub->glibId, sub->shaderIndex);

	const ShaderPair* shaderPair = shaderLib.getShaderForModule(module->id);
	if (!shaderPair || !shaderPair->isValid) {
		WARN("GLProcessor: No valid shader pair found for module %lld", (long long)module->id);
		dirty = false;
		return;
	}
	//INFO("GLProcessor: Found shader pair: %s", shaderPair->name.c_str());

	GLuint vertShader = gl::compileShader(shaderPair->vertexSource.c_str(), GL_VERTEX_SHADER);
	if (!vertShader) {
		INFO("Failed to compile vertex shader");
		dirty = false;
		return;
	}

	GLuint fragShader = gl::compileShader(shaderPair->fragmentSource.c_str(), GL_FRAGMENT_SHADER);
	if (!fragShader) {
		INFO("Failed to compile fragment shader");
		glDeleteShader(vertShader);
		dirty = false;
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
		glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
		INFO("Shader program linking failed: %s", infoLog);
		glDeleteProgram(shaderProgram);
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		shaderProgram = 0;
		dirty = false;
		return;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	glUseProgram(shaderProgram);
	posAttrib = glGetAttribLocation(shaderProgram, "vs_Pos");
	audioInLUniform = glGetUniformLocation(shaderProgram, "audioInL");
	audioInRUniform = glGetUniformLocation(shaderProgram, "audioInR");
	u1Uniform = glGetUniformLocation(shaderProgram, "u1");
	u2Uniform = glGetUniformLocation(shaderProgram, "u2");
	u3Uniform = glGetUniformLocation(shaderProgram, "u3");
	modeUniform = glGetUniformLocation(shaderProgram, "mode");

	setupFramebuffer();
	setupGeometry();

    gl::checkError("createShaderProgram");
	//INFO("Shader program created successfully");
	dirty = false;
}

void GLProcessor::setupFramebuffer() {
	if (frameBuffer) glDeleteFramebuffers(1, &frameBuffer);
	if (renderTexture) glDeleteTextures(1, &renderTexture);
	
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	
	glGenTextures(1, &renderTexture);
	glBindTexture(GL_TEXTURE_2D, renderTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2, 1, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);
}

void GLProcessor::setupGeometry() {
	float vertices[] = {
		-1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f
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
}

void GLProcessor::step() {
	if (!initialized) {
		OpenGlWidget::step();
		initialized = true;
		return;
	}
	
	if (dirty) {
		createShaderProgram();
	}

	processShader();
	
	OpenGlWidget::step();
}

void GLProcessor::processShader() {
	if (!shaderProgram || !frameBuffer || !renderTexture) return;

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, 2, 1);

	glUseProgram(shaderProgram);

	if (audioInLUniform >= 0) glUniform1f(audioInLUniform, currentFrame.inL);
	if (audioInRUniform >= 0) glUniform1f(audioInRUniform, currentFrame.inR);
	if (u1Uniform >= 0) glUniform1f(u1Uniform, currentFrame.u1);
	if (u2Uniform >= 0) glUniform1f(u2Uniform, currentFrame.u2);
	if (u3Uniform >= 0) glUniform1f(u3Uniform, currentFrame.u3);
	if (modeUniform >= 0) glUniform1i(modeUniform, currentFrame.mode);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	if (posAttrib >= 0) {
		glEnableVertexAttribArray(posAttrib);
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	if (posAttrib >= 0) {
		glDisableVertexAttribArray(posAttrib);
	}

	float result[8] = {0};
	glReadPixels(0, 0, 2, 1, GL_RGBA, GL_FLOAT, result);

	processedFrame.outL = std::isfinite(result[0]) ? clamp(result[0], -1.f, 1.f) : 0.f;
	processedFrame.outR = std::isfinite(result[4]) ? clamp(result[4], -1.f, 1.f) : 0.f;
}

void GLProcessor::processAudio(float& outL, float& outR, float inL, float inR, float u1, float u2, float u3, int mode) {
	currentFrame.inL = inL;
	currentFrame.inR = inR;
	currentFrame.u1 = u1;
	currentFrame.u2 = u2;
	currentFrame.u3 = u3;
	currentFrame.mode = mode;

	outL = processedFrame.outL;
	outR = processedFrame.outR;
}

GLProcessor::~GLProcessor() {
	if (shaderProgram) glDeleteProgram(shaderProgram);
	if (VBO) glDeleteBuffers(1, &VBO);
	if (EBO) glDeleteBuffers(1, &EBO);
	if (frameBuffer) glDeleteFramebuffers(1, &frameBuffer);
	if (renderTexture) glDeleteTextures(1, &renderTexture);
}

Glaze::Glaze() {
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	configInput(INPUT_L, "left audio");
	configInput(INPUT_R, "right audio");
	configInput(INPUT_LAYER, "layer cv");
	configInput(INPUT_BLEND, "blend cv");
	configInput(INPUT_MIX, "mix cv");
	configInput(INPUT_U1, "shader uniform #1");
	configInput(INPUT_U2, "shader uniform #2");
	configInput(INPUT_U3, "shader uniform #3");
	configParam(PARAM_LAYER, 0, 10, 0, "layer index");
	configParam(PARAM_BLEND, 0.f, 100.f, 0.f, "layer blend", "%");
	configParam(PARAM_MIX, 0.f, 100.f, 0.f, "wet/dry mix", "%");
	configButton(PARAM_MODE, "select mode");
	configOutput(OUTPUT_L, "left audio");
	configOutput(OUTPUT_R, "right audio");

	layer1Buffer.resize(bufferSize, 0.f);
	layer2Buffer.resize(bufferSize, 0.f);

	shaderSub.glibId = -1;
	shaderSub.shaderIndex = -1;
	shaderSub.isValid = false;

	shaderBuffer = new float[SHADER_BUFFER_SIZE];
	for (int i = 0; i < SHADER_BUFFER_SIZE; i++) {
		shaderBuffer[i] = static_cast<float>(i) / SHADER_BUFFER_SIZE;
	}
}

Glaze::~Glaze() {
	if (shaderBuffer) delete[] shaderBuffer;
}

void Glaze::onSampleRateChange(const SampleRateChangeEvent& e) {
	sampleRate = e.sampleRate;
}

void Glaze::onReset() {
	layer1Buffer.clear();
	layer2Buffer.clear();
	layer1Buffer.resize(bufferSize, 0.f);
	layer2Buffer.resize(bufferSize, 0.f);
	currentMode = MODE_REV;
	fuzzLastL = 0.f;
	fuzzLastR = 0.f;
	glidePhaseL = 0.f;
	glidePhaseR = 0.f;
	glideLastFreqL = 1.f;
	glideLastFreqR = 1.f;
	writePos = 0;
	grainTrigPhase = 0.f;
	for (auto& grain : grains) {
		grain.active = false;
	}
	warpPhaseL = 0.f;
	warpPhaseR = 0.f;
	warpLastL = 0.f;
	warpLastR = 0.f;
	std::fill(specBufL.begin(), specBufL.end(), 0.f);
	std::fill(specBufR.begin(), specBufR.end(), 0.f);
	specPos = 0;
	specPhase = 0.f;
}

void Glaze::processMode() {
	if (modeTrigger.process(params[PARAM_MODE].getValue())) {
		currentMode = (Mode)((currentMode + 1) % NUM_MODES);
	}
	
	for (int i = 0; i < NUM_MODES; i++) {
		lights[LIGHT_REV + i].setBrightness(currentMode == i ? 1.f : 0.f);
	}
}

void Glaze::onShaderSubscribe(int64_t glibId, int shaderIndex) {
	//INFO("Glaze: Shader subscription received - Glib: %lld, Shader: %d", (long long)glibId, shaderIndex);
	shaderDirty = true;
	shaderEnabled = true;
	if (processor) {
		//INFO("Glaze: Marking processor as dirty");
		processor->dirty = true;
	} else {
		WARN("Glaze: No processor available");
	}
}

void Glaze::processShader() {
	if (shaderDirty) {
		//INFO("Glaze: Processing shader update");
		auto& shaderLib = SharedShaderLibrary::getInstance();
		const ShaderSubscription* sub = shaderLib.getSubscription(id);
		if (sub) {
			shaderSub = *sub;
			//INFO("Glaze: Updated subscription - Glib: %lld, Shader: %d, Valid: %d", (long long)shaderSub.glibId, shaderSub.shaderIndex, shaderSub.isValid);
			
			if (shaderSub.isValid) {
				const ShaderPair* shaderPair = shaderLib.getShaderForModule(id);
				if (shaderPair && shaderPair->isValid) {
					//INFO("Glaze: Valid shader found: %s", shaderPair->name.c_str());
					useShaderWaveshaping = true;
				} else {
					WARN("Glaze: No valid shader pair found");
					useShaderWaveshaping = false;
				}
			} else {
				WARN("Glaze: Invalid shader subscription");
				useShaderWaveshaping = false;
			}
		} else {
			WARN("Glaze: No shader subscription found");
			useShaderWaveshaping = false;
		}
		
		shaderDirty = false;
	}
}

float Glaze::processShaderWaveshaping(float input) {
	float index = (input + 1.f) * 0.5f * (SHADER_BUFFER_SIZE - 1);
	int idx1 = static_cast<int>(index);
	
	return shaderBuffer[idx1];
}

void Glaze::processReverb(float input, std::vector<float>& buffer, float decay, float diffusion) {
	if (buffer.size() != static_cast<size_t>(bufferSize)) return;

	float newSample = input;
	float wetSignal = 0.6f * newSample;
	
	for (int i = 0; i < 4; i++) {
		size_t delayIdx = (i + 1) * (static_cast<size_t>(bufferSize) / 8);
		if (delayIdx < buffer.size()) {
			wetSignal += buffer[delayIdx] * (0.5f / (i + 1));
		}
	}

	for (size_t i = buffer.size() - 1; i > 0; i--) {
		buffer[i] = buffer[i-1] * decay;
	}
	buffer[0] = wetSignal * diffusion;
}

void Glaze::processDelay(float input, std::vector<float>& buffer, float delayTime, float feedback, float modulation, const ProcessArgs& args) {
	if (buffer.size() != static_cast<size_t>(bufferSize)) return;

	size_t baseDelay = 1 + static_cast<size_t>(delayTime * bufferSize * 0.99f);
	baseDelay = std::min(baseDelay, buffer.size() - 1);

	float modPhase = sinf(args.frame * 0.001f) * modulation;
	size_t actualDelay = std::min(
		baseDelay + static_cast<size_t>(modPhase * baseDelay * 0.1f), 
		buffer.size() - 1
	);

	float delayedSample = buffer[actualDelay];

	for (size_t i = buffer.size() - 1; i > 0; i--) {
		buffer[i] = buffer[i-1];
	}

	buffer[0] = input + (delayedSample * feedback);
}

float Glaze::processFuzz(float input, float drive, float shape, float tone, float& lastSample) {
	float driven = input * (1.f + drive * 19.f); // drive range: 1x to 20x

	float shaped;
	if (shape < 0.5f) {
		float softClip = tanh(driven);
		float hardClip = clamp(driven, -1.f, 1.f);
		shaped = softClip * (1.f - shape * 2.f) + hardClip * (shape * 2.f);
	} else {
		float hardClip = clamp(driven, -1.f, 1.f);
		float foldback = driven / (1.f + std::abs(driven));
		shaped = hardClip * (2.f - shape * 2.f) + foldback * (shape * 2.f - 1.f);
	}

	// tone control (1-pole lowpass filter)
	float filtered = shaped * tone + lastSample * (1.f - tone);
	lastSample = filtered;

	return filtered;
}

float Glaze::processGlide(float input, float& phase, float& lastFreq, float targetFreq, float glideSpeed, float waveform, const ProcessArgs& args) {
	float freq = lastFreq + (targetFreq - lastFreq) * glideSpeed;
	lastFreq = freq;

	phase += freq * args.sampleTime * 2.f * M_PI;
	if (phase > 2.f * M_PI) phase -= 2.f * M_PI;

	float mod;
	if (waveform < 0.5f) {
		float sine = sinf(phase);
		float tri = 2.f * std::abs(phase / M_PI - 1.f) - 1.f;
		mod = sine * (1.f - waveform * 2.f) + tri * (waveform * 2.f);
	} else {
		float tri = 2.f * std::abs(phase / M_PI - 1.f) - 1.f;
		float square = phase < M_PI ? 1.f : -1.f;
		mod = tri * (2.f - waveform * 2.f) + square * (waveform * 2.f - 1.f);
	}

	return input * mod;
}

void Glaze::processGrain(float input, std::vector<float>& buffer, float& outL, float& outR,
						 float density, float size, float pitch, const ProcessArgs& args) {
	if (buffer.size() != static_cast<size_t>(bufferSize)) return;

	buffer[writePos] = input;
	writePos = (writePos + 1) % buffer.size();

	float trigFreq = 0.5f + std::pow(density, 2.f) * 49.5f;
	grainTrigPhase += trigFreq * args.sampleTime;

	if (grainTrigPhase >= 1.f) {
		grainTrigPhase -= 1.f;
		
		size_t activeGrains = 0;
		for (const auto& grain : grains) {
			if (grain.active) activeGrains++;
		}

		size_t maxGrains = 4 + static_cast<size_t>(density * 12.f); // 4 to 16 max grains
		if (activeGrains < maxGrains) {
			for (auto& grain : grains) {
				if (!grain.active) {
					grain.active = true;
					grain.position = writePos;
					
					float sizeVar = size * (0.9f + random::uniform() * 0.2f); // ±10% variation
					grain.length = static_cast<size_t>((0.005f + sizeVar * 0.095f) * sampleRate); // 5ms to 100ms
					
					grain.pan = 0.1f + random::uniform() * 0.8f;
					float pitchVar = pitch * (0.95f + random::uniform() * 0.1f); // ±5% variation
					grain.pitch = 0.5f + pitchVar * 1.5f;
					
					grain.phase = 0.f;
					break;
				}
			}
		}
	}

	outL = outR = 0.f;
	float totalEnv = 0.f;

	for (auto& grain : grains) {
		if (!grain.active) continue;

        // Hann window
		float envPhase = static_cast<float>(grain.phase) / grain.length;
		float env = 0.5f * (1.f - cosf(2.f * M_PI * envPhase));
		totalEnv += env;

		size_t readPos = (grain.position + static_cast<size_t>(grain.phase * grain.pitch)) % buffer.size();
		float grainSample = buffer[readPos] * env;

		float panL = cosf(grain.pan * M_PI_2);
		float panR = sinf(grain.pan * M_PI_2);
		outL += grainSample * panL;
		outR += grainSample * panR;

		grain.phase++;
		if (grain.phase >= grain.length) {
			grain.active = false;
		}
	}

	if (totalEnv > 1.f) {
		float norm = 0.7f / totalEnv;
		outL *= norm;
		outR *= norm;
	}
}

float Glaze::processFold(float input, float folds, float symmetry, float bias) {
	float biased = input + (bias * 2.f - 1.f);
	float numFolds = 1.f + folds * 7.f;
	float phase = biased * numFolds * M_PI;
	float folded;
	
	if (symmetry < 0.5f) {
		float sine = sinf(phase);
		float tri = 2.f * (std::abs(fmodf(phase / M_PI + 0.5f, 2.f) - 1.f) - 0.5f);
		folded = sine * (1.f - symmetry * 2.f) + tri * (symmetry * 2.f);
	} else {
		float tri = 2.f * (std::abs(fmodf(phase / M_PI + 0.5f, 2.f) - 1.f) - 0.5f);
		float parabolic = phase / M_PI;
		parabolic = parabolic - floorf(parabolic + 0.5f);
		parabolic = 4.f * (parabolic * parabolic - 0.25f);
		folded = tri * (2.f - symmetry * 2.f) + parabolic * (symmetry * 2.f - 1.f);
	}

	return tanh(folded * 0.7f);
}

float Glaze::processWarp(float input, float& phase, float& lastSample, float amount, float shape, float skew, const ProcessArgs& args) {
	phase += args.sampleTime;
	if (phase >= 1.f) phase -= 1.f;
	float warpedPhase = phase;
	
	if (shape < 0.5f) {
		float mod = sinf(phase * 2.f * M_PI);
		warpedPhase = phase + (mod * amount * 0.1f);
	} else {
		float expBase = 1.f + (shape * 2.f - 1.f) * 9.f;
		float expPhase = (std::pow(expBase, phase) - 1.f) / (expBase - 1.f);
		warpedPhase = phase + (expPhase - phase) * amount;
	}

	float skewedPhase = warpedPhase + (skew * 2.f - 1.f) * 0.25f;
	skewedPhase -= floorf(skewedPhase);

	size_t idx1 = static_cast<size_t>(skewedPhase * bufferSize) % bufferSize;
	float frac = (skewedPhase * bufferSize) - floorf(skewedPhase * bufferSize);

	float warped = input * (1.f - frac) + lastSample * frac;
	lastSample = input;

	return tanh(warped);
}

// TODO: spectral processing is not exactly working... 
float Glaze::processSpectral(float input, std::vector<float>& specBuf, std::vector<float>& window,
						 float spread, float shift, float smear) {
	if (specBuf.size() != SPEC_SIZE || window.size() != SPEC_SIZE) return input;

	specBuf[specPos] = tanh(input * 1.5f);
	
	float output = 0.f;
	float totalWeight = 0.f;
	
	for (size_t i = 0; i < SPEC_SIZE; i++) {
		size_t readPos = (specPos + i) % SPEC_SIZE;
		float spreadAmount = spread * 2.f;
		float readOffset = spreadAmount * sinf(2.f * M_PI * i / SPEC_SIZE);
		size_t offsetPos = (readPos + static_cast<size_t>(readOffset * SPEC_SIZE)) % SPEC_SIZE;
		
        // Blackman window
		float win = 0.42f - 0.5f * cosf(2.f * M_PI * i / SPEC_SIZE) 
					+ 0.08f * cosf(4.f * M_PI * i / SPEC_SIZE);
		window[i] = win;
		
		float phase = shift * 4.f * M_PI * i / SPEC_SIZE;
		float modulator = sinf(phase) * (1.f + smear * sinf(phase * 0.5f));
		
		float sample = specBuf[offsetPos];
		float resonance = 1.f + smear * 2.f;
		float processed = sample * modulator * win * resonance;
		
		if (i > 0) {
			float feedback = smear * 0.8f;
			processed = processed * (1.f - feedback) + 
						specBuf[(offsetPos + SPEC_SIZE - 1) % SPEC_SIZE] * win * feedback;
		}
		
		output += processed;
		totalWeight += win;
	}
	
	if (totalWeight > 0.f) {
		output = output / totalWeight;
		output = output * (1.f + 0.2f * sinf(output * M_PI));
	}
	
	specPos = (specPos + 1) % SPEC_SIZE;
	
	return tanh(output * 1.5f);
}

void Glaze::process(const ProcessArgs& args) {
	processMode();
	processShader();

	bool leftConnected = inputs[INPUT_L].isConnected();
	bool rightConnected = inputs[INPUT_R].isConnected();

	if (!leftConnected && !rightConnected) {
		outputs[OUTPUT_L].setVoltage(0.f);
		outputs[OUTPUT_R].setVoltage(0.f);
		return;
	}

	float inL = leftConnected ? inputs[INPUT_L].getVoltage() : 0.f;
	float inR = rightConnected ? inputs[INPUT_R].getVoltage() : inL;

	inL = clamp(inL, -10.f, 10.f) / 10.f;
	inR = clamp(inR, -10.f, 10.f) / 10.f;

	float mix = params[PARAM_MIX].getValue() / 100.f;
	if (inputs[INPUT_MIX].isConnected()) {
		mix = clamp(inputs[INPUT_MIX].getVoltage() / 10.f, 0.f, 1.f);
	}

	float u1 = clamp(inputs[INPUT_U1].getVoltage() / 10.f, 0.f, 1.f);
	float u2 = clamp(inputs[INPUT_U2].getVoltage() / 10.f, 0.f, 1.f);
	float u3 = clamp(inputs[INPUT_U3].getVoltage() / 10.f, 0.f, 1.f);

	float outL = inL;
	float outR = inR;

	// try shader processing first if enabled
	if (shaderEnabled && processor) {
		processor->processAudio(outL, outR, inL, inR, u1, u2, u3, currentMode);
	} else {
		// fall back to DSP processing
		switch (currentMode) {
			case MODE_REV: {
				float decay = 0.5f + u1 * 0.499f;   // [0.5, 0.999]
				float diffusion = 0.2f + u2 * 0.7f; // [0.2, 0.9]

				if (leftConnected) {
					processReverb(inL, layer1Buffer, decay, diffusion);
					outL = layer1Buffer[0];
				}
				if (rightConnected) {
					processReverb(inR, layer2Buffer, decay, diffusion);
					outR = layer2Buffer[0];
				}
				break;
			}
			case MODE_DLY: {
				float delayTime = u1;           // [0, 1] (scaled by buffer size)
				float feedback = u2 * 0.99f;    // [0, 0.99]
				float modulation = u3;          // [0, 1]

				if (leftConnected) {
					processDelay(inL, layer1Buffer, delayTime, feedback, modulation, args);
					outL = layer1Buffer[0];
				}
				if (rightConnected) {
					processDelay(inR, layer2Buffer, delayTime * 1.01f, feedback, modulation, args);
					outR = layer2Buffer[0];
				}
				break;
			}
			case MODE_FZZ: {
				if (useShaderWaveshaping) {
					if (leftConnected) {
						outL = processShaderWaveshaping(inL);
					}
					if (rightConnected) {
						outR = processShaderWaveshaping(inR);
					}
				} else {
					float drive = u1;
					float shape = u2;
					float tone = 0.1f + u3 * 0.89f;

					if (leftConnected) {
						outL = processFuzz(inL, drive, shape, tone, fuzzLastL);
					}
					if (rightConnected) {
						outR = processFuzz(inR, drive, shape, tone, fuzzLastR);
					}
				}
				break;
			}
			case MODE_GLD: {
				float targetFreq = 0.25f + u1 * 4.f;        // [0.25, 4.0]
				float glideSpeed = 0.001f + u2 * 0.099f;    // [0.001, 0.1]
				float waveform = u3;                        // morph 

				if (leftConnected) {
					outL = processGlide(inL, glidePhaseL, glideLastFreqL, targetFreq, glideSpeed, waveform, args);
				}
				if (rightConnected) {
					outR = processGlide(inR, glidePhaseR, glideLastFreqR, targetFreq * 1.003f, glideSpeed, waveform, args);
				}
				break;
			}
			case MODE_GRN: {
				float density = u1;
				float size = u2;
				float pitch = u3;

				float grainL = 0.f;
				float grainR = 0.f;
				if (leftConnected) {
					processGrain(inL, layer1Buffer, grainL, grainR, density, size, pitch, args);
					outL = grainL;
					if (!rightConnected) outR = grainR;
				}
				if (rightConnected) {
					float grainL2 = 0.f;
					float grainR2 = 0.f;
					processGrain(inR, layer2Buffer, grainL2, grainR2, density, size, pitch, args);
					outR = grainR2;
				}
				break;
			}
			case MODE_FLD: {
				float folds = u1;       // [1, 8] 
				float symmetry = u2;
				float bias = u3;

				if (leftConnected) {
					outL = processFold(inL, folds, symmetry, bias);
				}
				if (rightConnected) {
					outR = processFold(inR, folds, symmetry, bias);
				}
				break;
			}
			case MODE_WRP: {
				float amount = u1;     // [0, 1]
				float shape = u2;      // [sin, exp]
				float skew = u3;       // [-1, 1] 

				if (leftConnected) {
					outL = processWarp(inL, warpPhaseL, warpLastL, amount, shape, skew, args);
				}
				if (rightConnected) {
					outR = processWarp(inR, warpPhaseR, warpLastR, amount, shape, skew * 1.02f, args);
				}
				break;
			}
			case MODE_SPC: {
				float spread = u1 * 0.8f;      // [0, 0.8] 
				float shift = u2 * 4.f - 2.f;  // [-2, 2] 
				float smear = u3 * 0.9f;       // [0, 0.9] 

				if (leftConnected) {
					outL = processSpectral(inL, specBufL, specWindowL, spread, shift, smear);
				}
				if (rightConnected) {
					outR = processSpectral(inR, specBufR, specWindowR, spread, shift * 1.1f, smear);
				}
				break;
			}
			case NUM_MODES:
				break;
		}
	}

	outL = inL * (1.f - mix) + outL * mix;
	outR = inR * (1.f - mix) + outR * mix;

	outputs[OUTPUT_L].setVoltage(clamp(outL * 10.f, -10.f, 10.f));
	outputs[OUTPUT_R].setVoltage(clamp(outR * 10.f, -10.f, 10.f));
}

json_t* Glaze::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "currentMode", json_integer(currentMode));
	return rootJ;
}

void Glaze::dataFromJson(json_t* rootJ) {
	json_t* modeJ = json_object_get(rootJ, "currentMode");
	if (modeJ) {
		currentMode = (Mode)json_integer_value(modeJ);
	}
}

struct GlazeWidget : ModuleWidget {
	GlazeWidget(Glaze* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/glaze.svg")));

		GLProcessor* processor = new GLProcessor();
		if (module) {
			processor->setModule(module);
		}
		addChild(processor);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 14.552)), module, Glaze::INPUT_L));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 32.686)), module, Glaze::INPUT_R));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.879, 83.879)), module, Glaze::INPUT_U1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.115, 83.879)), module, Glaze::INPUT_U2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.482, 83.879)), module, Glaze::INPUT_U3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.478, 101.358)), module, Glaze::INPUT_LAYER));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.482, 101.358)), module, Glaze::INPUT_BLEND));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 118.858)), module, Glaze::INPUT_MIX));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.478, 83.879)), module, Glaze::PARAM_LAYER));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48, 101.358)), module, Glaze::PARAM_BLEND));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.482, 118.838)), module, Glaze::PARAM_MIX));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(10.478, 118.858)), module, Glaze::PARAM_MODE));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.482, 14.552)), module, Glaze::OUTPUT_L));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.482, 32.686)), module, Glaze::OUTPUT_R));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(10.442, 52.509)), module, Glaze::LIGHT_REV));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(23.879, 52.509)), module, Glaze::LIGHT_DLY));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(37.115, 52.509)), module, Glaze::LIGHT_FZZ));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(50.395, 52.531)), module, Glaze::LIGHT_GLD));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(10.442, 64.437)), module, Glaze::LIGHT_GRN));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(23.879, 64.437)), module, Glaze::LIGHT_FLD));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(37.115, 64.437)), module, Glaze::LIGHT_WRP));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(50.395, 64.459)), module, Glaze::LIGHT_SPC));
	}

	void appendContextMenu(Menu* menu) override {
		Glaze* module = dynamic_cast<Glaze*>(this->module);
		if (!module) return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Shader"));
		addShaderMenuItems(menu, module);
	}
};

Model* modelGlaze = createModel<Glaze, GlazeWidget>("glaze");
