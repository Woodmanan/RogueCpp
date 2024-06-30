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
	std::cout << "Adding " << material.m_mass << " kgs of " << material.GetMaterial().name << std::endl;
	if (m_matSize == maxIndices)
	{
		ASSERT(false);
		return;
	}

	if (index == -1)
	{
		index = m_matSize;
	}

	for (int matIndex = m_matSize - 1; matIndex >= index; matIndex--)
	{
		m_materials[matIndex + 1] = m_materials[matIndex];
	}

	m_materials[index] = material;
	m_matSize++;
	SortLayers();
}

void MaterialContainer::SortLayers()
{
	int currentLayer = 0;
	for (int i = 0; i < m_matSize; i++)
	{
		if (m_materials[i].GetStatic())
		{
			if (i - currentLayer > 0)
			{
				m_layers[currentLayer] = i;
				currentLayer++;
			}
		}
	}

	if (currentLayer == -1) { currentLayer++; }
	m_layers[currentLayer] = m_matSize;

	m_layerSize = currentLayer + 1;

	int lastIndex = 0;
	for (int i = 0; i < m_layerSize; i++)
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
	for (int layer = 0; layer < m_layerSize; layer++)
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
	
	m_matSize = realIndex;
}

void MaterialContainer::Debug_Print()
{
	std::cout << "Printing container contents:" << std::endl;
	int layerStart = 0;
	for (int layer = 0; layer < m_layerSize; layer++)
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
		for (int j = m_reactants.size() - 1; j >= i; j++)
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