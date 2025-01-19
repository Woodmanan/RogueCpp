#pragma once
#include "GameHeaders.h"
#include "Data/Serialization/BitStream.h"

/*
	IO controls! Defines the structs and serialization that we will need for communicating with our game threads and servers.
*/

#ifdef _DEBUG
typedef JSONStream StreamType;
#else
typedef PackedStream StreamType;
#endif

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
	virtual void Serialize(StreamType& stream) {}
	virtual void Deserialize(StreamType& stream) {}
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
	void Serialize(StreamType& stream);
	void Deserialize(StreamType& stream);

	Vec2 direction;
};

struct BeginSeededGameInput : public TInputBase<BeginSeededGame>
{
public:
	void Serialize(StreamType& stream);
	void Deserialize(StreamType& stream);

	int seed;
};

struct LoadSaveInput : public TInputBase<LoadSaveGame>
{
public:
	LoadSaveInput() { fileName = ""; }
	LoadSaveInput(const std::string& file) { fileName = file; }

	void Serialize(StreamType& stream);
	void Deserialize(StreamType& stream);

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

	template <typename T>
	void Set(std::shared_ptr<T> ptr)
	{
		m_type = ptr->GetType();
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

namespace Serialization
{
	template<typename Stream>
	void SerializeObject(Stream& stream, EInputType& value)
	{
		int asInt = value;
		Serialize(stream, asInt);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, EInputType& value)
	{
		int asInt;
		Deserialize(asInt);
		value = (EInputType)asInt;
	}

	template<typename Stream>
	void Serialize(Stream& stream, Input& value)
	{
		Write(stream, "Type", value.m_type);
		bool hasData = value.HasData();
		Write(stream "Has Data", hasData);
		if (hasData)
		{
			stream.WriteSpacing();
			value.Get<InputBase>()->Serialize(stream);
		}
	}

	template<typename Stream>
	void Deserialize(Stream& stream, Input& value)
	{
		Read(stream, "Type", value.m_type);
		bool hasData;
		Read(stream, "Has Data", hasData);
		if (hasData)
		{
			switch (value.m_type)
			{
			default:
				HALT(); //Bad type! Whatever value this is needs a corresponding case in the switch.
			case EInputType::Movement:
				value.m_data = std::make_shared<MoveInput>();
				break;
			case EInputType::BeginSeededGame:
				value.m_data = std::make_shared<BeginSeededGameInput>();
				break;
			case EInputType::LoadSaveGame:
				value.m_data = std::make_shared<LoadSaveInput>();
				break;
			}

			stream.ReadSpacing();
			value.Get<InputBase>()->Deserialize(stream);
		}
	}
}