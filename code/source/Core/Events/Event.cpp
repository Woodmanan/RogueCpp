#include "Event.h"
#include "Core/Collections/StackArray.h"
#include "Debug/Profiling.h"
#include <algorithm>

void FireEvent(EventData& data)
{
	ROGUE_PROFILE;
	EEventType type = data.type;
	int n = 0;
	STACKARRAY(EventHandler*, handlers, 100);

	for (int i = 0; i < data.targets.size(); i++)
	{
		EventHandlerContainer* container = data.targets[i];
		ASSERT(container != nullptr);

		for (int j = 0; j < container->eventHandlers.size(); j++)
		{
			EventHandler* handler = container->eventHandlers[j];
			if (handler->GetPriority(type) >= 0)
			{
				handlers[n] = handler;
				n++;
			}
		}
	}

	std::sort(handlers, handlers + n, [type](const EventHandler* lhs, const EventHandler* rhs)
	{
		return lhs->GetPriority(type) < rhs->GetPriority(type);
	});

	for (int i = 0; i < n; i++)
	{
		ASSERT(handlers[i] != nullptr);
		bool finish = handlers[i]->HandleEvent(type, data);
		if (finish) { break; }
	}
}