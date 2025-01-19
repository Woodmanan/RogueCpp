#include "Data/SaveManager.h"

namespace RogueSaveManager {
	thread_local std::ofstream SaveStreams::outStream;
	thread_local std::ifstream SaveStreams::inStream;
	thread_local SaveStreamType SaveStreams::stream;
}
