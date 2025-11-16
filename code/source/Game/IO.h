#pragma once
#include "GameHeaders.h"
#include "Data/Serialization/BitStream.h"

/*
	IO controls! Defines the structs and serialization that we will need for communicating with our game threads and servers.
*/

enum EInputType
{
	InvalidInput,
	Movement,
	Wait,
	BeginNewGame,
	BeginSeededGame,
	LoadSaveGame,
	SaveAndExit,
	ExitGame,

	DEBUG_FIRE,
	DEBUG_MAKE_STONE,
	DEBUG_LOAD_RESOURCES
};

class InputBase
{
public:
	virtual EInputType GetType() const { return EInputType::InvalidInput; }
	virtual void Serialize(DefaultStream& stream) {}
	virtual void Deserialize(DefaultStream& stream) {}
};

template<EInputType inputType>
class TInputBase : public InputBase
{
public:
	EInputType GetType() const override { return inputType; }
	const static EInputType type = inputType;
};

template <EInputType inputType>
class TInput : public TInputBase<inputType> {};

template<>
class TInput<Movement> : public TInputBase<Movement>
{
public:
	TInput() { m_direction = (Direction) -1; }
	TInput(Direction direction) { m_direction = direction; }


	void Serialize(DefaultStream& stream);
	void Deserialize(DefaultStream& stream);

	Direction m_direction;
};

template<>
class TInput<BeginSeededGame> : public TInputBase<BeginSeededGame>
{
public:
	TInput() { seed = 0; }
	TInput(uint seedValue) { seed = seedValue; }

	void Serialize(DefaultStream& stream);
	void Deserialize(DefaultStream& stream);

	uint seed;
};

template<>
class TInput<LoadSaveGame> : public TInputBase<LoadSaveGame>
{
public:
	TInput() { fileName = ""; }
	TInput(const std::string& file) { fileName = file; }

	void Serialize(DefaultStream& stream);
	void Deserialize(DefaultStream& stream);

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

	bool HasData() const
	{
		return m_data != nullptr;
	}

	template <EInputType type>
	std::shared_ptr<TInput<type>> Get() const
	{
		ASSERT(HasData());
		ASSERT(type == m_type);

		std::shared_ptr<TInput<type>> ptr = std::static_pointer_cast<TInput<type>>(m_data);
		return ptr;
	}
};

enum EOutputType
{
	InvalidOutput,
	GameReady,
	ViewUpdated
};

class OutputBase
{
public:
	virtual EOutputType GetType() const { return EOutputType::InvalidOutput; }
	virtual void Serialize(DefaultStream& stream) {}
	virtual void Deserialize(DefaultStream& stream) {}
};

template<EOutputType outputType>
class TOutputBase : public OutputBase
{
public:
	EOutputType GetType() const override { return outputType; }
	const static EOutputType type = outputType;
};

template <EOutputType outputType>
class TOutput : public TOutputBase<outputType> {};

template <>
class TOutput<ViewUpdated> : public TOutputBase<ViewUpdated>
{
public:
	TOutput() {}
	TOutput(PackedStream stream)
	{
		m_data.clear();
		std::shared_ptr<VectorBackend> backend = dynamic_pointer_cast<VectorBackend>(stream.GetDataBackend());
		ASSERT(backend != nullptr);
		m_data.insert(m_data.end(), backend->m_data.begin(), backend->m_data.end());
	}

	void Serialize(DefaultStream& stream);
	void Deserialize(DefaultStream& stream);

	std::vector<char> m_data;
};

struct Output
{
public:
	EOutputType m_type;
	std::shared_ptr<OutputBase> m_data;

	void Set(EOutputType type, std::shared_ptr<OutputBase> data = nullptr)
	{
		m_type = type;
		m_data = data;
	}

	template <EOutputType type>
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

	bool HasData() const
	{
		return m_data != nullptr;
	}

	template <EOutputType type>
	std::shared_ptr<TOutput<type>> Get() const
	{
		ASSERT(HasData());
		ASSERT(type == m_type);

		std::shared_ptr<TOutput<type>> ptr = std::static_pointer_cast<TOutput<type>>(m_data);
		return ptr;
	}
};

namespace Serialization
{
	template<typename Stream>
	void SerializeObject(Stream& stream, const EInputType& value)
	{
		stream.WriteEnum(value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, EInputType& value)
	{
		stream.ReadEnum(value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const EOutputType& value)
	{
		int asInt = value;
		Serialize(stream, asInt);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, EOutputType& value)
	{
		int asInt;
		Deserialize(asInt);
		value = (EOutputType)asInt;
	}

	template<typename Stream>
	void Serialize(Stream& stream, const Input& value)
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

	template<typename Stream>
	void Serialize(Stream& stream, const Output& value)
	{
		Write(stream, "Type", value.m_type);
		bool hasData = value.HasData();
		Write(stream "Has Data", hasData);
		if (hasData)
		{
			stream.WriteSpacing();
			value.Get<OutputBase>()->Serialize(stream);
		}
	}

	template<typename Stream>
	void Deserialize(Stream& stream, Output& value)
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
			}

			stream.ReadSpacing();
			value.Get<OutputBase>()->Deserialize(stream);
		}
	}
}