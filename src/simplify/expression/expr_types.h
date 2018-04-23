/* Copyright Ian Shehadeh 2018 

* @file expr_types.h
* expression type definitions and utilities.
*/

#ifndef SIMPLIFY_EXPRESSION_EXPR_TYPES_H_
#define SIMPLIFY_EXPRESSION_EXPR_TYPES_H_

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include "simplify/errors.h"
#include "simplify/rbtree/rbtree.h"

#define EXPRESSION_IS_OPERATOR(EXPR) ((EXPR)->type == (EXPRESSION_TYPE_OPERATOR))
#define EXPRESSION_IS_VARIABLE(EXPR) ((EXPR)->type == (EXPRESSION_TYPE_VARIABLE))
#define EXPRESSION_IS_NUMBER(EXPR)   ((EXPR)->type == (EXPRESSION_TYPE_NUMBER))
#define EXPRESSION_IS_PREFIX(EXPR)   ((EXPR)->type == (EXPRESSION_TYPE_PREFIX))

#define EXPRESSION_RIGHT(EXPR) \
    (EXPRESSION_IS_PREFIX(EXPR) ? \
    ((EXPR)->prefix.right) : (EXPRESSION_IS_OPERATOR(EXPR) ? \
    (EXPR->operator.right) : (expression_t*)NULL))

#define EXPRESSION_LEFT(EXPR) (EXPRESSION_IS_OPERATOR(EXPR) ? (EXPR->operator.left) : (expression_t*)NULL)


#define EXPRESSION_OPERATOR(EXPR)  \
    (EXPRESSION_IS_PREFIX(EXPR) ? \
    ((EXPR)->prefix.prefix) : (EXPRESSION_IS_PREFIX(EXPR)   ? \
    (EXPR->operator.infix)  : '\0'))

#define EXPRESSION_LIST_FOREACH(I, EXPR_LIST)        \
    (I) = (EXPR_LIST)->value;                        \
    for (expression_list_t* __item = (EXPR_LIST);    \
            __item && (I = __item->value);           \
            __item = __item->next)

/* A parsed mathmatical expression.
 *
 * An expression is a union of four structures:
 * 
 * - number
 * 
 * - prefix
 * 
 * - variable
 * 
 * - operator
 * 
 * Which one of these structures is currently in use is stored in the `type` option.
 * The EXPRESSION_IS_XXX macros can be used to conveniently check the type of an expression.
 * See the Expression Structures category for more information on each structure.
 */
typedef union expression      expression_t;


/* A scope holds information about an expression's local variables and functions
 */
typedef struct scope scope_t;

/* Holds information about a variable, constant, or function
 */
typedef struct variable_info variable_info_t;

/* Specifies an operator's type.
 * An operator represents a specific operation performed on one or more numbers.
 * At the moment an operator is just the literal token stored in a `char`. This is liable to change in the future
 */
typedef char                  operator_t;

/* A variable is a null terminated string.
 */
typedef char*                 variable_t;

/* Enumerates the values that can be held in `expression_t`.
 */
typedef enum  expression_type expression_type_t;

/* A singly linked list of expressions
 */
typedef struct expression_list expression_list_t;

/* Operator precedence enumerates the different precedence levels for groups of operators.
 */
typedef enum operator_precedence operator_precedence_t;

enum operator_precedence {
    OPERATOR_PRECEDENCE_MINIMUM,
    OPERATOR_PRECEDENCE_ASSIGN,
    OPERATOR_PRECEDENCE_COMPARE,
    OPERATOR_PRECEDENCE_SUM,
    OPERATOR_PRECEDENCE_PRODUCT,
    OPERATOR_PRECEDENCE_EXPONENT,
    OPERATOR_PRECEDENCE_MAXIMUM,
};

enum expression_type {
    EXPRESSION_TYPE_NUMBER,
    EXPRESSION_TYPE_PREFIX,
    EXPRESSION_TYPE_OPERATOR,
    EXPRESSION_TYPE_VARIABLE,
    EXPRESSION_TYPE_FUNCTION,
};


struct variable_info {
    int                constant;
    expression_list_t* named_inputs;
    expression_t*      value;
};


struct scope {
    scope_t* parent;
    rbtree_t variables;
    int      boolean;   /* The result of all boolean operations so far */
};


struct expression_list {
    expression_t* value;
    struct expression_list* next;
};


/* cldoc:begin-category(Expression Structures) */

struct expression_number {
    expression_type_t type;
    mpfr_t value;
};

struct expression_function {
    expression_type_t  type;
    variable_t         name;
    expression_list_t* parameters;
};

struct expression_prefix {
    expression_type_t type;
    operator_t    prefix;
    expression_t* right;
};


struct expression_variable {
    expression_type_t type;
    variable_t value;
    scope_t*   binding;
};

struct expression_operator {
    expression_type_t type;

    expression_t* left;
    operator_t    infix;
    expression_t* right;
};

union expression {
    expression_type_t type;
    struct expression_number   number;
    struct expression_variable variable;
    struct expression_prefix   prefix;
    struct expression_operator operator;
    struct expression_function function;
};

/* cldoc:end-category() */


/* initialize an operator expression
 * @expression the expression to initialize
 * @left the left side of the operator expression
 * @operator the infix operator in the expression
 * @right the right side of the operator expression
 */
void expression_init_operator(expression_t* expression,  expression_t* left, operator_t operator, expression_t* right);

/* initialize a prefix expression
 * @expression the expression to initialize
 * @operator the prefix operator in the expression
 * @right the expression to the right of the prefix
 */
void expression_init_prefix(expression_t* expression, operator_t operator, expression_t* right);

/* initialize a variable expression
 * @expression the expression to initialize
 * @name the variable's name
 * @length the length of the variable's name
 * 
 * NOTE: this function copies the value `name` is pointing to up through `length`.
 */
void expression_init_variable(expression_t* expression, char* name, size_t length);


/* initialize a function expression
 * @expression the expression to initialize
 * @name the function's name
 * @length the length of the function's name
 * @args the function's arguments
 *
 * NOTE: this function copies the value `name` is pointing to up through `length`.
 */
void expression_init_function(expression_t* expression, char* name, size_t length, expression_list_t* arguments);

/* initialize a new number expression
 * @expression the expression to initialize
 * @number the number to use as the expression's initial value
 */
void expression_init_number(expression_t* expression, mpfr_t number);

/* initialize a new number expression with a double
 * @expression the expression to initialize
 * @number the number to use as the expression initial value
 */
void expression_init_number_d(expression_t* expression, double number);


/* initialize a new number expression with an integer
 * @expression the expression to initialize
 * @number the number to use as the expression initial value
 */
void expression_init_number_si(expression_t* expression, int number);

/* free all memory referenced by expression recursively, but not expression itself.
 * @expression the expression to clean
 */
void expression_clean(expression_t* expression);

/* free all memory referenced by the expression and the expression.
 * @expression the expression to free
 */
void expression_free(expression_t* expression);

/* deep copy expression into expression2.
 * @expression the source expression
 * @expression2 the destination expression
 */
void expression_copy(expression_t* expression, expression_t* expression2);

/* get the precedence of a given operator (higher = should be executed first)
 * @op the operator to check
 * 
 * @returns `op's` precedence
 */
extern operator_precedence_t operator_precedence(operator_t op);

/* check if the expression is an operator expression that compares it's left & right branches.
 * @expression the expression to check
 * @returns a boolean integer, true if the expression has a comparison operator.
 */
static inline int expression_is_comparison(expression_t* expr) {
    return EXPRESSION_IS_OPERATOR(expr) &&
        (expr->operator.infix == '<' ||
        expr->operator.infix == '>' ||
        expr->operator.infix == '=');
}

/* append a new element to the end of `list`
 * 
 * @list the list to append to
 * @expr the expression to append
 */
static inline void expression_list_append(expression_list_t* list, expression_t* expr) {
    if (!list->value) {
        list->value = expr;
        return;
    }

    expression_list_t* next = malloc(sizeof(expression_list_t));
    next->value = expr;
    next->next  = NULL;
    while (list->next) list = list->next;

    list->next = next;
}

/* free the last element of the list (that may be `list` itself)
 * 
 * @list the list to pop
 */
static inline void expression_list_pop(expression_list_t* list) {
    while (list->next) list = list->next;
    if (list->value)
        expression_free(list->value);
    free(list);
}

/* initialize an expression list
 * @list the list to initialize
 */
static inline void expression_list_init(expression_list_t* list) {
    list->value = NULL;
    list->next = NULL;
}

/* free all expressions and units in the list
 * @list the list to clean
 */
static inline void expression_list_free(expression_list_t* list) {
    expression_list_t* last = NULL;
    while (list) {
        if (list->value) expression_free(list->value);
        last = list;
        list = list->next;
        free(last);
    }
}

/* copy all elements in one list to another
 *
 * @list1 the source list
 * @list2 the destination list
 */
static inline void expression_list_copy(expression_list_t* list1, expression_list_t* list2) {
    expression_t* expr;
    EXPRESSION_LIST_FOREACH(expr, list1) {
        expression_t* copy = malloc(sizeof(expression_t));
        expression_copy(expr, copy);
        expression_list_append(list2, copy);
    }
}


/* Initialize a scope
 *
 * @scope the scope to initialize
 */
static inline void scope_init(scope_t* scope) {
    rbtree_init(&scope->variables);
    scope->parent = NULL;
    scope->boolean = -1;
}

/* define a variable in the scope
 *
 * @scope the scope to define the variable in
 * @variable the variable's name
 * @value the variable's value
 * @return return an error code
 */
static inline error_t scope_define(scope_t* scope, char* variable, expression_t* value) {
    variable_info_t* info = malloc(sizeof(variable_info_t));
    info->value = value;
    info->named_inputs = NULL;
    info->constant = 0;
    return rbtree_insert(&scope->variables, variable, info);
}

/* define a constant in the scope
 *
 * @scope the scope to define the constant in
 * @name the constant's name
 * @value the constant's value
 * @return return an error code
 */
static inline error_t scope_define_constant(scope_t* scope, char* variable, expression_t* value) {
    variable_info_t* info = malloc(sizeof(variable_info_t));
    info->value = value;
    info->named_inputs = NULL;
    info->constant = 0;
    return rbtree_insert(&scope->variables, variable, info);
}

/* define a function in the scope
 *
 * @scope the scope to define the function in
 * @name the function's name
 * @body the functions's body
 * @args the function's arguments
 * @return return an error code
 */
static inline error_t scope_define_function(scope_t* scope,
                                            char* name,
                                            expression_t* value,
                                            expression_list_t* args) {
    variable_info_t* info = malloc(sizeof(variable_info_t));
    info->value = value;
    info->named_inputs = args;
    info->constant = 0;
    return rbtree_insert(&scope->variables, name, info);
}


/* get information about a variable
 *
 * @scope the scope to search for the variable
 * @variable the variable's name
 * @value location to store the variable info 
 * @return returns an error code
 */
static inline error_t scope_get_variable_info(scope_t* scope, char* variable, variable_info_t** value) {
    error_t err = rbtree_search(&scope->variables, variable, (void**)value);

    /* check the parent scope(s) for the variable */
    scope_t* parent = scope->parent;
    while (err && parent != NULL) {
        err = rbtree_search(&parent->variables, variable, (void**)value);
        parent = parent->parent;
    }
    return err;
}

/* get the value of a variable or constant
 * @scope the scope to search
 * @name the name to look for
 * @expr location for result
 * @return returns an error code
 */
static inline error_t scope_get_value(scope_t* scope, char* name, expression_t* expr) {
    variable_info_t* info;
    error_t err = scope_get_variable_info(scope, name, &info);
    if (err) return err;
    if (info->named_inputs)
        return ERROR_IS_A_FUNCTION;

    expression_copy(info->value, expr);
    return ERROR_NO_ERROR;
}

/* search a scope for a function
 * @scope the scope to search
 * @name the name to look for
 * @body location for function's body
 * @args location for argument list
 * @return returns an error code
 */
static inline error_t scope_get_function(scope_t* scope, char* name, expression_t* body, expression_list_t* args) {
    variable_info_t* info;
    error_t err = scope_get_variable_info(scope, name, &info);
    if (err) return err;
    if (!info->named_inputs)
        return ERROR_IS_A_VARIABLE;

    expression_list_init(args);
    expression_list_copy(info->named_inputs, args);

    expression_copy(info->value, body);
    return ERROR_NO_ERROR;
}

static void variable_info_free(variable_info_t* info) {
    if (info->named_inputs)
        expression_list_free(info->named_inputs);
    expression_free(info->value);
    free(info);
}

/* clean all resources associated with a scope
 *
 * @scope the scope to clean
 */
static inline void scope_clean(scope_t* scope) {
    rbtree_clean(&scope->variables, (void(*)(void*))&variable_info_free);
}


#endif  // SIMPLIFY_EXPRESSION_EXPR_TYPES_H_
