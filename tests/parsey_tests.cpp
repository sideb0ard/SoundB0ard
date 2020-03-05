#include <audio_action_queue.h>
#include <event_queue.h>
#include <iostream>
#include <tsqueue.hpp>

#include "gtest/gtest.h"

auto global_env = std::make_shared<object::Environment>();
Tsqueue<audio_action_queue_item> g_audio_action_queue;
Tsqueue<std::string> g_command_queue;
Tsqueue<std::string> g_reply_queue;
Tsqueue<event_queue_item> g_event_queue;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "YARLY!\n";
    return RUN_ALL_TESTS();
}
