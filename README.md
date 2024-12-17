 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sup><sub><sup><sub><sup><sub><sup><sub><sup>GL_INVALID_ERROR</sub></sup></sub></sup></sub></sup></sub></sup>
</sup><sub><sup>GL_INVALID_ERROR</sub></sup>
# 0x502
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<sub><sup><sub><sup><sub><sup><sub><sup>GL_INVALID_ERROR</sub></sup></sub></sup></sub></sup></sub></sup>

A collection of VCV Rack modules that use glsl shaders for audio processing

## Releases
0x502 is still in active development. [Here is the link to the current Mac ARM64 build](https://drive.google.com/file/d/1w6VC8NFBHGyhxQTp58SPqciRrI9FjcoH/view?usp=sharing) if you would like to try it out! Once you have downloaded the zip file, please unzip it and put the entire folder in your [rack plugin directory](https://vcvrack.com/manual/FAQ#Where-is-the-Rack-user-folder).

## Building
If you would like to build it yourself, please first make sure that you have all the necesary dependencies for your platform by following [VCV Rack's guide for setting up your build environment](https://vcvrack.com/manual/Building#Setting-up-your-development-environment).  

Once your environment is set up, clone this repo:  
```git clone https://github.com/teriyake/0x502.git/ && cd 0x502```  

Then run cmake:  
```make install```  
This should automatically install the plugin to your rack directory.

## Modules

### GLIB
GLIB (glsl shader library) is a utility module that manages loading shaders from local files and sharing them among 0x502 modules.  

![GLIB screenshot](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/glib.png)  

Every 0x502 module that uses shaders is a "GLIB subscriber" and fetches shaders from GLIB. To subscribe a module to GLIB, right-click on the module to open the menu and select the GLIB module under "Shader":  

![shader menu](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/shader-menu-2.png)  

The subscriber modules can only subscribe to GLIBs that already have a shader uploaded. 
![shader menu](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/shader-menu.jpg)  

To upload a shader to glib, click the upload button and select a vertex shader file. Note: GLIB uploads shaders as a pair (i.e., both the vertex and fragment shaders must have the same name--a valid pair would look like ```shader.vert```and```shader.frag```). GLIB validates and compiles the uploaded shaders: green LEDs indicate that the shaders are successfully compiled, linked, and ready to be used in other 0x502 modules.

### GLCV
GLCV is a module that uses shaders to generate up to 4 different control voltages.  

![GLCV screenshot](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/glcv.png)  

The module has inputs for clock & reset. The time/space input, chaos, and scale knobs are used to set shader uniforms. The majority of the computation happens in the shaders, which are fetched from GLIB and customizable. The demo shaders for this module are [```res/shaders/cv.vert```](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/res/shaders/cv.vert) and [```res/shaders/cv.frag```](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/res/shaders/cv.frag). The available shader uniforms are: ```uniform float u_Time;
uniform float u_Chaos;
uniform float u_Scale;
uniform float u_ClockTime;
uniform float u_TimeSpace;```

### GLAZE
GLAZE (glsl shoegaze) is a multi-mode effect module that uses both dsp and shaders to process input audio signals.  

![GLAZE screenshot](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/glaze.png)  

Right now, I've implemented dsp for all modes and shader-based processing for RVB, FZZ, and FLD.  
A more in-depth documentation of the different modes and some technical details can be found [here](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/glaze.md).

### GLAB
GLAB (glsl shader lab) is a live coding module for editing glsl shaders.  

![GLAB screenshot](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/glab.png)  

The text editor on the left is used for the vertex shader, and the one on the left is for the fragment shader. There is also a display above the fragment shader editor that shows the current status & potential GL errors.  
To use the shaders in other 0x502 modules, GLAB needs a GLIB subscription first. When you are finished editing the shaders, you can click the respective button to compile the vertex and fragment shaders. Once both shaders are compiled, you can publish them to the GLIB module by clicking the publish button. The LED under the publish button indicates the current state of the shaders: GREEN = published, RED = invalid shaders, BLUE = unpublished changes.  

Note: the text editors are sort of broken as of right now, and I'm having some issues with cursor positions when there are wrapped lines. I think the broken text editors may have messed up the shader validation as well, which worked before when I hadn't made any major changes to VCV Rack's ```LedDisplayTextField```. But I should be able to get this fixed soon!

### canvas
Canvas is a visualizer module that uses glsl shaders to interpret the input audio signals.  

![canvas screenshot](https://github.com/teriyake/0x502/blob/115a5632a4bed5d5fa62910835fc4b58fcc542f9/docs/screenshots/canvas.png)  

There are two available audio inputs and two knobs that adjust the "time warp" factors for each input. Canvas is a GLIB subscriber, and you can write your own shaders to customize the visuals. The demo shaders for this module are ```res/shaders/basic.vert``` and ```res/shaders/basic.frag```. The available shader uniforms are:  


```uniform float u_AudioData1[256];
uniform float u_AudioData2[256];
uniform float u_Time;
uniform vec2 u_Resolution;
uniform float u_Trigger1;
uniform float u_Trigger2;
uniform float u_TimeWarp1;
uniform float u_TimeWarp2;
```

## Development
This is a very rough draft of an idea I had, and any feedback/suggestions/bug reports are very very welcome! Please feel free to open an issue or make a pull request.



