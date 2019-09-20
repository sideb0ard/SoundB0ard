#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <iostream>

#include <algo_cmds.h>
#include <cmdloop.h>
#include <fx_cmds.h>
#include <interpreter/evaluator.hpp>
#include <interpreter/lexer.hpp>
#include <interpreter/object.hpp>
#include <interpreter/parser.hpp>
#include <interpreter/token.hpp>
#include <looper_cmds.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <pattern_generator_cmds.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <utils.h>
#include <value_generator_cmds.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"
#define MAXLINE 128

char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
char const *OSC_LISTEN_PORT = "7771";
static char last_line[MAXLINE] = {};

auto env = std::make_shared<object::Environment>();
auto lex = std::make_shared<lexer::Lexer>();

static void liblo_error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

void readline_cb(char *line)
{
    if (NULL == line)
        exxit();
    if (strlen(line) != 0)
    {
        if (strncmp(last_line, line, MAXLINE) != 0)
        {
            add_history(line);
            strncpy(last_line, line, MAXLINE);
        }

        lex->ReadInput(line);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(lex);

        std::shared_ptr<ast::Program> program = parsley->ParseProgram();

        auto evaluated = evaluator::Eval(program, env);
        if (evaluated)
        {
            auto result = evaluated->Inspect();
            if (result.compare("null") != 0)
                std::cout << result << std::endl;
        }

        lex->Reset();
        // interpret(line);
    }
}

void *loopy(void *arg)
{

    print_logo();

    read_history(NULL);
    rl_callback_handler_install(prompt, (rl_vcpfunc_t *)&readline_cb);

    setlocale(LC_ALL, "");

    lo_server s = lo_server_new(OSC_LISTEN_PORT, liblo_error);
    // lo_server_add_method(s, NULL, NULL, generic_osc_handler, NULL);
    lo_server_add_method(s, "/trigger", "i", trigger_osc_handler, NULL);
    lo_server_add_method(s, "/note_on", "iiii", osc_note_on_handler, NULL);
    int lo_fd = lo_server_get_socket_fd(s);

    fd_set rfds;
    struct timeval tv;

    int retval;

    if (lo_fd > 0)
    {

        // printf("%s", prompt);
        // fflush(stdout);
        while (1)
        {

            FD_ZERO(&rfds);
            FD_SET(0, &rfds); /* stdin */
            FD_SET(lo_fd, &rfds);

            retval =
                select(lo_fd + 1, &rfds, NULL, NULL, NULL); /* no timeout */

            if (retval == -1)
            {

                printf("select() error\n");
                exit(1);
            }
            else if (retval > 0)
            {

                if (FD_ISSET(0, &rfds))
                {

                    rl_callback_read_char();
                }
                if (FD_ISSET(lo_fd, &rfds))
                {

                    lo_server_recv_noblock(s, 0);
                }
            }
        }
    }
    else
        printf("Error, liblo -- fd <= zero\n");

    return NULL;
}

static bool _is_meta_cmd(char *line)
{
    if (strncmp("every", line, 5) == 0 || strncmp("over", line, 4) == 0 ||
        strncmp("for", line, 3) == 0)
        return true;

    return false;
}

void interpret(char *line)
{
    char wurds[NUM_WURDS][SIZE_OF_WURD] = {};

    if (_is_meta_cmd(line))
    {
        int num_wurds = parse_wurds_from_cmd(wurds, line);
        algorithm *a = new_algorithm(num_wurds, wurds);
        if (a)
            mixer_add_algorithm(mixr, a);
    }

    char *cmd, *last_s;
    char const *sep = ";";
    char tmp[1024] = {};
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s))
    {
        strncpy((char *)tmp, cmd, 127);
        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        //////////////////////////////////////////////////////////////////////

        if (strncmp("help", wurds[0], 4) == 0)
            oblique_strategy();

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0)
            exxit();

        else if (strncmp("print", wurds[0], 5) == 0)
            printf("%s\n", wurds[1]);

        else if (parse_mixer_cmd(num_wurds, wurds))
            continue;

        else if (parse_algo_cmd(num_wurds, wurds))
            continue;

        else if (parse_fx_cmd(num_wurds, wurds))
            continue;

        else if (parse_looper_cmd(num_wurds, wurds))
            continue;

        else if (parse_midi_cmd(num_wurds, wurds))
            continue;

        else if (parse_new_item_cmd(num_wurds, wurds))
            continue;

        else if (parse_pattern_generator_cmd(num_wurds, wurds))
            continue;

        else if (parse_value_generator_cmd(num_wurds, wurds))
            continue;

        else if (parse_synth_cmd(num_wurds, wurds))
            continue;

        else if (parse_stepper_cmd(num_wurds, wurds))
            continue;
    }
}

int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line)
{
    memset(wurds, 0, NUM_WURDS * SIZE_OF_WURD);
    int num_wurds = 0;
    char const *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s))
    {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

int exxit()
{
    printf(COOL_COLOR_PINK
           "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET); // Thrashin' reference
    write_history(NULL);
    pa_teardown();
    exit(0);
}

int generic_osc_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i = 0; i < argc; i++)
    {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}

int trigger_osc_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    // printf("Target %d\n", argv[0]->i);
    int target_sg = argv[0]->i;
    fflush(stdout);
    midi_event _event = new_midi_event(MIDI_ON, 32, 128);
    _event.source = EXTERNAL_OSC;
    if (mixer_is_valid_soundgen_num(mixr, target_sg))
    {
        SoundGenerator *sg = mixr->SoundGenerators[target_sg];
        sg->parseMidiEvent(_event, mixr->timing_info);
    }

    return 0;
}

int osc_note_on_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    int target_sg = argv[0]->i;
    int midi_note = argv[1]->i;
    int octave = argv[2]->i;
    int velocity = argv[3]->i;

    midi_event _event =
        new_midi_event(MIDI_ON, (octave * 12) + midi_note, velocity);
    _event.source = EXTERNAL_OSC;
    if (mixer_is_valid_soundgen_num(mixr, target_sg))
    {
        SoundGenerator *sg = mixr->SoundGenerators[target_sg];
        sg->parseMidiEvent(_event, mixr->timing_info);
    }
    return 0;
}
