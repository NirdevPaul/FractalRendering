#include "VulkanEngine.h"

VulkanEngine engine;

void init() {
	engine.glfwWindowInit();
	engine.initVulkan();
	engine.initUI();
}

int main() {
	init();
	engine.mainLoop();

	engine.cleanup();
	return 0;
}
