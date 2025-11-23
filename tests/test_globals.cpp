// Test globals - provides the global variables needed by sbsh_lib
// These are normally defined in main.cpp but tests need their own copies

#include <audio_action_queue.h>
#include <event_queue.h>

#include <memory>
#include <string>
#include <tsqueue.hpp>

#include "PerlinNoise.hpp"
#include "interpreter/object.hpp"

// Global queues for communication between threads
Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
Tsqueue<int> audio_reply_queue;
Tsqueue<std::string> eval_command_queue;
Tsqueue<std::string> repl_queue;
Tsqueue<event_queue_item> process_event_queue;

// Perlin noise generator
siv::PerlinNoise perlinGenerator;

// Global environment for the interpreter
auto global_env = std::make_shared<object::Environment>();
