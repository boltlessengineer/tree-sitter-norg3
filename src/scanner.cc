#include <bitset>
#include <cwctype>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include <vector>

#include "tree_sitter/parser.h"

using namespace std;

enum TokenType : char {
    WHITESPACE,
    WORD,

    BOLD_OPEN,
    BOLD_CLOSE,
    FREE_BOLD_OPEN,
    FREE_BOLD_CLOSE,
    ITALIC_OPEN,
    ITALIC_CLOSE,
    FREE_ITALIC_OPEN,
    FREE_ITALIC_CLOSE,
    STRIKETHROUGH_OPEN,
    STRIKETHROUGH_CLOSE,
    FREE_STRIKETHROUGH_OPEN,
    FREE_STRIKETHROUGH_CLOSE,
    UNDERLINE_OPEN,
    UNDERLINE_CLOSE,
    FREE_UNDERLINE_OPEN,
    FREE_UNDERLINE_CLOSE,
    SPOILER_OPEN,
    SPOILER_CLOSE,
    FREE_SPOILER_OPEN,
    FREE_SPOILER_CLOSE,
    SUPERSCRIPT_OPEN,
    SUPERSCRIPT_CLOSE,
    FREE_SUPERSCRIPT_OPEN,
    FREE_SUPERSCRIPT_CLOSE,
    SUBSCRIPT_OPEN,
    SUBSCRIPT_CLOSE,
    FREE_SUBSCRIPT_OPEN,
    FREE_SUBSCRIPT_CLOSE,
    INLINE_COMMENT_OPEN,
    INLINE_COMMENT_CLOSE,
    FREE_INLINE_COMMENT_OPEN,
    FREE_INLINE_COMMENT_CLOSE,
    VERBATIM_OPEN,
    VERBATIM_CLOSE,
    FREE_VERBATIM_OPEN,
    FREE_VERBATIM_CLOSE,
    INLINE_MATH_OPEN,
    INLINE_MATH_CLOSE,
    FREE_INLINE_MATH_OPEN,
    FREE_INLINE_MATH_CLOSE,
    INLINE_MACRO_OPEN,
    INLINE_MACRO_CLOSE,
    FREE_INLINE_MACRO_OPEN,
    FREE_INLINE_MACRO_CLOSE,

    LINK_MODIFIER,
    ESCAPE_SEQUENCE,

    PUNC_END,

    ERROR_SENTINEL,

    COUNT
};

bool was_attached_close_mod(TokenType token) {
    return token >= BOLD_OPEN && token < LINK_MODIFIER && (token % 2 == BOLD_CLOSE % 2);
}

struct Scanner {
    TSLexer* lexer;
    const std::unordered_map<int32_t, TokenType> lookup = {
        {'*', BOLD_OPEN},        {'/', ITALIC_OPEN},       {'-', STRIKETHROUGH_OPEN},
        {'_', UNDERLINE_OPEN},   {'!', SPOILER_OPEN},      {'`', VERBATIM_OPEN},
        {'^', SUPERSCRIPT_OPEN}, {',', SUBSCRIPT_OPEN},    {'%', INLINE_COMMENT_OPEN},
        {'$', INLINE_MATH_OPEN}, {'&', INLINE_MACRO_OPEN},
    };

    // HACK: fix this to use less space
    std::bitset<COUNT> active_mods;
    char cache;

    TokenType last_token = WHITESPACE;

    bool is_word(char c) {
        return !iswspace(c) && !iswpunct(c);
    }

    bool scan(const bool *valid_symbols) {
        if (lexer->eof(lexer))
            return false;

        if (lexer->get_column(lexer) == 0)
            last_token = WHITESPACE;

        if (valid_symbols[PUNC_END] && last_token != PUNC_END) {
            lexer->mark_end(lexer);
            last_token = PUNC_END;
            return true;
        }

        cache = lexer->lookahead;
        advance();

        // ESCAPE SEQUENCE
        if (valid_symbols[ESCAPE_SEQUENCE] && cache == '\\') {
            if (lexer->lookahead) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = ESCAPE_SEQUENCE;
                return true;
            }
            return false;
        }
        // WHITESPACE
        if (iswblank(cache)) {
            while (iswblank(lexer->lookahead)) {
                advance();
            }
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = WHITESPACE;
            return true;
        }

        // WORD
        const bool found_punc = cache && iswpunct(cache);
        if (valid_symbols[WORD] && cache && is_word(cache)) {
            while (lexer->lookahead && !iswspace(lexer->lookahead) && !iswpunct(lexer->lookahead)) {
                advance();
            }
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = WORD;
            return true;
        }

        if (valid_symbols[LINK_MODIFIER] && last_token == WORD && cache == ':' && !iswspace(lexer->lookahead)) {
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = LINK_MODIFIER;
            return true;
        } else if (valid_symbols[LINK_MODIFIER] && was_attached_close_mod(last_token) && cache == ':' && lexer->lookahead && is_word(lexer->lookahead)) {
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = LINK_MODIFIER;
            return true;
        }

        const auto n_attached_mod = lookup.find(lexer->lookahead);
        if (cache == '|' && (n_attached_mod != lookup.end()) && valid_symbols[n_attached_mod->second + 3]) {
            advance();
            if (!lexer->lookahead || (!is_word(lexer->lookahead) && lexer->lookahead != n_attached_mod->first)) {
                lexer->mark_end(lexer);
                lexer->result_symbol = last_token = (TokenType)(n_attached_mod->second + 3);
                active_mods[n_attached_mod->second] = false;
                return true;
            }
            return false;
        }
        const auto attached_mod = lookup.find(cache);
        if (attached_mod != lookup.end() && valid_symbols[attached_mod->second + 2] && !active_mods[attached_mod->second] && last_token != WORD && lexer->lookahead == '|') {
            // _FREE_OPEN
            // (non word) ['*', '|']
            advance();
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = (TokenType)(attached_mod->second + 2);
            active_mods[attached_mod->second] = true;
            return true;
        }
        if (attached_mod != lookup.end() && valid_symbols[attached_mod->second + 1] && active_mods[attached_mod->second] && last_token != WHITESPACE && (!lexer->lookahead || (!is_word(lexer->lookahead) && lexer->lookahead != attached_mod->first))) {
            // _CLOSE
            // (-ws) ["*", -(word | "*")]
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = (TokenType)(attached_mod->second + 1);
            active_mods[attached_mod->second] = false;
            return true;
        }
        if (attached_mod != lookup.end() && valid_symbols[attached_mod->second] && !active_mods[attached_mod->second] && last_token != WORD && lexer->lookahead && !iswspace(lexer->lookahead) && (lexer->lookahead != attached_mod->first)) {
            // _OPEN
            // (-word) ["*", -(ws | "*")]
            lexer->mark_end(lexer);
            lexer->result_symbol = last_token = attached_mod->second;
            active_mods[attached_mod->second] = true;
            return true;
        }

        return false;
    }

    void advance() {
        lexer->advance(lexer, false);
    }
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
