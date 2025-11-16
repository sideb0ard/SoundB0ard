#include <cmdloop.h>
#include <locale.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <synth_cmds.h>
#include <sys/select.h>
#include <utils.h>

#include <filereader.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <tsqueue.hpp>

namespace fs = std::filesystem;

extern std::unique_ptr<Mixer> global_mixr;
extern Tsqueue<std::string> eval_command_queue;
extern Tsqueue<std::string> repl_queue;

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"

char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
static bool active{true};
const std::string tick("tick");

int event_hook() {
  while (auto reply = repl_queue.try_pop()) {
    if (reply) {
      // TODO - this is a bit of a fudged way to signal a midi tick
      if (tick.compare(reply->data()) == 0) {
        for (auto &f : global_mixr->file_monitors) {
          if (!f.function_file_filepath.empty()) {
            //    // std::cout << "GOT A FILE TO MONITOR!\n";
            fs::path func_path = f.function_file_filepath;
            if (fs::exists(func_path)) {
              std::error_code ec;
              auto ftime = fs::last_write_time(func_path, ec);
              if (!ec) {
                if (ftime > f.function_file_filepath_last_write_time) {
                  std::string contents =
                      ReadFileContents(f.function_file_filepath);
                  eval_command_queue.push(contents);

                  std::cout << "UPdatin' " << f.function_file_filepath
                            << std::endl;

                  f.function_file_filepath_last_write_time = ftime;
                  rl_line_buffer[0] = '\0';
                  rl_done = 1;
                }
              } else {
                std::cerr << "Error opening file:" << ec << std::endl;
              }
            }
          }
        }
      } else {
        std::cout << reply->data();
        rl_line_buffer[0] = '\0';
        rl_done = 1;
      }
    }
  }
  return 0;
}

void *loopy() {
  std::cout << get_string_logo();
  read_history(NULL);
  setlocale(LC_ALL, "");

  std::string last_line;
  rl_event_hook = event_hook;
  rl_set_keyboard_input_timeout(500);

  while (true) {
    std::unique_ptr<char, void (*)(void *)> line(readline(prompt), free);

    if (!line) break;  // readline returned NULL

    if (line.get() && *line.get()) {
      std::string current_line(line.get());

      if (current_line != last_line) {
        add_history(line.get());
        last_line = current_line;
      }
      eval_command_queue.push(current_line);
    }
  }

  printf(COOL_COLOR_PINK
         "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET);  // Thrashin'
                                                         // reference
  write_history(nullptr);

  return nullptr;
}
