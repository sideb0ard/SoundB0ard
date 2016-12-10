#ifndef INTERPRETER_H_
#define INTERPRETER_H_

Stack *new_rpn_stack(char *apattern);
char interpreter(Stack *rpn_stack);

#endif //INTERPRETER_H_
