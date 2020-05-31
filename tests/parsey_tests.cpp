#include <audio_action_queue.h>
#include <event_queue.h>
#include <iostream>
#include <tsqueue.hpp>

#include "gtest/gtest.h"

auto global_env = std::make_shared<object::Environment>();
Tsqueue<audio_action_queue_item> audio_queue;
Tsqueue<std::string> interpret_command_queue;
Tsqueue<std::string> repl_queue;
Tsqueue<event_queue_item> process_event_queue;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "YARLY!\n";
    return RUN_ALL_TESTS();
}
