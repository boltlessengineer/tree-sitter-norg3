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
    // IN_BOLD,
    ITALIC_OPEN,
    ITALIC_CLOSE,
    // IN_ITALIC,
    VERBATIM_OPEN,
    VERBATIM_CLOSE,
    // IN_VERBATIM,

    LINK_MODIFIER,
    ESCAPE_SEQUENCE,

    PUNC,

    ERROR_SENTINEL,

    COUNT
};

struct Scanner {
    TSLexer* lexer;
    const std::unordered_map<int32_t, TokenType> attached_modifier_lookup = {
        {'*', BOLD_OPEN},        {'/', ITALIC_OPEN},       {'`', VERBATIM_OPEN},
    };

    std::bitset<COUNT> active_mods;

    TokenType last_token = WHITESPACE;

    bool scan(const bool *valid_symbols) {
        if (lexer->eof(lexer))
            return false;

        const bool found_whitespace = iswspace(lexer->lookahead) && lexer->lookahead != '\n' && lexer->lookahead != '\r';
        if (found_whitespace) {
            while (iswspace(lexer->lookahead) && lexer->lookahead != '\n' && lexer->lookahead != '\r')
                advance();

            lexer->mark_end(lexer);
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
            } else {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = PUNC;
                return true;
            }
        }

        if (valid_symbols[LINK_MODIFIER] && lexer->lookahead == ':') {
            advance();
            if (
                (last_token == WORD && !iswspace(lexer->lookahead))
                || ((last_token == BOLD_CLOSE || last_token == ITALIC_CLOSE || last_token == VERBATIM_CLOSE)
                    && lexer->lookahead
                    && !iswspace(lexer->lookahead)
                    && !iswpunct(lexer->lookahead))
            ) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = LINK_MODIFIER;
                return true;
            } else {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = PUNC;
                return true;
            }
        }

        const auto attached_mod = attached_modifier_lookup.find(lexer->lookahead);
        if (attached_mod != attached_modifier_lookup.end()) {
            if (!valid_symbols[attached_mod->second] && !valid_symbols[attached_mod->second + 1]) 
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

            if (valid_symbols[attached_mod->second + 1]
                    && last_token != WHITESPACE
                    && (!lexer->lookahead
                        || iswspace(lexer->lookahead)
                        || lexer->lookahead == '\n'
                        || lexer->lookahead == '\r'
                        || iswpunct(lexer->lookahead))) {
                // _CLOSE
                active_mods[attached_mod->second] = false;
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = (TokenType)(attached_mod->second + 1);
                return true;
            } else if (valid_symbols[attached_mod->second]
                    // && !valid_symbols[attached_mod->second + 2]
                    && last_token != WORD
                    && !active_mods[attached_mod->second]
                    && lexer->lookahead
                    && !iswspace(lexer->lookahead)) {
                // _OPEN
                active_mods[attached_mod->second] = true;
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = attached_mod->second;
                return true;
            } else if (valid_symbols[PUNC]) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = PUNC;
                return true;
            }
        }
        const bool found_punc = lexer->lookahead && iswpunct(lexer->lookahead);
        if (valid_symbols[PUNC] && found_punc) {
            advance();
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = PUNC;
            return true;
        }
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
