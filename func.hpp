

#include <sys\stat.h>
#include <locale.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>


//-------------------- SETTINGS --------------------
#define ON_STACK_ERROR_DUMPING
#define ON_STACK_AFTER_OPERATION_DUMPING
#define ON_CANARY_PROTECTION
#define ON_HASH_PROTECTION

#define  log_file_name "logs.txt"
#define dump_file_name "dump.txt"

const double stack_resize_coefficient = 1.2;
//--------------------------------------------------


#define COMMA ,


#define log_error(code) _log_error (code, __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define  stack_ctor(x)  _stack_ctor (x, #x + (#x[0] == '&'), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define  stack_dump(x) _fstack_dump (x, nullptr,             __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define fstack_dump(x) _fstack_dump (x, dump_file_name,      __FILE__, __PRETTY_FUNCTION__, __LINE__)


#ifdef ON_STACK_ERROR_DUMPING

    #define assert_stack_ok(x) if (stack_damaged (x)) { fstack_dump (x); log_error (BAD_ARGS); return BAD_ARGS; }

    #define assert_stack_ok_for_stack_pop(x)\
        if (stack_damaged (x)) { fstack_dump (x); log_error (BAD_ARGS); if (return_code_ptr) { *return_code_ptr = BAD_ARGS; } return Element {NAN, true}; }

    #define stack_error_dump(x) fstack_dump(x)

#else

    #define assert_stack_ok(x)\
    if (stack_damaged (x)) {                 log_error (BAD_ARGS); return BAD_ARGS; }

    #define assert_stack_ok_myreturn(x, y)\
    if (stack_damaged (x)) {                 log_error (BAD_ARGS); if (return_code_ptr) { *return_code_ptr = BAD_ARGS; } return Element {NAN, true}; }

    #define stack_error_dump(x)

#endif


#ifdef ON_STACK_AFTER_OPERATION_DUMPING
    #define STACK_AFTER_OPERATION_DUMPING(x) fstack_dump (x)
#else
    #define STACK_AFTER_OPERATION_DUMPING(x)
#endif


#ifdef ON_CANARY_PROTECTION

    #define IF_CANARY_PROTECTED(x) x;
    #define CANARY_SIZE sizeof (canary_t)
    #define FIRST_CANARY_VALUE  0xDEADBEEF
    #define SECOND_CANARY_VALUE 0xDEADBEEF
    #define stack_resize(x,y)     _stack_canary_resize       (x, y)

    #ifdef ON_HASH_PROTECTION
        #define stack_recount_hash(x) _stack_canary_recount_hash (x)
    #endif

    #define stack_dtor(x)         _stack_canary_dtor         (x)

#else

    #define IF_CANARY_PROTECTED(x)  ;
    #define stack_resize(x,y)     _stack_resize       (x, y)

    #ifdef ON_HASH_PROTECTION
        #define stack_recount_hash(x) _stack_recount_hash (x)
    #endif

    #define stack_dtor(x)         _stack_dtor         (x)

#endif


#ifdef ON_HASH_PROTECTION
    #define IF_HASH_PROTECTED(x) x;
    #define HASH_SIZE sizeof (hash_t)
    #define HASH_MAX  ( (hash_t) -1)
    #define HASH_SALT ( (hash_t) 0xD1E2A3D4B5E6E7F )
#else
    #define IF_HASH_PROTECTED(x)  ;
#endif


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
IF_HASH_PROTECTED   (typedef unsigned long long   hash_t);

typedef unsigned char Stack_state; /*
    1:  &stack   == nullptr
    2:  size     >  capacity
    4:  elements == nullptr, size != 0
    8:  poison is distributed incorrectly
    16: stack canary corrupted
    32: data  canary corrupted
    64: stack has incorrect hash
*/


struct  Stack_info_structure  {

    const  char*  name;
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

    IF_HASH_PROTECTED (hash_t hash;);


    IF_CANARY_PROTECTED (canary_t SECOND_CANARY);
};

struct  Element_structure  {

    Element_value value;
    bool          poisoned;
};


const size_t   STACK_SIZE             = sizeof (Stack);
const size_t ELEMENT_SIZE             = sizeof (Element);
const size_t time_str_len             = 40;


Return_code _stack_ctor           (Stack* stack, const char* name, const char* file, const char* func, int line);
Return_code _stack_dtor           (Stack* stack);
Return_code _stack_canary_dtor    (Stack* stack);
Return_code _stack_resize         (Stack* stack, size_t new_capacity);
Return_code _stack_canary_resize  (Stack* stack, size_t new_capacity);
Return_code  stack_push           (Stack* stack, Element_value new_element_value);
Element      stack_pop            (Stack* stack, Return_code* return_code_ptr = nullptr);

Stack_state  stack_damaged        (Stack* stack);
void       _fstack_dump           (Stack* stack, const char* file_name, const char* file, const char* function, int line);

IF_HASH_PROTECTED (

    hash_t        hash300                      (void* _data_ptr, size_t size);
    Return_code  _stack_recount_hash           (Stack* stack);
    Return_code  _stack_canary_recount_hash    (Stack* stack);
);






void       _log_error              (Return_code, const char*, const char*, int);
void        log_message            (const char* message);
void        log_start              (void);
void        log_end                (void);
void        print_log_time         (void);
char*       tm_to_str              (struct tm* time_structure);

