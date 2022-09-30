

#include <sys\stat.h>
#include <locale.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

//-------------------- SETTINGS --------------------
#define ON_STACK_DUMPING
#define ON_CANARY_PROTECTION
#define ON_HASH_PROTECTION
//--------------------------------------------------

#define  log_file_name "logs.txt"
#define dump_file_name "dump.txt"


#define log_error(code) _log_error (code, __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define  stack_ctor(x)  _stack_ctor (x, #x + (#x[0] == '&'), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define  stack_dump(x) _fstack_dump (x, nullptr,             __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define fstack_dump(x) _fstack_dump (x, dump_file_name,      __FILE__, __PRETTY_FUNCTION__, __LINE__)


#ifdef ON_STACK_DUMPING

    #define assert_stack_ok(x)\
    if (stack_damaged (x)) { stack_dump (x); log_error (BAD_ARGS); return BAD_ARGS; }

    #define assert_stack_ok_for_stack_pop(x)\
    if (stack_damaged (x)) { stack_dump (x); log_error (BAD_ARGS); if (return_code_ptr) { *return_code_ptr = BAD_ARGS; } return Element {NAN, true}; }

#else

    #define assert_stack_ok(x)\
    if (stack_damaged (x)) {                 log_error (BAD_ARGS); return BAD_ARGS; }

    #define assert_stack_ok_myreturn(x, y)\
    if (stack_damaged (x)) {                 log_error (BAD_ARGS); if (return_code_ptr) { *return_code_ptr = BAD_ARGS; } return Element {NAN, true}; }

#endif


#ifdef ON_CANARY_PROTECTION
    #define IF_CANARY_PROTECTED(x) x;
    #define CANARY_SIZE sizeof (unsigned long long)
    #define FIRST_CANARY_VALUE  0xDEADBEEF
    #define SECOND_CANARY_VALUE 0xDEADBEEF
    #define stack_resize(x,y) _stack_canary_resize (x, y);
#else
    #define IF_CANARY_PROTECTED(x)  ;
    #define CANARY_MODE false
    #define stack_resize(x,y) _stack_resize (x, y);

#endif


#ifdef ON_HASH_PROTECTION
    #define IF_HASH_PROTECTED(x) x;
#else
    #define IF_HASH_PROTECTED(x)  ;
#endif


const double stack_resize_coefficient = 2;
const size_t time_str_len             = 40;


enum  Return_code  {

    SUCCESS    = 0,
    MEMORY_ERR = 1,
    BAD_ARGS   = 2,
    FILE_ERR   = 3,
};


typedef struct Stack_structure      Stack;
typedef double                      Element_value;
typedef struct Element_structure    Element;
typedef struct Stack_info_structure Stack_info;
IF_CANARY_PROTECTED (typedef unsigned long long canary_t);

typedef unsigned char Stack_state; /*
    1:  &stack   == nullptr
    2:  size     >  capacity
    4:  elements == nullptr, size != 0
    8:  poison is distributed incorrectly
    16: stack canary corrupted
    32: data  canary corrupted
*/


struct  Stack_info_structure  {

    const  char*  name; //calloc?
    Stack*        adress;
    const  char*  birth_file;
    const  char*  birth_func;
    int           birth_line;
};

struct  Stack_structure  {

    IF_CANARY_PROTECTED (canary_t FIRST_CANARY);


    Element* elements;
    size_t   size;
    size_t   capacity;

    Stack_info debug_info;


    IF_CANARY_PROTECTED (canary_t SECOND_CANARY);
};

struct  Element_structure  {

    Element_value value;
    bool          poisoned;
};


Return_code _stack_ctor           (Stack* stack, const char* name, const char* file, const char* func, int line);
Return_code  stack_dtor           (Stack* stack);
Return_code _stack_resize         (Stack* stack, size_t new_capacity);
Return_code _stack_canary_resize  (Stack* stack, size_t new_capacity);
Return_code  stack_push           (Stack* stack, Element_value new_element_value);
Element      stack_pop            (Stack* stack, Return_code* return_code_ptr = nullptr);
Stack_state  stack_damaged        (Stack* stack);
void       _fstack_dump           (Stack* stack, const char* file_name, const char* file, const char* function, int line);






void       _log_error              (Return_code, const char*, const char*, int);
void        log_message            (const char* message);
void        log_start              (void);
void        log_end                (void);
void        print_log_time         (void);
char*       tm_to_str              (struct tm* time_structure);

