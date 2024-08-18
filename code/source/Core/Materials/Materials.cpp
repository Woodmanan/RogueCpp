#include "Materials.h"
#include "../../Debug/Debug.h"
#include <iostream>
#include <algorithm>

MaterialManager* MaterialManager::manager = nullptr;

const MaterialDefinition& Material::GetMaterial() const
{
	return MaterialManager::Get()->GetMaterialByID(m_materialID);
}

void MaterialContainer::AddMaterial(int materialIndex, float mass, bool staticMaterial, int index)
{
	Material mat = Material(materialIndex, mass, staticMaterial);
	AddMaterial(mat, index);
}

void MaterialContainer::AddMaterial(const Material& material, int index)
{
	if (material.m_mass == 0) { return; }

	std::cout << "Adding " << material.m_mass << " kgs of " << material.GetMaterial().name << std::endl;
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
		
		if (m_materials[i].m_mass > mass)
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

void MaterialContainer::Debug_Print()
{
	std::cout << "Printing container contents:" << std::endl;
	int layerStart = 0;
	for (int layer = 0; layer < m_layers.size(); layer++)
	{
		int layerEnd = m_layers[layer];
		std::cout << "\tSort layer from [" << layerStart << ", " << layerEnd << ")" << std::endl;
		for (int material = layerStart; material < layerEnd; material++)
		{
			std::cout << "\t\t" << material << ": ";
			if (m_materials[material].m_materialID < 0)
			{
				std::cout << "[invalid id: " << m_materials[material].m_materialID << "]";
			}
			else
			{
				const MaterialDefinition& fullMat = m_materials[material].GetMaterial();
				std::cout << fullMat.name << " (" << fullMat.density << " kg/m^3)";
			}
			std::cout << ", " << m_materials[material].m_mass << " kgs";
			if (m_materials[material].GetStatic())
			{
				std::cout << " [Static]";
			}
			std::cout << std::endl;
		}
	}
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
	std::cout << "Sort layer from [" << startIndex << ", " << endIndex << ")" << std::endl;
	if (m_materials[startIndex].m_static)
	{
		startIndex++;
	}

	if (startIndex != endIndex)
	{
		sort(m_materials.begin() + startIndex, m_materials.begin() + endIndex, [](const Material& rhs, const Material& lhs)
			{
				const float rhsDensity = rhs.GetMaterial().density;
				const float lhsDensity = lhs.GetMaterial().density;
				if (rhsDensity == lhsDensity)
				{
					return rhs.m_materialID < lhs.m_materialID;
				}
				return rhsDensity > lhsDensity;
			});
	}
}

void MixtureContainer::LoadMixture(MaterialContainer& one, MaterialContainer& two)
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

int MixtureContainer::LoadContainer(MaterialContainer& container, FixedArray<Material, MixtureContainer::maxIndices>& sortedArray)
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

void MaterialManager::EvaluateReaction(MaterialContainer& one, MaterialContainer& two)
{
	//Having an empty container invalidates a reaction - self reaction will catch it!
	if (one.m_materials.size() == 0 || two.m_materials.size() == 0) { return; }

	MixtureContainer mixture;
	mixture.LoadMixture(one, two);

	for (Reaction& reaction : m_reactions)
	{
		if (ReactionMatchesMixture(reaction, mixture))
		{
 			ExecuteReaction(reaction, mixture, one, two);
			return;
		}
	}
}

void MaterialManager::ExecuteReaction(Reaction& reaction, MixtureContainer& mixture, MaterialContainer& one, MaterialContainer& two)
{
	float reactionMultiple = GetReactionMultiple(reaction, mixture);
	std::cout << "Starting a reaction! (x" << reactionMultiple << ")" << std::endl;

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
}

const MaterialDefinition& MaterialManager::GetMaterialByID(int index)
{
	ASSERT(index >= 0 && index < m_materialDefinitions.size());
	return m_materialDefinitions[index];
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

bool MaterialManager::ReactionMatchesMixture(Reaction& reaction, MixtureContainer& mixture)
{
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