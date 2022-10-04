

#include "func.hpp"


Return_code  _stack_ctor  (Stack* stack, const char* name, const char* file, const char* func, int line) {

    assert ( (file != nullptr) && (func != nullptr) && (line > 0) );
    if (stack == nullptr || name == nullptr) { LOG_ERROR (BAD_ARGS); STACK_ERROR_DUMP (stack); return BAD_ARGS; }


    stack->elements = nullptr;
    stack->size     = 0;
    stack->capacity = 0;


    stack->debug_info.name = name;
    stack->debug_info.birth_file = file;
    stack->debug_info.birth_func = func;
    stack->debug_info.birth_line = line;
    stack->debug_info.adress     = stack;


    IF_CANARY_PROTECTED (

        stack-> FIRST_CANARY =  FIRST_CANARY_VALUE;
        stack->SECOND_CANARY = SECOND_CANARY_VALUE;

        stack->elements = (Element*) calloc (2 * CANARY_SIZE, 1);
        if (stack->elements == nullptr) { LOG_ERROR (MEMORY_ERR); STACK_ERROR_DUMP (stack); return MEMORY_ERR; }

        ( (canary_t*) stack->elements) [0] =  FIRST_CANARY_VALUE;
        ( (canary_t*) stack->elements) [1] = SECOND_CANARY_VALUE;

        stack->elements = (Element*) ( (char*) stack->elements + CANARY_SIZE );
    );


    IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


    STACK_AFTER_OPERATION_DUMPING (stack);


    return SUCCESS;
}


Return_code  _stack_dtor  (Stack* stack) {

    ASSERT_STACK_OK (stack);


    if (stack->elements != nullptr) {

        free (stack->elements);
        stack->elements = nullptr;
    }


    STACK_AFTER_OPERATION_DUMPING (stack);


    return SUCCESS;
}

IF_CANARY_PROTECTED (

    Return_code  _stack_canary_dtor  (Stack* stack) {

        ASSERT_STACK_OK (stack);


        if (stack->elements != nullptr) {

            free ( (char*) stack->elements - CANARY_SIZE);
            stack->elements = nullptr;
        }


        STACK_AFTER_OPERATION_DUMPING (stack);


        return SUCCESS;
    }
)

Return_code  _stack_resize  (Stack* stack, size_t new_capacity) {

    ASSERT_STACK_OK (stack);


    stack->elements = (Element*) realloc (stack->elements, sizeof (Element) * new_capacity);
    if (stack->elements == nullptr && new_capacity != 0) { LOG_ERROR (MEMORY_ERR); STACK_ERROR_DUMP (stack); return MEMORY_ERR; }


    for (size_t i = stack->size; i < new_capacity; i++) { stack->elements [i] = Element {NAN, true}; }

    stack->capacity = new_capacity;
    //printf ("%zd %zd\n", stack->capacity, new_capacity);
    //STACK_DUMP (stack);

    if (stack->size > new_capacity) { stack->size = new_capacity; }


    IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


    STACK_AFTER_OPERATION_DUMPING (stack);


    return SUCCESS;
}


IF_CANARY_PROTECTED (

    Return_code  _stack_canary_resize  (Stack* stack, size_t new_capacity) {

        ASSERT_STACK_OK (stack); //putc('a', stderr);

        size_t new_size = new_capacity * sizeof (Element) + 2 * CANARY_SIZE;


        if (new_capacity == stack->capacity) { STACK_AFTER_OPERATION_DUMPING (stack); return SUCCESS; }

        if (new_capacity > stack->capacity) {

            stack->elements = (Element*) ( (char*) stack->elements - CANARY_SIZE);


            stack->elements = (Element*) realloc (stack->elements, new_size);
            if (stack->elements == nullptr && new_capacity != 0) { LOG_ERROR (MEMORY_ERR); STACK_ERROR_DUMP (stack); return MEMORY_ERR; }


            stack->elements = (Element*) ( (char*) stack->elements + CANARY_SIZE ) + stack->capacity;
            canary_t second_canary_buffer = *( (canary_t*) stack->elements ); //fprintf (stderr, "%llX", second_canary_buffer);


            for (size_t i = 0; i < new_capacity - stack->capacity; i++) {

                stack->elements[i] = Element {NAN COMMA true};
            }


            *( (canary_t*) (stack->elements + new_capacity - stack->capacity) ) = second_canary_buffer;
            //printf ("%llX", * ( (canary_t*) (stack->elements + new_capacity - stack->capacity) ) );


            stack->elements -= stack->capacity;
            stack->capacity  = new_capacity;


        IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


        STACK_AFTER_OPERATION_DUMPING(stack);


        return SUCCESS;
        }

        stack->elements = (Element*) ( (char*) stack->elements - CANARY_SIZE );


        canary_t second_canary_buffer = *(canary_t*) ( (char*) (stack->elements + stack->capacity) + CANARY_SIZE );


        stack->elements = (Element*) realloc (stack->elements, new_size);
        if (stack->elements == nullptr) { LOG_ERROR (MEMORY_ERR); STACK_ERROR_DUMP (stack); return MEMORY_ERR; }


        *(canary_t*) ( (char*) (stack->elements + new_capacity) + CANARY_SIZE ) = second_canary_buffer;


        stack->elements = (Element*) ( (char*) stack->elements + CANARY_SIZE );
        stack->capacity = new_capacity;


        IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


        STACK_AFTER_OPERATION_DUMPING(stack);


        return SUCCESS;
    }
);


Return_code  stack_push  (Stack* stack, Element_value new_element_value) {

    ASSERT_STACK_OK (stack);


    if (stack->size == stack->capacity) {

        Return_code resize_code = SUCCESS; // А СТОИТ ЛИ ОНО ТОГО????? ЗАЧЕМММММММММММММММММММММ

        if (!stack->capacity) {

            resize_code = stack_resize (stack, 1);
        }
        else {

            resize_code = stack_resize ( stack, (size_t) fmax ( ceil ((double) stack->capacity * stack_resize_coefficient) , stack->capacity + 1) );
        }

        if (resize_code) { LOG_ERROR (resize_code);  STACK_ERROR_DUMP (stack); return resize_code; }
    }
   // STACK_DUMP (stack);

    stack->size += 1;
    stack->elements [stack->size - 1] = Element {new_element_value, false};


    IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


    STACK_AFTER_OPERATION_DUMPING (stack);


    return SUCCESS;
}


Element  stack_pop  (Stack* stack, Return_code* return_code_ptr) {

    ASSERT_STACK_OK_FOR_STACK_POP (stack);


    Element return_element = {NAN, true};

    if (stack->size != 0) {

        stack->size -= 1;
        return_element = stack->elements [stack->size];
        stack->elements [stack->size] = Element {NAN, true};

        IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );
    }


    if ( (double) stack->size * pow (stack_resize_coefficient, 2) <= (double) stack->capacity) {

        Return_code resize_code = stack_resize (stack, (size_t) fmin ( ceil ( (double) stack->capacity / stack_resize_coefficient), stack->capacity - 1) );

        if (resize_code) {

            LOG_ERROR (resize_code);
            if (return_code_ptr) { *return_code_ptr = BAD_ARGS; }
            STACK_ERROR_DUMP (stack);
            return Element {NAN, true};
        }
    }


    IF_HASH_PROTECTED ( STACK_RECOUNT_HASH (stack); );


    if (return_code_ptr) { *return_code_ptr = SUCCESS; }


    STACK_AFTER_OPERATION_DUMPING (stack);


    return return_element;
}


Stack_state  stack_damaged  (Stack* stack) {

    Stack_state stack_state = 0;


    if (stack == nullptr) { stack_state |= 1; }


    if (stack->size > stack->capacity)                  { stack_state |= (1<<1); }
    if (stack->elements == nullptr && stack->size != 0) { stack_state |= (1<<2); }


    for (size_t i = 0; i < stack->capacity; i++) {

        if (i <  stack->size) if ( stack->elements [i].poisoned) { stack_state |= (1<<3); break; }
        if (i >= stack->size) if (!stack->elements [i].poisoned) { stack_state |= (1<<3); break; }
    }

    IF_CANARY_PROTECTED (

        if (stack-> FIRST_CANARY !=  FIRST_CANARY_VALUE) { stack_state |= (1<<4);  }
        if (stack->SECOND_CANARY != SECOND_CANARY_VALUE) { stack_state |= (1<<4);  }

        if ( *( (canary_t*) stack->elements - 1)                  !=  FIRST_CANARY_VALUE) { stack_state |= (1<<5); }
        if ( *( (canary_t*) (stack->elements + stack->capacity) ) != SECOND_CANARY_VALUE) { stack_state |= (1<<5); }
    );


    IF_HASH_PROTECTED (

        hash_t old_hash = stack->hash; //printf ("hash in stack - %llX\n", old_hash);
        STACK_RECOUNT_HASH (stack);    //printf ("hash after check - %llX\n", stack->hash);
        if (old_hash != stack->hash) { stack_state |= (1<<6); }
    );


    //if (stack_state) { printf ("stack state - %hhu\n", stack_state); }
    return stack_state;
}


void  _fstack_dump  (Stack* stack, const char* file_name, const char* file, const char* func, int line) {

    assert (file != nullptr && func != nullptr);


    FILE* dump_file;
    if (file_name == nullptr) {

        dump_file = stdout;
    }

    else {

        dump_file = fopen (file_name, "a");
        if (dump_file == nullptr) { LOG_ERROR (FILE_ERR); return; }
    }


    setvbuf (dump_file, NULL, _IONBF, 0);


    fprintf (dump_file, "--------------------\n");
    fprintf (dump_file, "Dumping stack at %s in function %s (line %d)...\n\n", file, func, line);


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


    IF_HASH_PROTECTED (

        fprintf (dump_file, "stack hash is %llX\n\n", stack->hash);
    );


    fprintf (dump_file, "stack is ");
    Stack_state stack_state = stack_damaged (stack);
    if (stack_state) { fprintf (dump_file, "damaged (damage code %u)\n", stack_state); }
    else             { fprintf (dump_file, "ok\n"); }


    fprintf (dump_file, "size     %zd\n",      stack->size);
    fprintf (dump_file, "capacity %zd\n\n",  stack->capacity);
    if (stack->elements) { fprintf (dump_file, "elements [%p]:\n", stack->elements); }
    else                 { fprintf (dump_file, "elements [nullptr]:\n"); }


    for (size_t i = 0; i < stack->capacity; i++) {

        if (i < stack->size) { fprintf (dump_file, "(in)  "); }
        else                 { fprintf (dump_file, "(out) "); }

        fprintf (dump_file, "[%zd] = %-5lg (", i, stack->elements[i].value);         // different fprintf function
        if (stack->elements[i].poisoned) { fprintf (dump_file,     "poisoned)\n"); }
        else                             { fprintf (dump_file, "not poisoned)\n"); }
    }
    fprintf (dump_file, "\n");


    IF_CANARY_PROTECTED (
        fprintf (dump_file, "first  stack canary - %llX (", stack->FIRST_CANARY);

        if (stack->FIRST_CANARY == FIRST_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                           { fprintf (dump_file, "corrupted)\n"); }

        fprintf (dump_file, "second stack canary - %llX (", stack->SECOND_CANARY);

        if (stack->SECOND_CANARY == SECOND_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                             { fprintf (dump_file, "corrupted)\n"); }


        fprintf (dump_file, "first  data  canary - %llX (", *( (canary_t*) stack->elements - 1));

        if ( *( (canary_t*) stack->elements - 1) == FIRST_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                           { fprintf (dump_file, "corrupted)\n"); }

        fprintf (dump_file, "second data  canary - %llX (", *(canary_t*)(stack->elements + stack->capacity) );

        if ( *(canary_t*)(stack->elements + stack->capacity) == SECOND_CANARY_VALUE) { fprintf (dump_file, "untouched)\n"); }
        else                                             { fprintf (dump_file, "corrupted)\n"); }
    );

    fprintf (dump_file, "\n");


    fclose (dump_file);
}

IF_HASH_PROTECTED (

    hash_t  hash300  (void* _data_ptr, size_t size) {

    if (!_data_ptr) return 0;


    unsigned char* data_ptr = (unsigned char*) _data_ptr;


    hash_t hash_sum  = HASH_SALT;
    hash_t hash_salt = HASH_SALT; 
    for (size_t i = 0; i < size; i++) {

        hash_sum  ^= hash_sum * (data_ptr[i] * hash_salt);
    }


    return hash_sum;
}
);

IF_HASH_PROTECTED (

    Return_code  _stack_recount_hash  (Stack* stack) {

    if (!stack) { LOG_ERROR (BAD_ARGS); return BAD_ARGS; }


    hash_t hash1 = hash300 (stack, STACK_SIZE - HASH_SIZE);
    hash_t hash2 = hash300 (stack->elements, stack->capacity * ELEMENT_SIZE);


    stack->hash = hash1 ^ hash2;


    return SUCCESS;
}
);


IF_HASH_PROTECTED (

    IF_CANARY_PROTECTED (

        Return_code  _stack_canary_recount_hash  (Stack* stack) {

        if (!stack) { LOG_ERROR (BAD_ARGS); return BAD_ARGS; }


        hash_t hash1 = hash300 (stack, STACK_SIZE - HASH_SIZE - CANARY_SIZE);
        hash_t hash2 = hash300 (&stack->SECOND_CANARY, CANARY_SIZE);
        hash_t hash3 = hash300 ( (char*) stack->elements - CANARY_SIZE, stack->capacity + 2 * CANARY_SIZE);


        stack->hash = hash1 ^ hash2 ^ hash3; //printf ("%llX ^ %llX ^ %llX = %llX\n", hash1, hash2, hash3, stack->hash);


        return SUCCESS;
    }
    );
);




































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
        fprintf (log_file, "wrong error code given to the LOG_ERROR function in file %s in function %s (line %d)\n", file, func, line); break;
    }


    fclose (log_file);
}