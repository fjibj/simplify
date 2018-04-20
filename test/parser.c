/* Copyright Ian Shehadeh 2018 */

#include "test/test.h"
#include "simplify/parser.h"

static inline expression_t* expression_new_operator(expression_t* left, operator_t op, expression_t* right) {
    expression_t* x = malloc(sizeof(expression_t));
    expression_init_operator(x, left, op, right);
    return x;
}

static inline expression_t* expression_new_prefix(operator_t op, expression_t* right) {
    expression_t* x = malloc(sizeof(expression_t));
    expression_init_prefix(x, op, right);
    return x;
}

static inline expression_t* expression_new_number(double num) {
    expression_t* x = malloc(sizeof(expression_t));
    expression_init_number_d(x, num);
    return x;
}

static inline expression_t* expression_new_variable(variable_t var) {
    expression_t* x = malloc(sizeof(expression_t));
    expression_init_variable(x, var, strlen(var));
    return x;
}

int main() {
    struct {
        char*        string;
        expression_t* expr;
    } __string_expr_pairs[6] = {
        { "2 * 5.5",
            expression_new_operator(expression_new_number(2), '*', expression_new_number(5.5))
        },
        { "2 * ( 5 + 9 )",
            expression_new_operator(
                expression_new_number(2),
                '*',
                expression_new_operator(expression_new_number(5), '+', expression_new_number(9))),
        },
        { "2 + 3^2 * 5.5",
            expression_new_operator(
                expression_new_number(2),
                '+',
                expression_new_operator(
                    expression_new_operator(expression_new_number(3), '^', expression_new_number(2)),
                    '*',
                    expression_new_number(5.5)))
        },
        { "1 \\ 2 = 73",
            expression_new_operator(
                expression_new_operator(expression_new_number(1), '\\', expression_new_number(2)),
                '=',
                expression_new_number(73))
        },
        { "10 * 20 * 30^1 * 40",
            expression_new_operator(
                expression_new_operator(
                    expression_new_operator(
                        expression_new_number(10),
                        '*',
                        expression_new_number(20)),
                    '*',
                    expression_new_operator(
                        expression_new_number(30),
                        '^',
                        expression_new_number(1))),
                '*',
                expression_new_number(40))
        },
        { "2 * -5.5",
            expression_new_operator(
                expression_new_number(2),
                '*',
                expression_new_prefix('-', expression_new_number(5.5)))
        },
    };


    error_t err;
    for (int i = 0; i < 6; ++i) {
        expression_t expr;
        err = parse_string(__string_expr_pairs[i].string, &expr);
        if (err)
            FATAL("failed to parse string: %s", error_string(err));
        expression_assert_eq(&expr, __string_expr_pairs[i].expr);
    }
}