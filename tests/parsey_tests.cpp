#include <audio_action_queue.h>
#include <event_queue.h>
#include <iostream>
#include <tsqueue.hpp>

#include "PerlinNoise.hpp"

#include "gtest/gtest.h"

auto global_env = std::make_shared<object::Environment>();
Tsqueue<audio_action_queue_item> audio_queue;
Tsqueue<std::string> eval_command_queue;
Tsqueue<std::string> repl_queue;
Tsqueue<event_queue_item> process_event_queue;
Tsqueue<int> audio_reply_queue;

siv::PerlinNoise perlinGenerator;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "YARLY!\n";
    return RUN_ALL_TESTS();
}
