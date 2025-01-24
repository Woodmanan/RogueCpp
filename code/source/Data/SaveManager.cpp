#include "Data/SaveManager.h"

namespace RogueSaveManager {
#if defined(JSON) || defined(PACKED)
	thread_local std::ofstream SaveStreams::outStream;
	thread_local std::ifstream SaveStreams::inStream;
#endif
	thread_local SaveStreamType SaveStreams::stream;
}
