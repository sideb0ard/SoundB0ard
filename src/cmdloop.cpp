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
#include <tsqueue.hpp>

namespace fs = std::filesystem;

extern Mixer *mixr;
extern Tsqueue<std::string> eval_command_queue;
extern Tsqueue<std::string> repl_queue;

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"
#define MAXLINE 128

char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
static char last_line[MAXLINE] = {};
static bool active{true};
const std::string tick("tick");

int event_hook() {
  while (auto reply = repl_queue.try_pop()) {
    if (reply) {
      // TODO - this is a bit of a fudged way to signal a midi tick
      if (tick.compare(reply->data()) == 0) {
        for (auto &f : mixr->file_monitors) {
          if (!f.function_file_filepath.empty()) {
            //    // std::cout << "GOT A FILE TO MONITOR!\n";
            fs::path func_path = f.function_file_filepath;
            if (fs::exists(func_path)) {
              std::error_code ec;
              auto ftime = fs::last_write_time(func_path, ec);
              if (!ec) {
                std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
                if (cftime > f.function_file_filepath_last_write_time) {
                  std::string contents =
                      ReadFileContents(f.function_file_filepath);
                  eval_command_queue.push(contents);

                  std::cout << "UPdatin' " << f.function_file_filepath
                            << std::endl;

                  f.function_file_filepath_last_write_time = cftime;
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

  char *line;
  rl_event_hook = event_hook;
  rl_set_keyboard_input_timeout(500);
  while ((line = readline(prompt)) != NULL && active) {
    if (line && *line) {
      if (strncmp(last_line, line, MAXLINE) != 0) {
        add_history(line);
        strncpy(last_line, line, MAXLINE);
      }
      eval_command_queue.push(line);
    }
    free(line);
  }
  exxit();

  return NULL;
}

int exxit() {
  printf(COOL_COLOR_PINK
         "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET);  // Thrashin'
                                                         // reference
  write_history(NULL);

  pa_teardown();

  active = false;

  //      return 0;
  exit(0);
}
