/******************************************************************************
    Copyright (C) 2022 by Kristian Ollikainen <kristiankulta2000@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "vk-subsystem.hpp"
#include "vk-shaderhandler.hpp"
#include "vk-helpers.hpp"

#include <string>
#include <fstream>
#include <functional>
#include <filesystem>

#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <shaderc/shaderc.hpp>

std::vector<vk::DescriptorSetLayoutBinding> VulkanShader::SetupBindings()
{
	std::vector<vk::DescriptorSetLayoutBinding> result;

	for (const auto [binding, type] : vertexShader->bindings) {
		/*if (fragmentShader->bindings.find(binding) ==
		    fragmentShader->bindings.end())
			bindings.emplace_back(vk::DescriptorSetLayoutBinding(
				binding, type, 1,
				vk::ShaderStageFlagBits::eVertex));
		else*/ /* TODO: ^ See if this is even necessary, causes a validation with a shader ^ */
		result.emplace_back(vk::DescriptorSetLayoutBinding(
				binding, type, 1,
				vk::ShaderStageFlagBits::eVertex |
					vk::ShaderStageFlagBits::eFragment));
	}

	for (const auto [binding, type] : fragmentShader->bindings) {
		if (vertexShader->bindings.find(binding) ==
		    vertexShader->bindings.end())
			result.emplace_back(vk::DescriptorSetLayoutBinding(
				binding, type, 1,
				vk::ShaderStageFlagBits::eFragment));
	}

	return result;
}

void VulkanShader::Recreate()
{
	const auto logicalDevice = device->GetLogicalDevice();

	if (pipeline)
		logicalDevice.destroyPipeline(pipeline);
	if (pipelineLayout)
		logicalDevice.destroyPipelineLayout(pipelineLayout);
	if (descriptorSetLayout)
		logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

	descriptorSetLayout = device->CreateDescriptorSetLayout(descSetLayoutBindings);
	pipelineLayout = device->CreatePipelineLayout(descriptorSetLayout);
	if (device->currentSwapchain != -1)
		pipeline = device->CreateGraphicsPipeline(vertexShader.get(),
							  fragmentShader.get(),
							  pipelineLayout);
}

VulkanShader::VulkanShader(gs_device_t *_device, gs_vertex_shader *_vertexShader,
			   gs_fragment_shader *_fragmentShader)
	: vk_object(_device, vk_type::VK_COMBINEDSHADER),
	  vertexShader(std::unique_ptr<gs_vertex_shader>(_vertexShader)),
	  fragmentShader(std::unique_ptr<gs_fragment_shader>(_fragmentShader))
{
	descSetLayoutBindings = SetupBindings();
	descriptorSetLayout = device->CreateDescriptorSetLayout(descSetLayoutBindings);
	pipelineLayout = device->CreatePipelineLayout(descriptorSetLayout);
	if (device->currentSwapchain != -1)
		pipeline = device->CreateGraphicsPipeline(vertexShader.get(),
							  fragmentShader.get(),
						  	  pipelineLayout);

	matrix4_identity(&view);
	matrix4_identity(&projection);
	matrix4_identity(&viewProjection);
}

VulkanShader::~VulkanShader()
{
	const auto logicalDevice = device->GetLogicalDevice();
	if (pipeline)
		logicalDevice.destroyPipeline(pipeline);
	if (pipelineLayout)
		logicalDevice.destroyPipelineLayout(pipelineLayout);
	if (descriptorSetLayout)
		logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
}

void gs_shader::BuildUniformBuffer()
{
	for (size_t i = 0; i < params.size(); i++) {
		gs_shader_param &param = params[i];
		size_t size = 0;

		switch (param.type) {
		case GS_SHADER_PARAM_BOOL:
		case GS_SHADER_PARAM_INT:
		case GS_SHADER_PARAM_FLOAT:
			size = sizeof(float);
			break;
		case GS_SHADER_PARAM_INT2:
		case GS_SHADER_PARAM_VEC2:
			size = sizeof(vec2);
			break;
		case GS_SHADER_PARAM_INT3:
		case GS_SHADER_PARAM_VEC3:
			size = sizeof(float) * 3;
			break;
		case GS_SHADER_PARAM_INT4:
		case GS_SHADER_PARAM_VEC4:
			size = sizeof(vec4);
			break;
		case GS_SHADER_PARAM_MATRIX4X4:
			size = sizeof(float) * 4 * 4;
			break;
		case GS_SHADER_PARAM_TEXTURE:
		case GS_SHADER_PARAM_STRING:
		case GS_SHADER_PARAM_UNKNOWN:
			continue;
		}

		if (param.arrayCount)
			size *= param.arrayCount;

		/* checks to see if this constant needs to start at a new
		 * register */
		if (size && (constantSize & 15) != 0) {
			size_t alignMax = (constantSize + 15) & ~15;

			if ((size + constantSize) > alignMax)
				constantSize = alignMax;
		}

		param.pos = constantSize;
		constantSize += size;
	}

	if (constantSize) {
		try {
			uniformBuffer = std::make_unique<gs_uniform_buffer>(
					device,
					(constantSize + 15) & 0xFFFFFFF0);
		} catch (const std::runtime_error &e) {
			blog(LOG_ERROR, "Failed to create uniform buffer: %s",
			     e.what());
		}
	}

	for (size_t i = 0; i < params.size(); i++)
		gs_shader_set_default(&params[i]);
}

void gs_vertex_shader::GetBuffersExpected(const ShaderInputs &inputs)
{
	for (size_t i = 0; i < inputs.descs.size(); i++) {
		const auto &input = inputs.descs[i];
		if (strcmp(inputs.names[i].c_str(), "NORMAL") == 0)
			hasNormals = true;
		else if (strcmp(inputs.names[i].c_str(), "TANGENT") == 0)
			hasTangents = true;
		else if (strcmp(inputs.names[i].c_str(), "COLOR") == 0)
			hasColors = true;
		else if (strcmp(inputs.names[i].c_str(), "TEXCOORD") == 0)
			nTexUnits++;
	}
}

std::filesystem::path GetCachePath(const std::string &file)
{
	std::filesystem::path path = std::filesystem::current_path();
	path /= "_shaderCache";
	if (!std::filesystem::exists(path))
		std::filesystem::create_directories(path);

	path /= file;
	return path;
}

void CreateSPIRVFile(const std::string &file, std::vector<uint32_t> spirv)
{
	std::ofstream spvFile(GetCachePath(file + ".spv").string(),
			      std::ios::out | std::ios::binary);

	if (spvFile.is_open()) {
		spvFile.write(reinterpret_cast<const char *>(spirv.data()),
			      spirv.size() * sizeof(uint32_t));

		spvFile.close();
	}
}

std::vector<uint32_t> ReadSPIRVFile(const std::string &file)
{
	std::vector<uint32_t> spirv;
	std::ifstream spvFile(GetCachePath(file + ".spv").string(),
			      std::ios::in | std::ios::binary);

	if (spvFile.is_open()) {
		spirv.resize(std::filesystem::file_size(
				     GetCachePath(file + ".spv")) /
			     sizeof(uint32_t));
		spvFile.read(reinterpret_cast<char *>(spirv.data()),
			     spirv.size() * sizeof(uint32_t));

		spvFile.close();
	}
	return spirv;
}

void UpdateCache(const std::string &file, std::size_t hash,
		 std::vector<uint32_t> spirv)
{
	std::vector<std::string> cacheUpdate;
	std::ifstream cache(GetCachePath("hashes.txt").string());
	if (cache.is_open()) {
		std::string line;
		while (std::getline(cache, line))
			cacheUpdate.emplace_back(line);
	}
	cache.close();

	/* updating in-memory cache with new values */
	bool updated = false;
	for (auto &line : cacheUpdate) {
		const auto name = line.substr(0, line.find('\t'));
		if (name == file) {
			line = file + "\t" + std::to_string(hash);
			updated = true;
			break;
		}
	}

	if (!updated)
		cacheUpdate.emplace_back(file + "\t" + std::to_string(hash));

	CreateSPIRVFile(file, spirv);

	std::ofstream cacheOut(GetCachePath("hashes.txt").string());
	if (cacheOut.is_open()) {
		for (const auto &line : cacheUpdate)
			cacheOut << line << std::endl;
	}
	cacheOut.close();
}

std::size_t CreateHash(const std::string &contents)
{
	std::hash<std::string> hasher;
	return hasher(contents);
}

std::size_t GetCachedHash(const std::string &file)
{
	std::ifstream cache(GetCachePath("hashes.txt").string());
	std::string line;
	while (std::getline(cache, line)) {
		std::stringstream ss(line);
		std::string name;
		std::string hash;
		std::getline(ss, name, '\t');
		std::getline(ss, hash);
		if (name == file)
			return std::stoull(hash);
	}
	return {};
}

std::string ProcessString(const std::string &str)
{
	std::stringstream ss;
	for (char c : str) {
		if (std::isspace(c)) {
			ss << '_';
			continue;
		} else if (c == '(' || c == ',')
			continue;

		ss << c;
	}
	return ss.str();
}

void gs_shader::Compile(const std::string &source)
{
	/* SPIR-V compilation and caching */
	/* taking out relative paths and shortening file string */
	name = GetStringBetween(file, "/", ")");
	name = ProcessString(name);
	const auto hash = CreateHash(source);
	const auto cachedHash = GetCachedHash(name);

	if (!cachedHash || hash != cachedHash) {
		/* shader not cached, compile and cache */
		blog(LOG_INFO, "Compiling Shader to SPIR-V: %s", file.c_str());

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan,
					     shaderc_env_version_vulkan_1_2);

		options.SetOptimizationLevel(
			shaderc_optimization_level_performance);

		options.SetSourceLanguage(shaderc_source_language_hlsl);
		options.SetTargetSpirv(shaderc_spirv_version_1_5);

		const auto compiled = compiler.CompileGlslToSpv(
			source.c_str(), source.size(),
			type == GS_SHADER_VERTEX ? shaderc_glsl_vertex_shader
						 : shaderc_glsl_fragment_shader,
			name.c_str(), options);

		if (compiled.GetCompilationStatus() !=
		    shaderc_compilation_status_success)
			throw std::runtime_error(compiled.GetErrorMessage());

		spirv = {compiled.cbegin(), compiled.cend()};

		/* cache the shader */
		UpdateCache(name, hash, spirv);
	} else {
		/* shader is cached, load from cache */
		spirv = ReadSPIRVFile(name);
	}
}

void gs_shader::UpdateParam(std::vector<uint8_t> &constData,
				   gs_shader_param &param, bool &upload)
{
	if (param.type != GS_SHADER_PARAM_TEXTURE) {
		if (param.curValue.empty())
			throw std::runtime_error("Not all shader parameters were set");

		/* padding in case the constant needs to start at a new
		 * register */
		if (param.pos > constData.size()) {
			uint8_t zero = 0;

			constData.insert(constData.end(),
					 param.pos - constData.size(), zero);
		}

		constData.insert(constData.end(), param.curValue.begin(),
				 param.curValue.end());

		if (param.changed) {
			upload = true;
			param.changed = false;
		}

	} else if (param.curValue.size() == sizeof(gs_shader_texture)) {
		gs_shader_texture shader_tex;
		memcpy(&shader_tex, param.curValue.data(), sizeof(shader_tex));
		if (shader_tex.srgb)
			device_load_texture_srgb(device, shader_tex.tex,
						 0); // TODO: Tex units
		else
			device_load_texture(device, shader_tex.tex,
					    0); // TODO: Tex units

		if (param.nextSampler) {
			//const auto &sampler = param.nextSampler->sampler;
			// TODO: This
			param.nextSampler = nullptr;
		}
	}
}

void gs_shader::UploadParams()
{
	std::vector<uint8_t> constData;
	bool upload = false;

	constData.reserve(constantSize);

	for (size_t i = 0; i < params.size(); i++)
		UpdateParam(constData, params[i], upload);

	if (constData.size() != constantSize)
		throw std::runtime_error("Invalid constant data size given to shader");

	if (upload)
		uniformBuffer->Update(constData.data(), constantSize);
}

gs_shader::gs_shader(gs_device_t *device, gs_shader_type type, std::string file)
	: vk_object(device, type == GS_SHADER_VERTEX
				    ? vk_type::VK_VERTEXSHADER
				    : vk_type::VK_FRAGMENTSHADER),
	  type(type),
	  file(file)
{
}

gs_shader::~gs_shader()
{
	const auto logicalDevice = device->GetLogicalDevice();
	logicalDevice.destroyShaderModule(module);
	uniformBuffer.reset();
}

gs_vertex_shader::gs_vertex_shader(gs_device_t *_device, const char *source,
				   const char *file)
	: gs_shader(_device, GS_SHADER_VERTEX, file)
{
	/* processing */
	std::string hlslSource;
	ShaderProcessor processor(device);
	processor.Process(source, file);
	processor.BuildString(hlslSource);
	processor.BuildParams(params);
	processor.BuildInputLayout(shaderInputs);
	const auto vulkanified = processor.Vulkanify(this, hlslSource, true);
	GetBuffersExpected(shaderInputs);
	BuildUniformBuffer();

	code = vulkanified;

	try {
		Compile(vulkanified);
	} catch (const std::runtime_error &e) {
		throw e;
	}

	module = device->CreateShaderModule(this);

	viewProjection = gs_shader_get_param_by_name(this, "ViewProj");
	world = gs_shader_get_param_by_name(this, "World");
}

gs_fragment_shader::gs_fragment_shader(gs_device_t *_device, const char *source,
				       const char *file)
	: gs_shader(_device, GS_SHADER_PIXEL, file)
{
	/* processing */
	std::string hlslSource;
	ShaderProcessor processor(device);
	processor.Process(source, file);
	processor.BuildString(hlslSource);
	processor.BuildParams(params);
	processor.BuildSamplers(samplers);
	const auto vulkanified = processor.Vulkanify(this, hlslSource, false);
	BuildUniformBuffer();

	code = vulkanified;

	try {
		Compile(vulkanified);
	} catch (const std::runtime_error &e) {
		throw e;
	}

	module = device->CreateShaderModule(this);
}

gs_shader_param::gs_shader_param(shader_var &var, uint32_t &texCounter)
	: name(var.name),
	  type(get_shader_param_type(var.type)),
	  arrayCount(var.array_count),
	  textureID(texCounter),
	  changed(false)
{
	defaultValue.resize(var.default_val.num);
	memcpy(defaultValue.data(), var.default_val.array, var.default_val.num);

	if (type == GS_SHADER_PARAM_TEXTURE)
		texCounter++;
	else
		textureID = 0;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device,
					const char *shader_string,
					const char *file, char **error_string)
{
	gs_vertex_shader *shader = nullptr;

	try {
		shader = static_cast<gs_vertex_shader *>(
			device->SubmitShader(std::make_unique<gs_vertex_shader>(
				device, shader_string, file)));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_vertexshader_create (Vulkan): %s",
		     e.what());
		return nullptr;
	}

	return shader;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device,
				       const char *shader_string,
				       const char *file, char **error_string)
{
	gs_fragment_shader *shader = nullptr;

	try {
		shader = static_cast<gs_fragment_shader *>(device->SubmitShader(
			std::make_unique<gs_fragment_shader>(
				device, shader_string, file)));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_pixelshader_create (Vulkan): %s",
		     e.what());
		return nullptr;
	}

	return shader;
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	device->SetShader(vertshader);
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	device->SetShader(pixelshader);
}

void gs_shader_destroy(gs_shader_t *shader)
{
	shader->markedForDeletion = true;
}

int gs_shader_get_num_params(const gs_shader_t *shader)
{
	return static_cast<int>(shader->params.size());
}

gs_sparam_t *gs_shader_get_param_by_idx(gs_shader_t *shader, uint32_t param)
{
	return &shader->params[param];
}

gs_sparam_t *gs_shader_get_param_by_name(gs_shader_t *shader, const char *name)
{
	for (auto &param : shader->params) {
		if (strcmp(param.name.c_str(), name) == 0)
			return &param;
	}

	return nullptr;
}

gs_sparam_t *gs_shader_get_viewproj_matrix(const gs_shader_t *shader)
{
	if (!shader || shader->type != GS_SHADER_VERTEX)
		return nullptr;

	return static_cast<const gs_vertex_shader *>(shader)->viewProjection;
}

gs_sparam_t *gs_shader_get_world_matrix(const gs_shader_t *shader)
{
	if (!shader || shader->type != GS_SHADER_VERTEX)
		return nullptr;

	return static_cast<const gs_vertex_shader *>(shader)->world;
}

void gs_shader_get_param_info(const gs_sparam_t *param,
			      struct gs_shader_param_info *info)
{
	if (!param)
		return;

	info->name = param->name.c_str();
	info->type = param->type;
}

static inline void shader_setval_inline(gs_shader_param *param,
					const void *data, size_t size)
{
	if (!param)
		return;

	bool size_changed = param->curValue.size() != size;
	if (size_changed)
		param->curValue.resize(size);

	if (size_changed || memcmp(param->curValue.data(), data, size) != 0) {
		memcpy(param->curValue.data(), data, size);
		param->changed = true;
	}
}

void gs_shader_set_bool(gs_sparam_t *param, bool val)
{
	int b = static_cast<int>(val);
	shader_setval_inline(param, &b, sizeof(int));
}

void gs_shader_set_float(gs_sparam_t *param, float val)
{
	shader_setval_inline(param, &val, sizeof(float));
}

void gs_shader_set_int(gs_sparam_t *param, int val)
{
	shader_setval_inline(param, &val, sizeof(int));
}

void gs_shader_set_matrix3(gs_sparam_t *param, const struct matrix3 *val)
{
	matrix4 mat;
	matrix4_from_matrix3(&mat, val);
	shader_setval_inline(param, &mat, sizeof(matrix4));
}

void gs_shader_set_matrix4(gs_sparam_t *param, const struct matrix4 *val)
{
	shader_setval_inline(param, val, sizeof(matrix4));
}

void gs_shader_set_vec2(gs_sparam_t *param, const struct vec2 *val)
{
	shader_setval_inline(param, val, sizeof(vec2));
}

void gs_shader_set_vec3(gs_sparam_t *param, const struct vec3 *val)
{
	shader_setval_inline(param, val, sizeof(float) * 3);
}

void gs_shader_set_vec4(gs_sparam_t *param, const struct vec4 *val)
{
	shader_setval_inline(param, val, sizeof(vec4));
}

void gs_shader_set_texture(gs_sparam_t *param, gs_texture_t *val)
{
	shader_setval_inline(param, &val, sizeof(gs_texture_t *));
}

void gs_shader_set_val(gs_sparam_t *param, const void *val, size_t size)
{
	shader_setval_inline(param, val, size);
}

void gs_shader_set_default(gs_sparam_t *param)
{
	if (!param->defaultValue.empty())
		shader_setval_inline(param, param->defaultValue.data(),
				     param->defaultValue.size());
}

void gs_shader_set_next_sampler(gs_sparam_t *param, gs_samplerstate_t *sampler)
{
	param->nextSampler = sampler;
}
