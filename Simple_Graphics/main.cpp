#include "VulkanRenderer.h"
//#include "DX11.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
	VulkanRenderer app;
	//D3DRenderer app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}