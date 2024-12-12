struct DrawWidget : OpenGlWidget {
    DrawWidget(Draw* module) {
        setModule(module);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/draw/draw.svg"), asset::plugin(pluginInstance, "res/draw/draw-dark.svg")));

        // ======= ADD SCREWS ===========
        //
        // ======= ADD COMPONENTS =======
        //
        // ======= PARAMS ===============
        //
        // ======= OUTPUTS ==============
    }
};
