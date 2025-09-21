#include "Materials.h"
#include "Debug/Debug.h"
#include "Utils/Utils.h"
#include "Utils/FileUtils.h"
#include "Data/SaveManager.h"
#include "Game/ThreadManagers.h"
#include <iostream>
#include <algorithm>

MaterialManager::MaterialManager()
{

}

void MaterialManager::Init()
{
	std::vector<ResourcePointer> materials = ResourceManager::Get()->LoadFromConfigSynchronous("Mat", "materials");
	for (ResourcePointer& matArray : materials)
	{
		TResourcePointer<std::vector<MaterialDefinition>> pointer = matArray;
		for (auto it = pointer->begin(); it != pointer->end(); it++)
		{
			AddMaterialDefinition(*it);
		}
	}

	std::vector<ResourcePointer> reactions = ResourceManager::Get()->LoadFromConfigSynchronous("Reaction", "reactions");
	for (ResourcePointer& array : reactions)
	{
		TResourcePointer<std::vector<Reaction>> pointer = array;
		for (auto it = pointer->begin(); it != pointer->end(); it++)
		{
			AddReaction(*it);
		}
	}
}

const MaterialDefinition& Material::GetMaterial() const
{
	return GetMaterialManager()->GetMaterialByID(m_materialID);
}

void MaterialContainer::AddMaterial(int materialIndex, float mass, bool staticMaterial, int index)
{
	Material mat = Material(materialIndex, mass, staticMaterial);
	AddMaterial(mat, index);
}

void MaterialContainer::AddMaterial(const std::string& name, float mass, bool staticMaterial)
{
	int ID = GetMaterialManager()->GetMaterialByName(name).ID;
	Material mat = Material(ID, mass, staticMaterial);
	AddMaterial(mat, -1);
}

void MaterialContainer::AddMaterial(const Material& material, int index)
{
	if (material.m_mass == 0) { return; }

	if (m_materials.size() == maxIndices)
	{
		ASSERT(false);
		return;
	}

	if (index == -1)
	{
		m_materials.push_back(material);
	}
	else
	{
		m_materials.insert(material, index);
	}

	SortLayers();
}

void MaterialContainer::RemoveMaterial(const MaterialDefinition& material, float mass)
{
	if (mass == 0) { return; }

	int layerMin = (m_layers.size() >= 2) ? m_layers[m_layers.size() - 2] : 0;
	int layerMax = m_layers[m_layers.size() - 1];

	for (int i = layerMin; i < layerMax; i++)
	{
		if (m_materials[i].m_materialID != material.ID) { continue; }
		
		if ((m_materials[i].m_mass - mass) > 0.01f)
		{
			m_materials[i].m_mass -= mass;
			return;
		}
		else
		{
			m_materials.remove(i);
			SortLayers();
			return;
		}
	}

	//Removed material wasn't in the last layer!
	HALT();
}

void MaterialContainer::SortLayers()
{
	m_layers.clear();
	int currentLayer = 0;
	for (int i = 0; i < m_materials.size(); i++)
	{
		if (m_materials[i].GetStatic())
		{
			if (i - currentLayer > 0)
			{
				m_layers.push_back(i);
				currentLayer++;
			}
		}
	}

	m_layers.push_back(m_materials.size());

	int lastIndex = 0;
	for (int i = 0; i < m_layers.size(); i++)
	{
		SortLayerByDensity(lastIndex, m_layers[i]);
		lastIndex = m_layers[i];
	}

	CollapseDuplicates();
}

void MaterialContainer::CollapseDuplicates()
{
	int realIndex = 0;
	int layerStart = 0;
	for (int layer = 0; layer < m_layers.size(); layer++)
	{
		int layerEnd = m_layers[layer];
		for (int mat = layerStart; mat < layerEnd; mat++)
		{
			bool shouldAdd = true;
			if ((mat != layerStart) && m_materials[mat].m_materialID == m_materials[realIndex-1].m_materialID)
			{
				shouldAdd = false;
				m_materials[realIndex - 1].m_mass += m_materials[mat].m_mass;
			}

			if (shouldAdd)
			{
				m_materials[realIndex] = m_materials[mat];
				realIndex++;
			}
		}
		layerStart = layerEnd;

		m_layers[layer] = realIndex;
	}
	
	m_materials.resize(realIndex);
}

void MaterialContainer::SortLayerByDensity(int startIndex, int endIndex)
{
	if (startIndex == 0 && endIndex == 0)
	{
		m_layers.resize(1);
		m_layers[0] = 0;
		return;
	}

	ASSERT(startIndex != endIndex);
	if (m_materials[startIndex].m_static)
	{
		startIndex++;
	}

	if (startIndex != endIndex)
	{
		sort(m_materials.begin() + startIndex, m_materials.begin() + endIndex, [&](const Material& rhs, const Material& lhs)
			{
				const float rhsDensity = rhs.GetMaterial().density;
				const float lhsDensity = lhs.GetMaterial().density;
				if (rhsDensity == lhsDensity)
				{
					return rhs.m_materialID < lhs.m_materialID;
				}
				return (rhsDensity > lhsDensity) != m_inverted; //XOR with m_inverted to reverse density sort on inverted containers
			});
	}
}

void MixtureContainer::LoadMixture(const MaterialContainer& one, const MaterialContainer& two)
{
	m_materials.clear();
	m_mixture.clear();
	FixedArray<Material, maxIndices> oneMaterials;
	FixedArray<Material, maxIndices> twoMaterials;

	int oneMax = LoadContainer(one, oneMaterials);
	int twoMax = LoadContainer(two, twoMaterials);

	int oneIdx = 0;
	int twoIdx = 0;

	while (oneIdx < oneMax && twoIdx < twoMax)
	{
		const Material& matOne = oneMaterials[oneIdx];
		const Material& matTwo = twoMaterials[twoIdx];

		if (matOne.m_materialID == matTwo.m_materialID)
		{
			m_materials.push_back(Material(matOne.m_materialID, matOne.m_mass + matTwo.m_mass, false));
			m_mixture.push_back(matOne.m_mass / m_materials.last().m_mass);
			oneIdx++;
			twoIdx++;
		}
		else if (matOne.m_materialID < matTwo.m_materialID)
		{

			m_materials.push_back(Material(matOne.m_materialID, matOne.m_mass, false));
			m_mixture.push_back(1);
			oneIdx++;
		}
		else
		{
			m_materials.push_back(Material(matTwo.m_materialID, matTwo.m_mass, false));
			m_mixture.push_back(0);
			twoIdx++;
		}
	}

	//Add in all remaining elements in order
	for (int index = oneIdx; index < oneMax; index++)
	{
		const Material& matOne = oneMaterials[index];
		m_materials.push_back(Material(matOne.m_materialID, matOne.m_mass, false));
		m_mixture.push_back(1);
	}

	for (int index = twoIdx; index < twoMax; index++)
	{
		const Material& matTwo = twoMaterials[index];
		m_materials.push_back(Material(matTwo.m_materialID, matTwo.m_mass, false));
		m_mixture.push_back(0);
	}

	ASSERT(m_materials.size() == m_mixture.size());
}

void MixtureContainer::LoadMixture(MaterialContainer& single, int layer)
{
	int below = layer - 1;

	const int minIdx = (layer >= 0) ? single.m_layers[below] : 0;
	const int maxIdx = single.m_layers[layer];

	m_materials.clear();
	m_mixture.clear();

	for (int i = minIdx; i < maxIdx; i++)
	{
		const Material& mat = single.m_materials[minIdx + i];
		m_materials.push_back(Material(mat.m_materialID, mat.m_mass, false));
		m_mixture.push_back(1);
	}
}

int MixtureContainer::LoadContainer(const MaterialContainer& container, FixedArray<Material, MixtureContainer::maxIndices>& sortedArray)
{
	ASSERT(container.m_layers.size() > 0);
	int startIndex = container.m_layers.size() - 2;
	int endIndex = container.m_layers.size() - 1;

	int matStartIndex = (startIndex >= 0) ? container.m_layers[startIndex] : 0;
	int matEndIndex = container.m_layers[endIndex];

	for (int matIndex = matStartIndex; matIndex < matEndIndex; matIndex++)
	{
		sortedArray.push_back(container.m_materials[matIndex]);
	}

	std::sort(sortedArray.begin(), sortedArray.end());
	return sortedArray.size();
}

int MixtureContainer::GetIndexOf(const MaterialDefinition& material)
{
	for (int i = 0; i < m_materials.size(); i++)
	{
		if (m_materials[i].m_materialID == material.ID)
		{
			return i;
		}
	}

	HALT();
	return -1;
}

void Reaction::SortReactantsByID()
{
	//Add extra empty results if needed. Used to maintain reactant->result relationships
	while (m_products.size() < m_reactants.size())
	{
		m_products.push_back(Material());
	}

	//Bubble sort!
	for (int i = 0; i < m_reactants.size(); i++)
	{
		for (int j = m_reactants.size() - 1; j > i; j--)
		{
			if (m_reactants[j].m_materialID < m_reactants[j - 1].m_materialID)
			{
				std::swap(m_reactants[j - 1], m_reactants[j]);
				std::swap(m_products[j - 1], m_products[j]);
			}
		}
	}
}

void Reaction::Debug_Print()
{

}

void MaterialManager::AddMaterialDefinition(MaterialDefinition material)
{
	material.ID = m_materialDefinitions.size();
	m_materialDefinitions.push_back(material);
}

void MaterialManager::AddReaction(Reaction reaction)
{
	reaction.SortReactantsByID();
	m_reactions.push_back(reaction);
}

bool MaterialManager::CheckReaction(MaterialContainer& one, MaterialContainer& two, float& heat) const
{
	ROGUE_PROFILE_SECTION("Check Reaction");
	//Having an empty container invalidates a reaction - self reaction will catch it!
	if (one.m_materials.size() == 0 || two.m_materials.size() == 0) { return false; }

	MixtureContainer mixture;
	mixture.LoadMixture(one, two);

	for (const Reaction& reaction : m_reactions)
	{
		if (ReactionMatchesMixture(reaction, mixture, heat))
		{
			return true;
		}
	}

	return false;
}

bool MaterialManager::EvaluateReaction(MaterialContainer& one, MaterialContainer& two, float& heat)
{
	//Having an empty container invalidates a reaction - self reaction will catch it!
	if (one.m_materials.size() == 0 || two.m_materials.size() == 0) { return false; }

	MixtureContainer mixture;
	mixture.LoadMixture(one, two);

	for (Reaction& reaction : m_reactions)
	{
		if (ReactionMatchesMixture(reaction, mixture, heat))
		{
 			ExecuteReaction(reaction, mixture, one, two, heat);
			return true;
		}
	}

	return false;
}

void MaterialManager::ExecuteReaction(Reaction& reaction, MixtureContainer& mixture, MaterialContainer& one, MaterialContainer& two, float& heat)
{
	float reactionMultiple = GetReactionMultiple(reaction, mixture);

	for (Material& reactant : reaction.m_reactants)
	{
		const MaterialDefinition& definition = reactant.GetMaterial();
		int index = mixture.GetIndexOf(definition);

		float oneAmount = mixture.m_mixture[index];
		float twoAmount = 1 - oneAmount;
		ASSERT((oneAmount + twoAmount) == 1);

		one.RemoveMaterial(definition, oneAmount * reactant.m_mass * reactionMultiple);
		two.RemoveMaterial(definition, twoAmount * reactant.m_mass * reactionMultiple);
	}

	for (int i = 0; i < reaction.m_products.size(); i++)
	{
		if (reaction.m_products[i].m_materialID == -1) { continue; }

		float oneAmount = .5f;
		float twoAmount = .5f;

		if (i < reaction.m_reactants.size())
		{
			const MaterialDefinition& definition = reaction.m_reactants[i].GetMaterial();
			int index = mixture.GetIndexOf(definition);

			oneAmount = mixture.m_mixture[index];
			twoAmount = 1 - oneAmount;
		}

		ASSERT((oneAmount + twoAmount) == 1);
		Material& material = reaction.m_products[i];

		one.AddMaterial(Material(material.m_materialID, material.m_mass * oneAmount * reactionMultiple, false));
		two.AddMaterial(Material(material.m_materialID, material.m_mass * twoAmount * reactionMultiple, false));
	}

	heat += (reaction.m_deltaHeat * reactionMultiple);
}

const MaterialDefinition& MaterialManager::GetMaterialByID(int index)
{
	ASSERT(index >= 0 && index < m_materialDefinitions.size());
	ASSERT(m_materialDefinitions[index].ID == index);
	return m_materialDefinitions[index];
}

const MaterialDefinition& MaterialManager::GetMaterialByName(const std::string& name)
{
	for (int index = 0; index < m_materialDefinitions.size(); index++)
	{
		if (m_materialDefinitions[index].name == name)
		{
			return m_materialDefinitions[index];
		}
	}

	HALT();
}

void MaterialManager::SortReactions()
{
	sort(m_reactions.begin(), m_reactions.end(), [](const Reaction& lhs, const Reaction& rhs)
		{
			if (lhs.m_reactants.size() == rhs.m_reactants.size())
			{
				//TODO: Make sorting sort the correct way.
				return true;
			}

			return lhs.m_reactants.size() > rhs.m_reactants.size();
		});
}

bool MaterialManager::ReactionMatchesMixture(const Reaction& reaction, MixtureContainer& mixture, const float& heat) const
{
	if (heat < reaction.m_minHeat || heat > reaction.m_maxHeat)
	{
		return false;
	}

	auto mixtureIterator = mixture.m_materials.begin();

	for (const Material& reactant : reaction.m_reactants)
	{
		while (mixtureIterator->m_materialID < reactant.m_materialID)
		{
			mixtureIterator++;
			if (mixtureIterator == mixture.m_materials.end()) { return false; }
		}

		if (mixtureIterator->m_materialID == reactant.m_materialID)
		{
			if (reactant.m_mass > mixtureIterator->m_mass) { return false; }
		}
		else
		{
			return false;
		}
	}

	return true;
}

float MaterialManager::GetReactionMultiple(Reaction& reaction, MixtureContainer& mixture)
{
	float maxAmount = INFINITY;
	auto mixtureIterator = mixture.m_materials.begin();

	for (const Material& reactant : reaction.m_reactants)
	{
		while (mixtureIterator->m_materialID < reactant.m_materialID)
		{
			mixtureIterator++;
		}

		maxAmount = std::min(maxAmount, mixtureIterator->m_mass / reactant.m_mass);
	}

	ASSERT(maxAmount >= 1);
	return maxAmount;
}

namespace RogueResources
{
	void PackMaterial(PackContext& packContext)
	{
		std::ifstream stream;
		stream.open(packContext.source);

		SkipLine(stream);

		std::vector<MaterialDefinition> materials;

		while (stream.is_open())
		{
			std::string line;
			std::getline(stream, line);
			if (line.empty()) { break; }

			std::vector<std::string> tokens = string_split(line, ",");

			ASSERT(tokens.size() == 7);

			if (tokens[0].empty()) { continue; }

			//Name, Density, Movement Cost, floor char, wall char, Color, Phase

			MaterialDefinition definition;
			definition.ID = -1;
			definition.name = tokens[0];
			definition.density = string_to_float(tokens[1]);
			definition.movementCost = string_to_float(tokens[2]);
			definition.floorChar = tokens[3].empty() ? '\0' : tokens[3][0];
			definition.wallChar = tokens[4].empty() ? '\0' : tokens[4][0];
			definition.color = string_to_color(tokens[5]);
			definition.phase = magic_enum::enum_cast<Phase>(tokens[6]).value();

			materials.push_back(definition);
		}

		stream.close();

		OpenWritePackFile(packContext.destination, packContext.header);
		RogueSaveManager::Write("Materials", materials);
		RogueSaveManager::CloseWriteSaveFile();
	}

	std::shared_ptr<void> LoadMaterial(LoadContext& loadContext)
	{
		std::vector<MaterialDefinition>* materials = new std::vector<MaterialDefinition>();

		OpenReadPackFile(loadContext.source);
		RogueSaveManager::Read("Materials", *materials);
		RogueSaveManager::CloseReadSaveFile();

		return std::shared_ptr<std::vector<MaterialDefinition>>(materials);
	}

	void PackReaction(PackContext& packContext)
	{
		std::vector<ResourcePointer> materialLists = ResourceManager::Get()->LoadFromConfigSynchronous("Mat", "materials", &packContext);

		auto FindIndex = [materialLists](const std::string& name) {
			int index = 0;
			for (int i = 0; i < materialLists.size(); i++)
			{
				TResourcePointer<std::vector<MaterialDefinition>> materials = materialLists[i];
				for (int j = 0; j < materials->size(); j++)
				{
					const MaterialDefinition& material = materials->operator[](j);
					if (material.name == name)
					{
						return index;
					}
					index++;
				}
			}

			return -1;
		};

		std::ifstream stream;
		stream.open(packContext.source);

		SkipLine(stream);

		std::vector<Reaction> reactions;

		while (stream.is_open())
		{
			std::string line;
			std::getline(stream, line);
			if (line.empty()) { break; }

			std::vector<std::string> tokens = string_split(line, ",");

			ASSERT(tokens.size() == 6);

			if (tokens[0].empty()) { continue; }

			Reaction nextReaction;

			//Name,Inputs,Outputs,MinTemp,DeltaTemp
			nextReaction.name = tokens[0];

			std::vector<std::string> inputs = string_split(tokens[1], " + ");
			std::vector<std::string> outputs = string_split(tokens[2], " + ");

			for (std::string& input : inputs)
			{
				std::vector<std::string> item = string_split(input, " ");
				ASSERT(item.size() == 2);
				nextReaction.m_reactants.push_back(Material(FindIndex(item[1]), string_to_float(item[0]), false));
			}

			for (std::string& output : outputs)
			{
				if (output.size() == 1 && output[0] == '_') //Catch wildcard output
				{
					nextReaction.m_products.push_back(Material(-1, -1, false));
					continue;
				}

				std::vector<std::string> item = string_split(output, " ");
				ASSERT(item.size() == 2);
				nextReaction.m_products.push_back(Material(FindIndex(item[1]), string_to_float(item[0]), false));
			}

			nextReaction.m_minHeat = string_to_float(tokens[3]);
			nextReaction.m_maxHeat = string_to_float(tokens[4]);
			nextReaction.m_deltaHeat = string_to_float(tokens[5]);
			nextReaction.SortReactantsByID();

			reactions.push_back(nextReaction);
		}

		stream.close();

		OpenWritePackFile(packContext.destination, packContext.header);
		RogueSaveManager::Write("Reactions", reactions);
		RogueSaveManager::CloseWriteSaveFile();
	}

	std::shared_ptr<void> LoadReaction(LoadContext& loadContext)
	{
		std::vector<Reaction>* reactions = new std::vector<Reaction>();

		OpenReadPackFile(loadContext.source);
		RogueSaveManager::Read("Materials", *reactions);
		RogueSaveManager::CloseReadSaveFile();

		return std::shared_ptr<std::vector<Reaction>>(reactions);
	}
}