

#include "func.hpp"


Return_code  _stack_ctor  (Stack* stack, const char* name, const char* file, const char* func, int line) {

    assert ( (file != nullptr) && (func != nullptr) && (line > 0) );
    if (stack == nullptr || name == nullptr) { log_error (BAD_ARGS); return BAD_ARGS; }


    IF_CANARY_PROTECTED (stack->FIRST_CANARY = FIRST_CANARY_VALUE);

    stack->elements = nullptr;
    stack->size     = 0;
    stack->capacity = 0;

    stack->debug_info.name = name;
    stack->debug_info.birth_file = file;
    stack->debug_info.birth_func = func;
    stack->debug_info.birth_line = line;
    stack->debug_info.adress     = stack;

    IF_CANARY_PROTECTED (stack->SECOND_CANARY = SECOND_CANARY_VALUE);


    return SUCCESS;
}

Return_code  stack_dtor  (Stack* stack) {

    assert_stack_ok (stack);


    if (stack->elements != nullptr) { free (stack->elements); }


    return SUCCESS;
}

Return_code  stack_resize  (Stack* stack, size_t new_capacity) {

    assert_stack_ok (stack);


    stack->elements = (Element*) realloc (stack->elements, sizeof (Element) * new_capacity); // + 2 * sizeof canary;
    if (stack->elements == nullptr) { log_error (MEMORY_ERR); return MEMORY_ERR; }


    for (size_t i = stack->size; i < new_capacity; i++) { stack->elements [i] = Element {NAN, true}; } // poisoned fillers

    stack->capacity = new_capacity;
    //printf ("%zd %zd\n", stack->capacity, new_capacity);
    //stack_dump (stack);

    if (stack->size > new_capacity) { stack->size = new_capacity; }


    return SUCCESS;
}

Return_code  stack_push  (Stack* stack, Element_value new_element_value) {

    assert_stack_ok (stack);


    if (stack->size == stack->capacity) {

        Return_code resize_code = SUCCESS; // А СТОИТ ЛИ ОНО ТОГО????? ЗАЧЕМММММММММММММММММММММ

        if (!stack->capacity) {

            resize_code = stack_resize (stack, 1);
        }
        else {

            resize_code = stack_resize (stack, (size_t) ceil ((double) stack->capacity * stack_resize_coefficient) );
        }

        if (resize_code) { log_error (resize_code); return resize_code; }
    }


    stack->size += 1;
    stack->elements [stack->size - 1] = Element {new_element_value, false};


    return SUCCESS;
}

Element  stack_pop  (Stack* stack, Return_code* return_code_ptr) {

    assert_stack_ok_for_stack_pop (stack);


    if (stack->size == 0) {

        if (return_code_ptr) { *return_code_ptr = SUCCESS; } // OR NOT SUCCESS? SHOULD I RETURN SMTH LIKE CALL_ERR??
        return Element {NAN, true};
    }


    stack->size -= 1;
    Element return_element = stack->elements [stack->size];
    stack->elements [stack->size] = Element {NAN, true};


    if ( (double) stack->size * pow (stack_resize_coefficient, 2) <= (double) stack->capacity) {

        Return_code resize_code = stack_resize (stack, (size_t) ceil ( (double) stack->capacity / stack_resize_coefficient) );
        if (resize_code) {

            log_error (resize_code);
            if (return_code_ptr) { *return_code_ptr = BAD_ARGS; }
            return Element {NAN, true};
        }
    }


    if (return_code_ptr) { *return_code_ptr = SUCCESS; }
    return return_element;
}

Stack_state  stack_damaged  (Stack* stack) {

    Stack_state stack_state = 0;

    if (stack == nullptr) { stack_state |= 1; return stack_state; }


    if (stack->size > stack->capacity)                  { stack_state |= (1<<1); }
    if (stack->elements == nullptr && stack->size != 0) { stack_state |= (1<<2); }

    for (size_t i = 0; i < stack->capacity; i++) {

        if (i <  stack->size) if ( stack->elements [i].poisoned) { stack_state |= (1<<3); break; }
        if (i >= stack->size) if (!stack->elements [i].poisoned) { stack_state |= (1<<3); break; }
    }

    return stack_state;
}

void  _fstack_dump  (Stack* stack, const char* file_name, const char* file, const char* func, int line) {

    assert (file != nullptr and func != nullptr);


    FILE* dump_file;
    if (file_name == nullptr) {

        dump_file = stdout;
    }

    else {

        dump_file = fopen (file_name, "a");
        if (dump_file == nullptr) { log_error (FILE_ERR); return; }
    }


    setvbuf (dump_file, NULL, _IONBF, 0);


    fprintf (dump_file, "--------------------\n");
    fprintf (dump_file, "Dumping stack at %s in function %s (line %d)...\n", file, func, line);


    if (!stack) { fprintf (dump_file, "Stack pointer is nullptr!\n\n"); return; }


    fprintf (dump_file, "this stack has name ");
    if (stack->debug_info.name != nullptr) { fprintf (dump_file, "%s ", stack->debug_info.name); }
    else                                   { fprintf (dump_file, "UNKNOWN NAME "); }
    fprintf (dump_file, "[%p]\n", stack->debug_info.adress);

    fprintf (dump_file, "it was created in file ");
    if (stack->debug_info.birth_file != nullptr) { fprintf (dump_file, "%s\n", stack->debug_info.birth_file); }
    else                                         { fprintf (dump_file, "UNKNOWN NAME\n"); }

    fprintf (dump_file, "in function ");
    if (stack->debug_info.birth_func != nullptr) { fprintf (dump_file, "%s ", stack->debug_info.birth_func); }
    else                                         { fprintf (dump_file, "UNKNOWN NAME "); }

    fprintf (dump_file, "(line %d)\n\n", stack->debug_info.birth_line);


    fprintf (dump_file, "stack is ");
    Stack_state stack_state = stack_damaged (stack);
    if (stack_state) { fprintf (dump_file, "damaged (damage code %u)\n", stack_state); }
    else             { fprintf (dump_file, "ok\n"); }


    fprintf (dump_file, "size %zd\n",      stack->size);
    fprintf (dump_file, "capacity %zd\n\n",  stack->capacity);
    if (stack->elements) { fprintf (dump_file, "elements [%p]:\n", stack->elements); }
    else                 { fprintf (dump_file, "elements [nullptr]:\n"); }


    for (size_t i = 0; i < stack->capacity; i++) {

        if (i < stack->size) { fprintf (dump_file, "(in)  "); }
        else                 { fprintf (dump_file, "(out) "); }

        fprintf (dump_file, "[%zd] = %lg (", i, stack->elements[i].value);         // different fprintf function
        if (stack->elements[i].poisoned) { fprintf (dump_file,     "poisoned)\n"); }
        else                             { fprintf (dump_file, "not poisoned)\n"); }
    }
    fprintf (dump_file, "\n");


    IF_CANARY_PROTECTED (
        fprintf (dump_file, "first  canary - %llX (", stack->FIRST_CANARY);

        if (stack->FIRST_CANARY == FIRST_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                           { fprintf (dump_file, "corrupted)\n"); }

        fprintf (dump_file, "second canary - %llX (", stack->SECOND_CANARY);

        if (stack->SECOND_CANARY == SECOND_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                             { fprintf (dump_file, "corrupted)\n"); }
    )

    fprintf (dump_file, "\n");


    fclose (dump_file);
}








































void  log_message  (const char* message) {

    FILE* log_file = fopen (log_file_name, "a");
    setvbuf                (log_file, NULL, _IONBF, 0);


    print_log_time();
    fprintf (log_file, "%s\n", message);


    fclose (log_file);
}

char*  tm_to_str  (struct tm* time_structure) {

    assert (time_structure != nullptr);


    static char time_str[time_str_len] = "";
    for (size_t i = 0; i < time_str_len; i++) time_str[i] = '\0';


    strftime (  time_str, time_str_len, "%d.%m.%Y %H:%M:%S: ", time_structure);


    return time_str;
}

void  print_log_time  (void) {

    FILE* log_file = fopen (log_file_name, "a");
    setvbuf                (log_file, NULL, _IONBF, 0);

    struct tm*   time_structure = nullptr;
    char*        time_str       = nullptr;
    const time_t cur_time       = time (nullptr);

    time_structure = localtime   (&cur_time);
    time_str       = tm_to_str (time_structure);
    fprintf (log_file, "%s", time_str);


    fclose (log_file);
}

void  log_start  (void) {

    FILE* log_file = fopen (log_file_name, "a");
    setvbuf                (log_file, NULL, _IONBF, 0);


    char log_starter[] = "-------STARTING THE PROGRAM...-------\n\n";

    fprintf (log_file, "%s", log_starter);


    fclose (log_file);


    atexit (log_end);
}

void  log_end  (void) {

    static bool logs_ended_flag = true;

    FILE*  log_file = fopen (log_file_name, "a");
    setvbuf                 (log_file, NULL, _IONBF, 0);


    if (logs_ended_flag) return;

    char log_ender[] = "-------ENDING   THE PROGRAM...-------\n\n";

    fprintf (log_file, "%s", log_ender); logs_ended_flag = true;


    fclose (log_file);
}

void  _log_error  (Return_code _code, const char* file, const char* func, const int line) {

    Return_code code     = _code;
    FILE*       log_file = fopen (log_file_name, "a");
    setvbuf                      (log_file, NULL, _IONBF, 0);

    switch (code) {

    case SUCCESS:
      /*fprintf (log_file, "everything ok!\n");*/                                                                                       break;

    case MEMORY_ERR:
        print_log_time();
        fprintf (log_file, "memory error in file %s in function %s (line %d)\n", file, func, line);                                     break;

    case BAD_ARGS:
        print_log_time();
        fprintf (log_file, "wrong parameters given to the function %s in file %s (line %d)\n", func, file, line);                       break;

    case FILE_ERR:
        print_log_time();
        fprintf (log_file, "file opening error in file %s in function %s (line %d)\n", file, func, line);                               break;

    default:
        print_log_time();
        fprintf (log_file, "wrong error code given to the log_error function in file %s in function %s (line %d)\n", file, func, line); break;
    }


    fclose (log_file);
}