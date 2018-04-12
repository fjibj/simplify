// Copyright Ian R. Shehadeh 2018

#include "simplify/expression.h"
#include "simplify/errors.h"

void expression_free(expression_t* expr) {
    expression_clean(expr);
    free(expr);
}

void expression_clean(expression_t* expr) {
    switch (expr->type) {
        case EXPRESSION_TYPE_PREFIX:
            expression_free(expr->prefix.right);
            break;
        case EXPRESSION_TYPE_OPERATOR:
            expression_free(expr->operator.left);
            expression_free(expr->operator.right);
            break;
        case EXPRESSION_TYPE_NUMBER:
            SCALAR_CLEAN(expr->number.value);
            break;
        default:
            break;
    }
}

int variable_id(variable_t var) {
    if (var <= 'Z' && var >= 'A') {
        return var - 'A' + 25;
    } else if (var <= 'z' && var >= 'a') {
        return var - 'a';
    }
    return -1;
}

error_t expression_print(expression_t* expr) {
    if (!expr)
        return ERROR_NULL_EXPRESSION;

    switch (expr->type) {
        case EXPRESSION_TYPE_NUMBER:
        {
            int len = SCALAR_REQUIRED_CHARS(expr->number.value);
            char* buf = NULL;

            if (len < 256) {
                buf = alloca(len + 1);
            } else {
                buf = malloc(len + 1);
            }

            buf[len] = 0;

            SCALAR_TO_STRING(expr->number.value, buf);
            printf("%s", buf);
            if (len >= 256) {
                free(buf);
            }
            break;
        }
        case EXPRESSION_TYPE_VARIABLE:
            printf("%c", expr->variable.value);
            break;
        case EXPRESSION_TYPE_OPERATOR:
            expression_print(expr->operator.left);
            printf(" %c ", expr->operator.infix);
            expression_print(expr->operator.right);
            break;
        case EXPRESSION_TYPE_PREFIX:
            printf("%c", expr->prefix.prefix);
            expression_print(expr->prefix.right);
            break;
        default:
            printf("(UNDEFINED TYPE %d)", expr->type);
            break;
    }

    return ERROR_NO_ERROR;
}


error_t expression_to_bool(expression_t* expr) {
    switch (expr->type) {
        case EXPRESSION_TYPE_NUMBER:
            return ERROR_CANNOT_COMPARE;
        case EXPRESSION_TYPE_PREFIX:
            return ERROR_CANNOT_COMPARE;
        case EXPRESSION_TYPE_VARIABLE:
            return ERROR_CANNOT_COMPARE;
        case EXPRESSION_TYPE_OPERATOR:
        {
            if (expr->operator.left->type != EXPRESSION_TYPE_NUMBER ||
                expr->operator.right->type != EXPRESSION_TYPE_NUMBER) {
                    return ERROR_CANNOT_COMPARE;
            }

            int len = SCALAR_REQUIRED_CHARS(expr->operator.left->number.value);
            int len1 = SCALAR_REQUIRED_CHARS(expr->operator.right->number.value);

            char* buf = malloc(len + 1);
            char* buf1 = malloc(len1 + 1);

            buf[len] = 0;

            SCALAR_TO_STRING(expr->operator.left->number.value, buf);
            SCALAR_TO_STRING(expr->operator.right->number.value, buf1);

            scalar_t l;
            scalar_t r;

            SCALAR_FROM_STRING(buf, l);
            SCALAR_FROM_STRING(buf1, r);

            int x = SCALAR_COMPARE(l, r);

            free(buf);
            free(buf1);
            SCALAR_CLEAN(r);
            SCALAR_CLEAN(l);
            expression_free(expr->operator.left);
            expression_free(expr->operator.right);

            switch (expr->operator.infix) {
                case '<':
                    SCALAR_SET_INT(x < 0, expr->number.value);
                    break;
                case '>':
                    SCALAR_SET_INT(x > 0, expr->number.value);
                    break;
                case '=':
                    SCALAR_SET_INT(x == 0, expr->number.value);
                    break;
                default:
                    return ERROR_INVALID_OPERATOR;
            }
            expr->type = EXPRESSION_TYPE_NUMBER;
        }
    }
    return ERROR_NO_ERROR;
}

error_t expression_simplify_vars(expression_t* expr, expression_t** variables) {
    switch (expr->type) {
        case EXPRESSION_TYPE_VARIABLE:
        {
            if (variables[variable_id(expr->variable.value)] != NULL)
                *expr = *variables[variable_id(expr->variable.value)];
        }
        case EXPRESSION_TYPE_NUMBER:
            break;
        case EXPRESSION_TYPE_PREFIX:
        {
            error_t err = expression_simplify_vars(expr->prefix.right, variables);
            if (err) return err;

            if (expr->prefix.right->type == EXPRESSION_TYPE_NUMBER) {
                SCALAR_DEFINE(result);

                switch (expr->prefix.prefix) {
                    case '+':
                        SCALAR_SET(expr->prefix.right->number.value, result);
                        break;
                    case '-':
                        SCALAR_NEGATE(expr->prefix.right->number.value, result);
                        break;
                    default:
                        return ERROR_INVALID_PREFIX;
                }
                expression_free(expr->prefix.right);

                expr->type = EXPRESSION_TYPE_NUMBER;
                SCALAR_SET(result, expr->number.value);
            }
            break;
        }
        case EXPRESSION_TYPE_OPERATOR:
        {
            error_t err = expression_simplify_vars(expr->operator.right, variables);
            if (err) return err;

            err = expression_simplify_vars(expr->operator.left, variables);
            if (err) return err;


            if (expr->operator.right->type == EXPRESSION_TYPE_NUMBER
                && expr->operator.left->type == EXPRESSION_TYPE_NUMBER) {
                SCALAR_DEFINE(result);

                switch (expr->operator.infix) {
                    case '+':
                        SCALAR_ADD(expr->operator.left->number.value, expr->operator.right->number.value, result);
                        break;
                    case '-':
                        SCALAR_SUB(expr->operator.left->number.value, expr->operator.right->number.value, result);
                        break;
                    case '/':
                        SCALAR_DIV(expr->operator.left->number.value, expr->operator.right->number.value, result);
                        break;
                    case '*':
                    case '(':
                        SCALAR_MUL(expr->operator.left->number.value, expr->operator.right->number.value, result);
                        break;
                    case '^':
                        SCALAR_POW(expr->operator.left->number.value, expr->operator.right->number.value, result);
                        break;
                    case '=':
                    case '>':
                    case '<':
                        return expression_to_bool(expr);
                    default:
                        return ERROR_INVALID_OPERATOR;
                }
                expression_free(expr->operator.left);
                expression_free(expr->operator.right);

                expr->type = EXPRESSION_TYPE_NUMBER;
                SCALAR_SET(result, expr->number.value);
            }
            break;
        }
    }
    return ERROR_NO_ERROR;
}

int expression_isolate_variable(expression_t* expr, variable_t var) {
    // TODO(IanS5)
    return 1;
}

void expression_simplify(expression_t* expr) {
    expression_t* variables[26 * 2];
    memset(&variables[0], 0, 26 * 2 *sizeof(expression_t*));

    expression_simplify_vars(expr, &variables[0]);
}
