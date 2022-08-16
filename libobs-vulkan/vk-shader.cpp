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

	/* SPIR-V compilation */
	blog(LOG_INFO, "Compiling Shader to SPIR-V: %s", file);

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetEnvironment(shaderc_target_env_vulkan,
				     shaderc_env_version_vulkan_1_2);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);
	options.SetSourceLanguage(shaderc_source_language_hlsl);
	options.SetTargetSpirv(shaderc_spirv_version_1_4);

	if (type == GS_SHADER_VERTEX) {
		const auto compiled = compiler.CompileGlslToSpv(
			hlslSource.c_str(), hlslSource.size(),
			shaderc_glsl_vertex_shader, file, options);

		if (compiled.GetCompilationStatus() !=
		    shaderc_compilation_status_success)
			throw std::runtime_error((compiled.GetErrorMessage() + std::string("\n\n\n") + hlslSource + std::string("\n\n\n")));

		spirv = {compiled.cbegin(), compiled.cend()};
	} else if (type == GS_SHADER_PIXEL) {
		const auto compiled = compiler.CompileGlslToSpv(
			hlslSource.c_str(), hlslSource.size(),
			shaderc_glsl_fragment_shader, file, options);

		if (compiled.GetCompilationStatus() !=
		    shaderc_compilation_status_success)
			throw std::runtime_error((compiled.GetErrorMessage() + std::string("\n\n\n") + hlslSource + std::string("\n\n\n")));

		spirv = {compiled.cbegin(), compiled.cend()};
	} else
		throw std::runtime_error("Invalid Shader type");
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
