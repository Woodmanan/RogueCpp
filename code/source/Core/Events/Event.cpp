#include "Event.h"
#include "Core/Collections/StackArray.h"
#include "Debug/Profiling.h"
#include <algorithm>

void FireEvent(EventData& data)
{
	ROGUE_PROFILE;
	EEventType type = data.type;
	STACKARRAY(EventHandler*, handlers, 200);

	for (int i = 0; i < data.targets.size(); i++)
	{
		EventHandlerContainer* container = data.targets[i];
		ASSERT(container != nullptr);

		for (int j = 0; j < container->eventHandlers.size(); j++)
		{
			EventHandler* handler = container->eventHandlers[j];
			ASSERT(handler != nullptr);
			if (handler->GetPriority(type) >= 0)
			{
				handlers.push_back(handler);
			}
		}
	}

	std::sort(handlers.begin(), handlers.end(), [type](const EventHandler* lhs, const EventHandler* rhs)
	{
		return lhs->GetPriority(type) < rhs->GetPriority(type);
	});

	for (uint i = 0; i < handlers.size(); i++)
	{
		ASSERT(handlers[i] != nullptr);
		bool finish = handlers[i]->HandleEvent(type, data);
		if (finish) { break; }
	}
}