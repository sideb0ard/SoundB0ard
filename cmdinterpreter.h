void cmd_loopy(void);
char *cmd_read_line(void);
char **cmd_split_line(char *line);

// function declarations for shell builtins
int cmd_execute(char **args);
int cmd_ps(char **args);
int cmd_gen(char **args);
int cmd_osc(char **args);
int cmd_help(char **args);
int cmd_exit(char **args);
int cmd_newmixer(char **args);
