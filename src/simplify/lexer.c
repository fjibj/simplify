/* Copyright Ian Shehadeh 2018 */

#include "simplify/lexer.h"
#include "simplify/errors.h"

#define __LEXER_LOOP_END() __lexer_loop_continue = 0

#define __LEXER_LOOP(LEXER, BLOCK) {                           \
    int __lexer_loop_continue = 1;                             \
    while (1) {                                                \
        if ((LEXER)->buffer_position >= (LEXER)->buffer_length)\
            break;                                             \
        BLOCK;                                                 \
        if (__lexer_loop_continue) {                           \
            lexer_advance(LEXER);                              \
        } else {                                               \
            break;                                             \
        }                                                      \
    }                                                          \
}

#define __LEXER_SKIP_WHILE(LEXER, FUNC) __LEXER_LOOP(LEXER, if (!FUNC(lexer_current(LEXER))) __LEXER_LOOP_END())

static inline char lexer_current(lexer_t* lexer) {
    return lexer->buffer[lexer->buffer_position];
}

static inline void lexer_advance(lexer_t* lexer) {
    ++lexer->buffer_position;
}

static inline int lexer_eof(lexer_t* lexer) {
    return lexer->buffer_position >= lexer->buffer_length;
}

error_t lexer_get_number(lexer_t* lexer, token_t* token) {
    token->start = lexer->buffer + lexer->buffer_position;

    __LEXER_SKIP_WHILE(lexer, isdigit);

    token->length = lexer->buffer + lexer->buffer_position - token->start;
    if (!lexer_eof(lexer) && lexer_current(lexer) == '.') {
        lexer_advance(lexer);
        __LEXER_SKIP_WHILE(lexer, isdigit);
    }

    if (!lexer_eof(lexer) && (lexer_current(lexer) == 'e' || lexer_current(lexer) == 'E')) {
        lexer_advance(lexer);
        if (lexer_current(lexer) == '-')
            lexer_advance(lexer);
        __LEXER_SKIP_WHILE(lexer, isdigit);
    }

    token->type = TOKEN_TYPE_NUMBER;

    token->length = lexer->buffer + lexer->buffer_position - token->start;
    return ERROR_NO_ERROR;
}

error_t lexer_next(lexer_t* lexer, token_t* token) {
    __LEXER_SKIP_WHILE(lexer, isspace);

    if (lexer_eof(lexer)) {
        token->type = TOKEN_TYPE_EOF;
        token->length = 0;
        token->start = lexer->buffer + lexer->buffer_position;
        return ERROR_NO_ERROR;
    }

    switch (lexer_current(lexer)) {
        case '+':
        case '-':
        case '/':
        case '\\':
        case '*':
        case '^':
        case '=':
        case '>':
        case '<':
        case ':':
            token->type = TOKEN_TYPE_OPERATOR;
            token->start = lexer->buffer + lexer->buffer_position;
            token->length = 1;
            ++lexer->buffer_position;
            break;
        case '(':
            token->type = TOKEN_TYPE_LEFT_PAREN;
            token->start = lexer->buffer + lexer->buffer_position;
            token->length = 1;
            ++lexer->buffer_position;
            break;
        case ')':
            token->type = TOKEN_TYPE_RIGHT_PAREN;
            token->start = lexer->buffer + lexer->buffer_position;
            token->length = 1;
            ++lexer->buffer_position;
            break;
        case ',':
            token->type = TOKEN_TYPE_COMMA;
            token->start = lexer->buffer + lexer->buffer_position;
            token->length = 1;
            ++lexer->buffer_position;
            break;
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':
            token->type  = TOKEN_TYPE_IDENTIFIER;
            token->start = lexer->buffer + lexer->buffer_position;
            __LEXER_SKIP_WHILE(lexer, isident)
            token->length = lexer->buffer + lexer->buffer_position - token->start;
            break;
        case '0' ... '9':
        case '.':
            return lexer_get_number(lexer, token);
        default:
            return ERROR_INVALID_CHARACTER;
        }

    return ERROR_NO_ERROR;
}
