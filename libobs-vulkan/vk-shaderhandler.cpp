/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

/*
 *  Modified for Vulkan use
 */

#include "vk-subsystem.hpp"
#include "vk-shaderhandler.hpp"
#include "vk-helpers.hpp"

#include <map>
#include <sstream>

static const char *semanticInputNames[] = {"POSITION", "NORMAL",   "COLOR",
					   "TANGENT",  "TEXCOORD", "VERTEXID"};
static const char *semanticOutputNames[] = {
	"SV_Position", "NORMAL", "COLOR", "TANGENT", "TEXCOORD", "VERTEXID"};

static const char *ConvertSemanticName(const char *name)
{
	const size_t num = sizeof(semanticInputNames) / sizeof(const char *);
	for (size_t i = 0; i < num; i++) {
		if (strcmp(name, semanticInputNames[i]) == 0)
			return semanticOutputNames[i];
	}

	throw std::runtime_error("Unknown Semantic Name");
}

static void GetSemanticInfo(shader_var *var, const char *&name, uint32_t &index)
{
	const char *mapping = var->mapping;
	const char *indexStr = mapping;

	while (*indexStr && !isdigit(*indexStr))
		indexStr++;
	index = (*indexStr) ? strtol(indexStr, NULL, 10) : 0;

	std::string nameStr;
	nameStr.assign(mapping, indexStr - mapping);
	name = ConvertSemanticName(nameStr.c_str());
}

static void AddInputLayoutVar(shader_var *var,
			      ShaderInputs &layout)
{
	vk::VertexInputAttributeDescription vkad;
	const char *semanticName;
	uint32_t semanticIndex;

	GetSemanticInfo(var, semanticName, semanticIndex);

	vkad.binding = semanticIndex;

	if (strcmp(var->mapping, "COLOR") == 0) {
		vkad.offset = layout.lastOffset;
		vkad.format = vk::Format::eR32Uint;
		layout.lastOffset += 4;
		vkad.location = static_cast<uint32_t>(layout.descs.size());
	} else if (strcmp(var->mapping, "POSITION") == 0 ||
		   strcmp(var->mapping, "NORMAL") == 0 ||
		   strcmp(var->mapping, "TANGENT") == 0) {
		vkad.offset = layout.lastOffset;
		vkad.format = vk::Format::eR32G32B32A32Sfloat;
		if (strcmp(var->mapping, "POSITION") == 0)
			vkad.offset = offsetof(vk_vb_data, vk_vb_data::point);
		else if (strcmp(var->mapping, "NORMAL") == 0)
			vkad.offset = offsetof(vk_vb_data, vk_vb_data::normal);
		else
			vkad.offset = offsetof(vk_vb_data, vk_vb_data::tangent);

		layout.lastOffset += 16;
		vkad.location = static_cast<uint32_t>(layout.descs.size());
	} else if (astrcmp_n(var->mapping, "TEXCOORD", 8) == 0) {
		vkad.offset = layout.lastOffset;
		/* type is always a 'float' type */
		switch (var->type[5]) {
		case 0:
			vkad.format = vk::Format::eR32Sfloat;
			layout.lastOffset += 4;
			break;
		case '2':
			vkad.format = vk::Format::eR32G32Sfloat;
			layout.lastOffset += 8;
			break;
		case '3':
		case '4':
			vkad.format = vk::Format::eR32G32B32A32Sfloat;
			layout.lastOffset += 16;
			break;
		}
		vkad.location = static_cast<uint32_t>(layout.descs.size());
	}

	layout.names.emplace_back(semanticName);
	layout.descs.emplace_back(vkad);
}

static inline bool SetSlot(ShaderInputs &layout,
			   const char *name, uint32_t index, uint32_t &slotIdx)
{
	for (size_t i = 0; i < layout.descs.size(); i++) {
		auto &input = layout.descs[i];
		if (input.binding == index &&
		    strcmpi(layout.names[i].c_str(), name) == 0) {
			layout.descs[i].location = slotIdx++;
			return true;
		}
	}

	return false;
}

static void BuildInputLayoutFromVars(shader_parser *parser, darray *vars,
				     ShaderInputs &layout)
{
	shader_var *array = (shader_var *)vars->array;

	for (size_t i = 0; i < vars->num; i++) {
		shader_var *var = array + i;
		if (var->mapping) {
			if (strcmp(var->mapping, "VERTEXID") != 0)
				AddInputLayoutVar(var, layout);
		} else {
			shader_struct *st =
				shader_parser_getstruct(parser, var->type);
			if (st)
				BuildInputLayoutFromVars(parser, &st->vars.da,
							 layout);
		}
	}

	/*
	 * Sets the input slot value for each semantic, however we do it in
	 * a specific order so that it will always match the vertex buffer's
	 * sub-buffer order (points-> normals-> colors-> tangents-> uvcoords)
	 */
	uint32_t slot = 0;
	SetSlot(layout, "SV_Position", 0, slot);
	SetSlot(layout, "NORMAL", 0, slot);
	SetSlot(layout, "COLOR", 0, slot);
	SetSlot(layout, "TANGENT", 0, slot);

	uint32_t index = 0;
	while (SetSlot(layout, "TEXCOORD", index++, slot))
		;
}

void ShaderProcessor::BuildInputLayout(ShaderInputs &layout)
{
	shader_func *func = shader_parser_getfunc(&parser, "main");
	if (!func)
		throw std::runtime_error("Failed to find 'main' shader function");

	BuildInputLayoutFromVars(&parser, &func->params.da, layout);
}

static inline void AddParam(shader_var &var, std::vector<gs_shader_param> &params,
			    uint32_t &texCounter)
{
	if (var.var_type != SHADER_VAR_UNIFORM ||
	    strcmp(var.type, "sampler") == 0)
		return;

	params.emplace_back(var, texCounter);
}

void ShaderProcessor::BuildParams(std::vector<gs_shader_param> &params)
{
	uint32_t texCounter = 0;

	for (size_t i = 0; i < parser.params.num; i++)
		AddParam(parser.params.array[i], params, texCounter);
}

static inline void AddSampler(gs_device_t *device, shader_sampler &sampler,
			      std::vector<std::unique_ptr<gs_samplerstate_t>> &samplers)
{
	gs_sampler_info si;
	shader_sampler_convert(&sampler, &si);
	samplers.emplace_back(std::make_unique<gs_sampler_state>(device, &si));
}

void ShaderProcessor::BuildSamplers(std::vector<std::unique_ptr<gs_samplerstate_t>> &samplers)
{
	for (size_t i = 0; i < parser.samplers.num; i++)
		AddSampler(device, parser.samplers.array[i], samplers);
}

void ShaderProcessor::BuildString(std::string &outputString)
{
	std::stringstream output;
	output << "static const bool obs_glsl_compile = false;\n\n";

	cf_token *token = cf_preprocessor_get_tokens(&parser.cfp.pp);
	while (token->type != CFTOKEN_NONE) {
		/* cheaply just replace specific tokens */
		if (strref_cmp(&token->str, "POSITION") == 0)
			output << "SV_Position";
		else if (strref_cmp(&token->str, "TARGET") == 0)
			output << "SV_Target";
		else if (strref_cmp(&token->str, "texture2d") == 0)
			output << "Texture2D";
		else if (strref_cmp(&token->str, "texture3d") == 0)
			output << "Texture3D";
		else if (strref_cmp(&token->str, "texture_cube") == 0)
			output << "TextureCube";
		else if (strref_cmp(&token->str, "texture_rect") == 0)
			throw std::runtime_error(
				"texture_rect is not supported");
		else if (strref_cmp(&token->str, "sampler_state") == 0)
			output << "SamplerState";
		else if (strref_cmp(&token->str, "VERTEXID") == 0)
			output << "SV_VertexID";
		else
			output.write(token->str.array, token->str.len);

		token++;
	}

	outputString = std::move(output.str());
}

void ShaderProcessor::Process(const char *shader_string, const char *file)
{
	bool success = shader_parse(&parser, shader_string, file);
	char *str = shader_parser_geterrors(&parser);
	if (str) {
		blog(LOG_WARNING, "Shader parser errors/warnings:\n%s\n", str);
		bfree(str);
	}

	if (!success)
		throw std::runtime_error("Failed to parse shader");
}

vk::DescriptorType GetDescriptorType(const std::string& line)
{
	/* if line contains Texture2D, returns combined image sampler */
	/* uniform otherwise */
	if (line.find("Texture2D") != std::string::npos)
		return vk::DescriptorType::eCombinedImageSampler;
	else
		return vk::DescriptorType::eUniformBuffer;
}

static uint32_t bindOffset = 0;
static std::map<std::string, uint32_t> bindings;
static std::map<std::string, uint32_t> locations;
std::string ShaderProcessor::Vulkanify(gs_shader_t *shader,
				       const std::string &shaderCode,
				       bool resetBindIndex)
{
	if (resetBindIndex) {
		bindOffset = 0;
		bindings.clear();
		locations.clear();
	}

	std::string line;
	std::stringstream ss;
	std::stringstream out;
	ss << shaderCode;

	/* go through each line and add required bindings and locations */
	uint32_t lastTexIdx = 0, locIdx = 0;
	while (std::getline(ss, line)) {
		if (line.find("uniform") != std::string::npos) {
			/* if identical line exists in bindings, use it's index */
			/* otherwise, increase index */
			if (bindings.find(line) != bindings.end()) {
				const auto t = GetDescriptorType(line);
				shader->bindings[bindings[line]] = t;
				line.insert(0, "[[vk::binding(" +
						       std::to_string(
							       bindings[line]) +
						       ")]] ");
				if (t ==
				    vk::DescriptorType::eCombinedImageSampler) {
					line.insert(
						0,
						"[[vk::combinedImageSampler]]");
					lastTexIdx = bindings[line];
				}
			} else {
				bindings[line] = bindOffset;
				const auto t = GetDescriptorType(line);
				shader->bindings[bindOffset] = t;
				line.insert(0,
					    "[[vk::binding(" +
						    std::to_string(bindOffset) +
						    ")]] ");
				if (t ==
				    vk::DescriptorType::eCombinedImageSampler) {
					line.insert(
						0,
						"[[vk::combinedImageSampler]]");
					lastTexIdx = bindOffset;
				}
			}
			bindOffset++;
		} else if (line.find("SamplerState") != std::string::npos) {
			line.insert(
				0,
				"[[vk::combinedImageSampler]][[vk::binding(" +
					std::to_string(lastTexIdx) + ")]] ");
		} else if (resetBindIndex &&
			   (line.find("float4 pos : SV_Position;") !=
				    std::string::npos ||
			    line.find("float4 color : COLOR;") !=
				    std::string::npos ||
			    line.find("float2 uv : TEXCOORD0;") !=
				    std::string::npos)) {
			const auto id = GetStringBetweenT(line, ": ", ";");
			if (locations.find(line) != locations.end()) {
				shader->locations[locations[line]] = id;
				PrependToStringBefore(
					line, "float",
					"[[vk::location(" +
						std::to_string(
							locations[line]) +
						")]] ");
			} else {
				locations[line] = locIdx;
				PrependToStringBefore(
					line, "float",
					"[[vk::location(" +
						std::to_string(locIdx) +
						")]] ");
				shader->locations[locIdx] = id;
			}
			locIdx++;
		} else if (!resetBindIndex &&
			   (line.find("float4 pos : SV_Position;") !=
				    std::string::npos ||
			    line.find("float4 color : COLOR;") !=
				    std::string::npos ||
			    line.find("float2 uv : TEXCOORD0;") !=
				    std::string::npos)) {
			if (locations.find(line) != locations.end()) {
				PrependToStringBefore(
					line, "float",
					"[[vk::location(" +
						std::to_string(
							locations[line]) +
						")]] ");
			}
		}

		out << line << std::endl;
	}

	return out.str();
}