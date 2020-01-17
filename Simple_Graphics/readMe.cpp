/*
Most of the code inside of "VulkanRenderer.cpp" and "VulkanRenderer.h" is from the tutorial found at
https://vulkan-tutorial.com/ .  I have decided to write what I have modified here as there is a lot of code
in those two files that I cannot claim to be mine, and trying to find the differences would be impossible


Here is a quick overview, if you want to see the important code you can scroll down.

What has been changed from the base tutorial is that I have added the feature to render multiple
models each with their own scale, rotation, location and textures.  The ability to have multiple models
and textures was not outlined in the tutorial, I had to figure it out myself and modify the code to achieve
that.

To get multiple models on the screen at once I had to modify the command buffer generation for
drawing the model.  It was originally static in the way that it would use a global index buffer, vertex
buffer, MVP matrices, and would use the same commands every frame.  I modified it such that there is a
per-model struct that contains the vertex and index buffers, MVP matrices, and a couple other variables.
I then update all the models separately in an update model function.  I then take all the updated model
information and a create command buffer based on all the models and their information.  During the
creation of the command buffer I use push constants to update the MVP matrices of each model in the
shaders.  One of the problems that I have now and would like to fix in a later push is that I am currently
using 196 bytes passing all these matrices which is way over the 128 bytes guaranteed for Vulkan and
does not need to be done. From my current understanding I can simply have the model matrix passed as
part of the graphics pipeline in the descriptor set layout as they are not updated as frequently as the
model matrix and do not need the speed of the push constants.

To load multiple textures and use them at once I used a solution very similar to what is found here
http://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html The way that this
works is that I have an global array of texture objects that I have created.  Then an image is loaded in to
a texture struct and then stored into the global texture array.  Then simply to choose a texture to use in
the shader, I pass an integer through a push constant (set in the command buffer generation function)
to select the texture that it will choose.  The textures themselves are stored in descriptors and are made
available to the shader.  From the shader it simply uses the texture at the index sent through the push
constant.

What I’m working on currently:
The original plan for this project was to create a very simple rendering engine that would draw models
and textures using both Vulkan and D3D11.  I wanted to start with Vulkan since it had a great tutorial
and would be a good way to learn the basics of real time graphic but D3D11 is proving to be more difficult 
than I initially thought(I’m not following a tutorialfor this one) I am currently working on matching what I 
have in Vulkan in D3D11 and then combining the two to create a simple object-oriented renderer that at least 
from a higher level will use common data structures, function calls, and program structure and will abstract 
out the details of the graphics API’s.
So, I figured I would develop them separately to learn the ropes and then bring them together once I
have completed the D3D11 portion.

Another small project I would like to add on top of this is creating a binary data format that I could store
models and textures in so that load times can be reduced because currently they take a very long time
to load at startup since it is loading the model directly from an OBJ.

*/

/* NOTE this file will not compile, and is just some snippets of what I have added onto the tutorial code
	There are many more small modifications that I have made through the code for these to work such as
	making the index and vertex buffers per model and not global, but I only put the most interesting
	pieces in here

	Also, none of these models are mine
*/

/*
Here are some of the structs that I am using, the reason that a texture object is not inside of the
model object is so that other things may use the texture if necessary, this also will introduce a lot of repeated textures,
since the index is the most important piece that is all that is in the Model struct
*/

struct Model {
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	constantBufferMVP mvp;
	uint32_t indiciesCount;

	Transform transform;
	uint32_t texture_index;
};

struct constantBufferMVP {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Transform {
	glm::vec3 location;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct Texture {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	uint32_t height;
	uint32_t width;
};

/*
The model’s array and texture array are global to the program
The texture is an actual array because as of right now the amount of
textures must be known before startup
*/

std::vector<Model*> modelsArray;
Texture* textureArray[TEXTURES_USED];

/* this is a snippet of the modified code in the graphics pipeline
   all this is doing is adding push constants to the pipeline
   and making them know to the shader this adds both the mvp
   matrix and the texture index push constants*/
void VulkanRenderer::createGraphicsPipeline() {
	.....

	//ADDED Push Constants:
	VkPushConstantRange pushConstantMVP;
	pushConstantMVP.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantMVP.offset = 0;
	pushConstantMVP.size = sizeof(constantBufferMVP);

	VkPushConstantRange pushConstantRangeTexture;
	pushConstantRangeTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRangeTexture.offset = sizeof(constantBufferMVP);
	pushConstantRangeTexture.size = sizeof(uint32_t);

	std::array<VkPushConstantRange, 2> pushConstantInfo = { pushConstantMVP, pushConstantRangeTexture };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 2;
	pipelineLayoutInfo.pPushConstantRanges = pushConstantInfo.data();

	.......
}

/*
	This is a snippet from the building of the command buffer that will render all the models
	this will iterate through and update all the necessary info for all the models and then submit all of these
	commands to the GPU.  All the info is updated from the draw frame function
*/

void VulkanRenderer::buildMainCommandBuffer(int imageIndex) {
	.......

	vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	for (const auto model : modelsArray) {
		vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constantBufferMVP), (void*)&model->mvp);
		vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(constantBufferMVP), sizeof(uint32_t), (void*)&model->texture_index);
		vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);

		VkBuffer vertexBuffers[] = { model->vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[imageIndex], model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(model->indiciesCount), 1, 0, 0, 0);

	}
	vkCmdEndRenderPass(commandBuffers[imageIndex]);

	if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

/*
This is called from the drawFrame function, this will iterate through and draw all the
models.  This is currently set up to rotate the models based on frame time but can be removed
to allow rotation to be set statically.
*/

void VulkanRenderer::updateModels() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	for (const auto model : modelsArray) {
		model->mvp.model = glm::translate(glm::mat4(1.0f), model->transform.location);
		model->mvp.model = glm::rotate(model->mvp.model, time * glm::radians(model->transform.rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
		model->mvp.model = glm::scale(model->mvp.model, model->transform.scale);
		model->mvp.view = glm::lookAt(glm::vec3(0.0f, 7.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		model->mvp.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

		//glm made for openGL, need to flip the y as they are opposite in vulkan
		model->mvp.proj[1][1] *= -1;
	}
}


/* 
	Here is the code to load in the models and textures and assign everything.
	Once it has been loaded in here it will automatically be updated and drawn based on the 
	transform.
*/
void VulkanRenderer::loadModels() {
	Model* tireModel = loadModel("models/tire.obj");
	Transform tireTransform = { glm::vec3(2.5f,0.0f,0.0f), glm::vec3(60.0f,0.0f,0.0f), glm::vec3(0.05f,0.05f,0.05f) };
	setModelTransform(tireModel, tireTransform);
	textureArray[0] = loadTexture("textures/Tire_Red_Color.png");
	tireModel->texture_index = 0;
	modelsArray.push_back(tireModel);

	Model* tireModel2 = loadModel("models/crypto.obj");
	Transform tireTransform2 = { glm::vec3(0.0f,0.0f,0.0f), glm::vec3(60.0f,0.0f,0.0f), glm::vec3(0.04f,0.04f,0.04f) };
	setModelTransform(tireModel2, tireTransform2);
	textureArray[1] = loadTexture("textures/crypto.png");
	tireModel2->texture_index = 1;
	modelsArray.push_back(tireModel2);

	Model* tireModel3 = loadModel("models/earth.obj");
	Transform tireTransform3 = { glm::vec3(-2.5f,0.0f,0.0f), glm::vec3(60.0f,0.0f,0.0f), glm::vec3(0.2f,0.2f,0.2f) };
	setModelTransform(tireModel3, tireTransform3);
	textureArray[2] = loadTexture("textures/earth_night.png");
	tireModel3->texture_index = 2;
	modelsArray.push_back(tireModel3);


}

void VulkanRenderer::freeModel(Model* model) {
	if (model != nullptr) {
		vkDestroyBuffer(device, model->vertexBuffer, nullptr);
		vkFreeMemory(device, model->vertexBufferMemory, nullptr);

		vkDestroyBuffer(device, model->indexBuffer, nullptr);
		vkFreeMemory(device, model->indexBufferMemory, nullptr);

		delete model;
	}
}

void VulkanRenderer::freeTexture(uint32_t index) {
	if (textureArray[index] != nullptr) {
		vkDestroyImage(device, textureArray[index]->image, nullptr);
		vkFreeMemory(device, textureArray[index]->imageMemory, nullptr);
		vkDestroyImageView(device, textureArray[index]->imageView, nullptr);
		delete textureArray[index];
	}
}

void VulkanRenderer::setModelTransform(Model* model, Transform transform) {
	setModelLocation(model, transform.location);
	setModelRotation(model, transform.rotation);
	setModelScale(model, transform.scale);
}

void VulkanRenderer::setModelLocation(Model* model, glm::vec3 location) {
	model->transform.location = location;
}
void VulkanRenderer::setModelRotation(Model* model, glm::vec3 rotation) {
	model->transform.rotation = rotation;
}
void VulkanRenderer::setModelScale(Model* model, glm::vec3 scale) {
	model->transform.scale = scale;
}


/*
	Here are the two important functions for allowing the textures to be stored in a global array and then
	accessed from the shader through a push constant index variable.  We simply set up descriptors that
	are combined image and sampler and let the GPU know that there will be this many textures to choose from
	and where to find them
*/

void VulkanRenderer::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 1> poolSizes = {};

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size()) * TEXTURES_USED;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanRenderer::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapChainImages.size());

	std::cout << "SwapChain size: " << swapChainImages.size() << std::endl;


	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < swapChainImages.size(); i++) {

		std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
		VkDescriptorImageInfo samplerInfo = {};
		std::vector<VkDescriptorImageInfo> descriptorImageInfos(TEXTURES_USED);

		for (uint32_t j = 0; j < TEXTURES_USED; j++) {
			if (textureArray[j] != nullptr) {
				descriptorImageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorImageInfos[j].imageView = textureArray[j]->imageView;
				descriptorImageInfos[j].sampler = textureSampler;
			}
		}

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = TEXTURES_USED;
		descriptorWrites[0].pImageInfo = descriptorImageInfos.data();


		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}