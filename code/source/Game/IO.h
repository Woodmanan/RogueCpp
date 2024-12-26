#include "Core/CoreDataTypes.h"
#include <string>
#include <memory>

/*
	IO controls! Defines the structs and serialization that we will need for communicating with our game threads and servers.
*/

enum EInputType
{
	Invalid,
	Movement,
	BeginNewGame,
	BeginSeededGame,
	LoadSaveGame,
	SaveAndExit,
	ExitGame
};

class InputBase
{
public:
	virtual EInputType GetType() const { return EInputType::Invalid; }
	virtual void Serialize() {}
	virtual void Deserialize() {}
};

template<EInputType inputType>
class TInputBase : public InputBase
{
public:
	EInputType GetType() const override { return inputType; }
	const static EInputType type = inputType;
};

class MoveInput : public TInputBase<Movement>
{
public:
	void Serialize();
	void Deserialize();

	Vec2 direction;
};

struct BeginSeededGameInput : public TInputBase<BeginSeededGame>
{
public:
	void Serialize();
	void Deserialize();

	int seed;
};

struct LoadSaveInput : public TInputBase<LoadSaveGame>
{
public:
	LoadSaveInput() { fileName = ""; }
	LoadSaveInput(const std::string& file) { fileName = file; }

	void Serialize();
	void Deserialize();

	std::string fileName = "";
};

struct Input
{
public:
	EInputType m_type;
	std::shared_ptr<InputBase> m_data;

	void Set(EInputType type, std::shared_ptr<InputBase> data = nullptr)
	{
		m_type = type;
		m_data = data;
	}

	template <EInputType type>
	void Set()
	{
		m_type = type;
		m_data = nullptr;
	}

	template <EInputType type>
	void Set(std::shared_ptr<TInputBase<type>> ptr)
	{
		m_type = type;
		m_data = ptr;
	}

	bool HasData()
	{
		return m_data != nullptr;
	}

	template <typename T>
	std::shared_ptr<T> Get()
	{
		ASSERT(HasData());
		std::shared_ptr<T> ptr = std::static_pointer_cast<T>(m_data);

		ASSERT(m_type == ptr->GetType());

		return ptr;
	}
};

namespace RogueSaveManager
{
	void Serialize(EInputType& value);
	void Deserialize(EInputType& value);

	void Serialize(Input& value);
	void Deserialize(Input& value);
}