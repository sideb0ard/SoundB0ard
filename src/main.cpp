#include <audio_action_queue.h>
#include <audioutils.h>
#include <cmdloop.h>
#include <defjams.h>
#include <event_queue.h>
#include <midimaaan.h>
#include <mixer.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <readline/history.h>

#include <AudioPlatform.hpp>
#include <PerlinNoise.hpp>
// #include <asio/io_context.hpp>  // Temporarily disabled for WebSocket
// #include <asio/executor_work_guard.hpp>  // Temporarily disabled for WebSocket
#include <interpreter/evaluator.hpp>
#include <interpreter/lexer.hpp>
#include <interpreter/object.hpp>
#include <interpreter/parser.hpp>
#include <interpreter/token.hpp>
#include <iomanip>
#include <iostream>
#include <process.hpp>
#include <thread>
#include <tsqueue.hpp>
#include <memory>

// #include "websocket/web_socket_server.h"  // Temporarily disabled

extern std::unique_ptr<Mixer> global_mixr;

Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
Tsqueue<int> audio_reply_queue;  // for reply from adding SoundGenerator
Tsqueue<std::string> eval_command_queue;
Tsqueue<std::string> repl_queue;
Tsqueue<event_queue_item> process_event_queue;

siv::PerlinNoise perlinGenerator;  // only for use by eval thread

auto global_env = std::make_shared<object::Environment>();

namespace {

constexpr int kPortNumber = 8080;

struct State {
  std::atomic<bool> running;
  ableton::Link link;
  ableton::linkaudio::AudioPlatform audioPlatform;

  State(Mixer &mixer) : running(true), link(140.), audioPlatform(link, mixer) {
    link.enable(true);
  }
};

}  // namespace

void Eval(char *line, std::shared_ptr<object::Environment> env) {
  auto lex = std::make_shared<lexer::Lexer>();
  lex->ReadInput(line);
  auto parsley = std::make_unique<parser::Parser>(lex);

  std::shared_ptr<ast::Program> program = parsley->ParseProgram();

  auto evaluated = evaluator::Eval(program, env);
  if (evaluated) {
    auto result = evaluated->Inspect();
    if (result.compare("null") != 0) {
      std::cout << result << std::endl;
    }
  }
}

void *eval_queue() {
  while (auto cmd = eval_command_queue.pop()) {
    if (cmd && cmd->data()) {
      Eval(cmd->data(), global_env);
    }
  }
  return nullptr;
}

void *process_worker_thread() {
  std::cout << "PROCESS WURKER THRED STARTED\n";
  while (auto const event = process_event_queue.pop()) {
    if (event) {
      if (event->type == Event::TIMING_EVENT) {
        auto timing_info = event->timing_info;

        if (global_mixr->proc_initialized_) {
          for (auto p : global_mixr->processes_) {
            if (p->active_) p->EventNotify(timing_info);
          }
        }
      } else if (event->type == Event::PROCESS_UPDATE_EVENT) {
        if (event->target_process_id >= 0 &&
            event->target_process_id < MAX_NUM_PROC) {
          ProcessConfig config = {

              .name = event->process_name,
              .process_type = event->process_type,
              .timer_type = event->timer_type,
              .loop_len = event->loop_len,
              .command = event->command,
              .target_type = event->target_type,
              .targets = event->targets,
              .pattern_expression = event->pattern_expression,
              .pattern = "",
              .funcz = event->funcz,
              .tinfo = event->timing_info};

          global_mixr->processes_[event->target_process_id]->EnqueueUpdate(config);
        } else {
          std::cerr << "WAH! INVALID process id: " << event->target_process_id
                    << std::endl;
        }
      } else if (event->type == Event::PROCESS_SET_PARAM_EVENT) {
        if (event->target_process_id >= 0 &&
            event->target_process_id < MAX_NUM_PROC) {
          global_mixr->processes_[event->target_process_id]->UpdateLoopLen(
              event->loop_len);
        }
      }
    }
  }
  return nullptr;
}

/*
void *websocket_worker(WebsocketServer &server) {
  std::cout << "AM A WEE WEBSOCKET THREAD!\n";
  auto eventLoop = link_asio_1_30_2::io_context{};

  // TODO: Fix WebSocket++ template issues with Link's ASIO
  // Temporarily disable WebSocket callbacks to test basic compilation
  // server.connect callback removed
  // server.disconnect callback removed
  
  std::thread websocket_server_thread([&server]() { server.run(kPortNumber); });

  // Keep the event loop alive
  // auto work = asio::executor_work_guard<asio::io_context::executor_type>(eventLoop.get_executor());
  eventLoop.run();

  return nullptr;
}
*/

int main() {
  srand(time(NULL));
  signal(SIGINT, SIG_IGN);

  // WebsocketServer server;  // Temporarily disabled
  global_mixr = std::make_unique<Mixer>();  // Create as unique_ptr

  State state(*global_mixr);

  // //// REPL
  std::thread repl_thread(loopy);

  //// Processes
  std::thread worker_thread(process_worker_thread);

  // //// WebSocket Server - Temporarily disabled
  // // std::thread websocket_worker_thread(websocket_worker, std::ref(server));  // Disabled

  //// Eval loop
  std::thread eval_thread(eval_queue);

  ////////////////
  /// shutdown

  repl_thread.join();

  // Signal other threads to shut down
  process_event_queue.close();
  worker_thread.join();

  eval_command_queue.close();
  eval_thread.join();

}
