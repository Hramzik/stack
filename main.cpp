

#include "func.hpp"


int main ()
{
    Stack stack;


    STACK_CTOR (&stack);

    stack_push (&stack, 7);
    stack_push (&stack, 13);
    stack_push (&stack, 44);
    stack_push (&stack, 0);
    stack_push (&stack, 2.55);
    stack_push (&stack, 2.55);
    stack_push (&stack, 2.55);

    Return_code return_code = SUCCESS; // why?????
    for (size_t i = 0; i < 8; i++) {

        Element popped = stack_pop (&stack, &return_code);
        printf ("popped %4lg, poisoned - %d, return code - %d\n", popped.value, popped.poisoned, return_code);
    }


    FSTACK_DUMP (&stack);


    STACK_DTOR (&stack);


    return 0;
}

