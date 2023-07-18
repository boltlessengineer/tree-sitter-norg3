#include <bitset>
#include <cwctype>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <iostream>

#include "tree_sitter/parser.h"

using namespace std;

enum TokenType : char {
    WHITESPACE,
    WORD,

    BOLD_OPEN,
    BOLD_CLOSE,
    ITALIC_OPEN,
    ITALIC_CLOSE,
    STRIKETHROUGH_OPEN,
    STRIKETHROUGH_CLOSE,
    UNDERLINE_OPEN,
    UNDERLINE_CLOSE,
    SPOILER_OPEN,
    SPOILER_CLOSE,
    SUPERSCRIPT_OPEN,
    SUPERSCRIPT_CLOSE,
    SUBSCRIPT_OPEN,
    SUBSCRIPT_CLOSE,
    INLINE_COMMENT_OPEN,
    INLINE_COMMENT_CLOSE,
    VERBATIM_OPEN,
    VERBATIM_CLOSE,
    INLINE_MATH_OPEN,
    INLINE_MATH_CLOSE,
    INLINE_MACRO_OPEN,
    INLINE_MACRO_CLOSE,

    FREE_FORM_MOD_OPEN,
    FREE_FORM_MOD_CLOSE,
    LINK_MODIFIER,
    ESCAPE_SEQUENCE,

    // token for repeated attached modifier which should be parsed as `punc`
    PUNC,
    SOFT_BREAK,
    PARA_BREAK,

    ERROR_SENTINEL,

    COUNT
};

bool is_token_attached_mod(TokenType token, bool is_close) {
    return token >= BOLD_OPEN && token < FREE_FORM_MOD_OPEN && token % 2 == is_close;
}

struct Scanner {
    TSLexer* lexer;
    const std::unordered_map<int32_t, TokenType> attached_modifier_lookup = {
        {'*', BOLD_OPEN},        {'/', ITALIC_OPEN},       {'-', STRIKETHROUGH_OPEN},
        {'_', UNDERLINE_OPEN},   {'!', SPOILER_OPEN},      {'`', VERBATIM_OPEN},
        {'^', SUPERSCRIPT_OPEN}, {',', SUBSCRIPT_OPEN},    {'%', INLINE_COMMENT_OPEN},
        {'$', INLINE_MATH_OPEN}, {'&', INLINE_MACRO_OPEN},
    };

    std::bitset<COUNT> active_mods;
    std::bitset<COUNT> free_active_mods;

    TokenType last_token = WHITESPACE;

    bool scan(const bool *valid_symbols) {
        if (lexer->eof(lexer))
            return false;

        bool found_whitespace = false;
        unsigned int newline_count = 0;
        while (iswspace(lexer->lookahead)) {
            found_whitespace = true;
            if (lexer->lookahead == '\n') {
                newline_count++;
                advance();
            } else if (lexer->lookahead == '\r') {
                newline_count++;
                advance();
                if (lexer->lookahead == '\n')  advance();
            } else {
                advance();
            }
        }
        if (found_whitespace) {
            lexer->mark_end(lexer);
            if (newline_count > 1)
                lexer->result_symbol = last_token = PARA_BREAK;
            else if (newline_count > 0)
                lexer->result_symbol = last_token = SOFT_BREAK;
            else
                lexer->result_symbol = last_token = WHITESPACE;
            return true;
        }

        if (valid_symbols[ESCAPE_SEQUENCE] && lexer->lookahead == '\\') {
            advance();
            if (lexer->lookahead) {
                advance();
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = ESCAPE_SEQUENCE;
                return true;
            }
            return false;
        }

        if (valid_symbols[FREE_FORM_MOD_OPEN] && lexer->lookahead == '|' && is_token_attached_mod(last_token, false) && active_mods[last_token]) {
            free_active_mods[last_token] = true;
            advance();
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = FREE_FORM_MOD_OPEN;
            return true;
        }
        if (valid_symbols[FREE_FORM_MOD_CLOSE] && lexer->lookahead == '|') {
            advance();
            lexer->mark_end(lexer);
            const auto attached_mod = attached_modifier_lookup.find(lexer->lookahead);
            if (attached_mod != attached_modifier_lookup.end() && free_active_mods[attached_mod->second]) {
                free_active_mods[last_token] = false;
                lexer->result_symbol = last_token = FREE_FORM_MOD_CLOSE;
            }
            else {
                lexer->result_symbol = last_token = PUNC;
            }
            return true;
        }

        if (valid_symbols[LINK_MODIFIER] && lexer->lookahead == ':') {
            advance();
            if (
                (last_token == WORD && !iswspace(lexer->lookahead))
                || (is_token_attached_mod(last_token, true)
                    && lexer->lookahead
                    && !iswspace(lexer->lookahead)
                    && !iswpunct(lexer->lookahead))
            ) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = LINK_MODIFIER;
                return true;
            }
            return false;
        }

        const auto attached_mod = attached_modifier_lookup.find(lexer->lookahead);
        if (attached_mod != attached_modifier_lookup.end()) {
            const TokenType OPEN_MOD = attached_mod->second;
            const TokenType CLOSE_MOD = (TokenType)(OPEN_MOD + 1);
            if (!valid_symbols[OPEN_MOD] && !valid_symbols[CLOSE_MOD]) 
                // no valid attached modifier in current state
                return false;

            advance();

            if (lexer->lookahead == attached_mod->first) {
                // **
                do {
                    advance();
                } while(lexer->lookahead == attached_mod->first);
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = PUNC;
                return true;
            }

            if (valid_symbols[CLOSE_MOD]
                    && last_token != WHITESPACE
                    && (!lexer->lookahead
                        || iswspace(lexer->lookahead)
                        || lexer->lookahead == '\n'
                        || lexer->lookahead == '\r'
                        || iswpunct(lexer->lookahead))) {
                // _CLOSE
                active_mods[attached_mod->second] = false;
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = CLOSE_MOD;
                return true;
            } else if (valid_symbols[OPEN_MOD]
                    && last_token != WORD
                    && !active_mods[attached_mod->second]
                    && lexer->lookahead
                    && !iswspace(lexer->lookahead)) {
                // _OPEN
                active_mods[attached_mod->second] = true;
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = OPEN_MOD;
                return true;
            } else if (valid_symbols[PUNC]) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = PUNC;
                return true;
            }
            return false;
        }
        const bool found_punc = lexer->lookahead && iswpunct(lexer->lookahead);
        if (valid_symbols[WORD] && lexer->lookahead && !iswspace(lexer->lookahead) && !found_punc) {
            while (lexer->lookahead && !iswspace(lexer->lookahead) && !iswpunct(lexer->lookahead)) {
                advance();
            }
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = WORD;
            return true;
        }
        return false;
    }

    void skip() { lexer->advance(lexer, true); }
    void advance() { lexer->advance(lexer, false); }
};

extern "C" {
    void *tree_sitter_norg_external_scanner_create() { return new Scanner(); }

    void tree_sitter_norg_external_scanner_destroy(void *payload) {
        delete static_cast<Scanner *>(payload);
    }

    bool tree_sitter_norg_external_scanner_scan(void *payload, TSLexer *lexer,
            const bool *valid_symbols) {
        Scanner *scanner = static_cast<Scanner*>(payload);
        scanner->lexer = lexer;
        return scanner->scan(valid_symbols);
    }

    unsigned tree_sitter_norg_external_scanner_serialize(void *payload,
            char *buffer) {
        Scanner* scanner = static_cast<Scanner*>(payload);
        size_t i = 0;
        buffer[i++] = scanner->last_token;
        const size_t size = i + scanner->active_mods.size();
        for(; i < size; ++i) {
            buffer[i] = scanner->active_mods[i];
        }
        return i;
    }

    void tree_sitter_norg_external_scanner_deserialize(void *payload,
            const char *buffer,
            unsigned length) {
        Scanner* scanner = static_cast<Scanner*>(payload);
        scanner->last_token = WHITESPACE;
        scanner->active_mods.reset();
        if (length > 0) {
            size_t i = 0;
            scanner->last_token = (TokenType)buffer[i++];
            const size_t size = i + scanner->active_mods.size();
            for(; i < size; ++i) {
                scanner->active_mods[i] = buffer[i];
            }
        }
    }
}
