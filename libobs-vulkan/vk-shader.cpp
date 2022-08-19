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

#include <string>
#include <fstream>
#include <functional>
#include <filesystem>

#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <shaderc/shaderc.hpp>

void gs_shader::BuildConstantBuffer()
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
				device, (constantSize + 15) & 0xFFFFFFF0,
				nullptr);
		} catch (const std::runtime_error &e) {
			blog(LOG_ERROR, "Failed to create uniform buffer: %s",
			     e.what());
		}
	}

	for (size_t i = 0; i < params.size(); i++)
		gs_shader_set_default(&params[i]);
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
		spvFile.write(
			reinterpret_cast<const char *>(
				spirv.data()),
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
		spirv.resize(std::filesystem::file_size(GetCachePath(file + ".spv")) / sizeof(uint32_t));
		spvFile.read(
			reinterpret_cast<char *>(
				spirv.data()),
			spirv.size() * sizeof(uint32_t));

		spvFile.close();
	}
	return spirv;
}

void UpdateCache(const std::string &file, std::size_t hash, std::vector<uint32_t> spirv)
{
	std::fstream cache(GetCachePath("hashes.txt").string(), std::ios::in | std::ios::out | std::ios::app);
	cache.seekg(0, std::ios::beg);
	if (cache.is_open()) {
		std::string line;
		bool found = false;
		while (std::getline(cache, line)) {
			if (line.find(file) != std::string::npos) {
				found = true;
				cache << file << "\t" << hash << std::endl;
				/* replace old .spv file */
				CreateSPIRVFile(file, spirv);
				break;
			}
		}

		if (!found) {
			cache.clear();
			cache << file << "\t" << hash << std::endl;
			/* add new .spv file */
			CreateSPIRVFile(file, spirv);
		}
	}
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

std::string GetStringBetween(const std::string &str, const std::string &start, const std::string &end)
{
	std::size_t start_pos = str.find_last_of(start);
	if (start_pos == std::string::npos)
		return "";

	start_pos += start.length();
	std::size_t end_pos = str.find(end, start_pos);
	if (end_pos == std::string::npos)
		return "";

	return str.substr(start_pos, end_pos - start_pos);
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

gs_shader::gs_shader(gs_device_t *device, gs_shader_type type, const char *source, const char *file)
	: device(device), type(type), constantSize(0)
{

	/* processing */
	std::string hlslSource;
	ShaderProcessor processor;
	processor.Process(source, file);
	processor.BuildString(hlslSource);
	processor.BuildParams(params);
	BuildConstantBuffer();

	/* SPIR-V compilation and caching */
	/* look for identical shader in cache */
	auto strFile = GetStringBetween(file, "/", ")");
	strFile = ProcessString(strFile);
	std::size_t hash = GetCachedHash(strFile);
	bool isCached = false;
	if (hash) {
		const auto comp = CreateHash(source);
		if (comp == hash)
			isCached = true;
	}

	if (!isCached) {
		/* shader not cached, compile and cache */
		blog(LOG_INFO, "Compiling Shader to SPIR-V: %s", file);

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan,
					     shaderc_env_version_vulkan_1_2);

		options.SetOptimizationLevel(
			shaderc_optimization_level_performance);

		options.SetSourceLanguage(shaderc_source_language_hlsl);
		options.SetTargetSpirv(shaderc_spirv_version_1_5);

		if (type == GS_SHADER_VERTEX) {
			const auto compiled = compiler.CompileGlslToSpv(
				hlslSource.c_str(), hlslSource.size(),
				shaderc_glsl_vertex_shader, file, options);

			if (compiled.GetCompilationStatus() !=
			    shaderc_compilation_status_success)
				throw std::runtime_error(
					(compiled.GetErrorMessage() +
					 std::string("\n\n\n") + hlslSource +
					 std::string("\n\n\n")));

			spirv = {compiled.cbegin(), compiled.cend()};
		} else if (type == GS_SHADER_PIXEL) {
			const auto compiled = compiler.CompileGlslToSpv(
				hlslSource.c_str(), hlslSource.size(),
				shaderc_glsl_fragment_shader, file, options);

			if (compiled.GetCompilationStatus() !=
			    shaderc_compilation_status_success)
				throw std::runtime_error(
					(compiled.GetErrorMessage() +
					 std::string("\n\n\n") + hlslSource +
					 std::string("\n\n\n")));

			spirv = {compiled.cbegin(), compiled.cend()};
		} else
			throw std::runtime_error("Invalid Shader type");

		/* cache the shader */
		const auto hsh = CreateHash(source);
		UpdateCache(strFile, hsh, spirv);
	} else {
		/* shader is cached, load from cache */
		spirv = ReadSPIRVFile(strFile);
	}
}

gs_shader::~gs_shader() {}

gs_vertex_shader::gs_vertex_shader(gs_device_t *device, const char *source, const char *file)
	: gs_shader(device, GS_SHADER_VERTEX, source, file)
{
}

gs_fragment_shader::gs_fragment_shader(gs_device_t *device, const char *source, const char *file)
	: gs_shader(device, GS_SHADER_PIXEL, source, file)
{
}

gs_shader_t *device_vertexshader_create(gs_device_t *device,
					const char *shader_string,
					const char *file, char **error_string)
{
	gs_vertex_shader *shader = nullptr;

	try {
		shader = new gs_vertex_shader(device, shader_string, file);
		shader->vertexModule = device->CreateShaderModule(shader);
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
		shader = new gs_fragment_shader(device, shader_string, file);
		shader->fragmentModule = device->CreateShaderModule(shader);

	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_pixelshader_create (Vulkan): %s",
		     e.what());
		return nullptr;
	}

	/* pipeline creation */
	if (shader->vertexModule) {
		try {
			shader->pipeline = device->CreateGraphicsPipeline(
				shader->vertexModule, shader->fragmentModule);

			device->GetLogicalDevice().destroyShaderModule(
				shader->vertexModule);

			device->GetLogicalDevice().destroyShaderModule(
				shader->fragmentModule);
		} catch (const std::runtime_error &e) {
			blog(LOG_ERROR,
			     "device_pixelshader_create (Vulkan): %s",
			     e.what());
			return nullptr;
		}
	}

	return shader;
}

void gs_shader_destroy(gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
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
	UNUSED_PARAMETER(shader);
	return nullptr;
}

gs_sparam_t *gs_shader_get_world_matrix(const gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
	return nullptr;
}

void gs_shader_get_param_info(const gs_sparam_t *param,
			      struct gs_shader_param_info *info)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(info);
}

void gs_shader_set_bool(gs_sparam_t *param, bool val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_float(gs_sparam_t *param, float val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_int(gs_sparam_t *param, int val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_matrix3(gs_sparam_t *param, const struct matrix3 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_matrix4(gs_sparam_t *param, const struct matrix4 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec2(gs_sparam_t *param, const struct vec2 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec3(gs_sparam_t *param, const struct vec3 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec4(gs_sparam_t *param, const struct vec4 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_texture(gs_sparam_t *param, gs_texture_t *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_val(gs_sparam_t *param, const void *val, size_t size)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
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

void gs_shader_set_default(gs_sparam_t *param)
{
	if (!param->defaultValue.empty())
		shader_setval_inline(param, param->defaultValue.data(),
				     param->defaultValue.size());
}

void gs_shader_set_next_sampler(gs_sparam_t *param, gs_samplerstate_t *sampler)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(sampler);
}
