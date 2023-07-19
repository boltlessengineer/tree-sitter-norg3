let newline = choice("\n", "\r", "\r\n");
let newline_or_eof = choice("\n", "\r", "\r\n", "\0");
// !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

// const alias = (a, b) => a;

module.exports = grammar({
    name: "norg",

    extras: $ => [],
    externals: $ => [
        // handle whitespace, word, _open/_close inline mods ourselves to check preceding word character
        $._whitespace,
        $._soft_break,
        $._para_break,
        $.word,
        $._punc_ch,

        $.bold_open,
        $.bold_close,
        $.free_bold_open,
        $.free_bold_close,
        $.italic_open,
        $.italic_close,
        $.free_italic_open,
        $.free_italic_close,
        $.strikethrough_open,
        $.strikethrough_close,
        $.free_strikethrough_open,
        $.free_strikethrough_close,
        $.underline_open,
        $.underline_close,
        $.free_underline_open,
        $.free_underline_close,
        $.spoiler_open,
        $.spoiler_close,
        $.free_spoiler_open,
        $.free_spoiler_close,
        $.superscript_open,
        $.superscript_close,
        $.free_superscript_open,
        $.free_superscript_close,
        $.subscript_open,
        $.subscript_close,
        $.free_subscript_open,
        $.free_subscript_close,
        $.inline_comment_open,
        $.inline_comment_close,
        $.free_inline_comment_open,
        $.free_inline_comment_close,
        $.verbatim_open,
        $.verbatim_close,
        $.free_verbatim_open,
        $.free_verbatim_close,
        $.inline_math_open,
        $.inline_math_close,
        $.free_inline_math_open,
        $.free_inline_math_close,
        $.inline_macro_open,
        $.inline_macro_close,
        $.free_inline_macro_open,
        $.free_inline_macro_close,

        $.link_modifier,
        $.escape_sequence_prefix,

        $.detached,

        $.error_sentinel,
    ],
    conflicts: $ => [
        [$.bold, $.mod_conflict],
        [$.italic, $.mod_conflict],
        [$.strikethrough, $.mod_conflict],
        [$.underline, $.mod_conflict],
        [$.spoiler, $.mod_conflict],
        [$.superscript, $.mod_conflict],
        [$.subscript, $.mod_conflict],
        [$.inline_comment, $.mod_conflict],
        [$.verbatim, $.mod_conflict],
        [$.inline_math, $.mod_conflict],
        [$.inline_macro, $.mod_conflict],
        [$._attached_modifier, $.mod_conflict],
        [$._attached_modifier],
    ],
    inlines: $ => [],
    supertypes: $ => [
        $.non_structural,
    ],
    rules: {
        document: $ => repeat(
            choice(
                $.non_structural,
                // $.heading,
            )
        ),
        non_structural: $ => prec.right(1, choice(
            $.paragraph,
            $._para_break,
        )),
        paragraph: $ => prec.right(seq(
            repeat1(
                choice(
                    $.paragraph_segment,
                    $._soft_break,
                )
            ),
        )),
        paragraph_segment: $ => prec.right(repeat1(
            choice(
                $._paragraph_element,
            )
        )),
        _paragraph_element: $ =>
            choice(
                $.escape_sequence,
                alias($.word, "_word"),
                $._whitespace,
                alias($.punc, "_word"),
                alias($.mod_conflict, "_word"),
                // $.punc,
                // $.mod_conflict,
                $._attached_modifier,
            ),
        // heading: $ =>
        //     seq(
        //         "*",
        //         $._whitespace,
        //         $.paragraph_segment,
        //     ),
        escape_sequence: $ =>
            seq(
                $.escape_sequence_prefix,
                field(
                    "token",
                    alias(choice(/./, newline), $.any_char),
                ),
            ),
        // TODO: handle PUNC in grammar.js. to have higher lexer precedence for headings
        punc: $ =>
            // $._punc_ch,
            choice(
                $._punc_ch,
                "*",
                ".",
                ",",
                "<",
                ">",
                "(",
                ")",
                "\\",
                ":",
                "|"
            ),
        _attached_mod_content: $ =>
            prec.right(
                repeat1(
                    choice(
                        $._paragraph_element,
                        $._soft_break,
                    ),
                )
            ),
        _verbatim_paragraph_content: $ =>
            prec.right(
                repeat1(
                    choice(
                        $.escape_sequence,
                        alias($.word, "_word"),
                        $._whitespace,
                        alias($.punc, "_word"),
                        alias($.mod_conflict, "_word"),
                        // $.punc,
                        // $.mod_conflict,
                        $._soft_break,
                    ),
                ),
            ),
        _free_form_verbatim_mod_content: $ =>
            prec.right(
                repeat1(
                    choice(
                        choice(
                            $.escape_sequence,
                            alias($.word, "_word"),
                            $._whitespace,
                            alias($.punc, "_word"),
                            alias($.mod_conflict, "_word"),
                            // $.punc,
                            // $.mod_conflict,
                        ),
                        $._soft_break,
                        prec(1, alias($.escape_sequence_prefix, "_word")),
                    )
                )
            ),
        _attached_modifier: $ =>
            seq(
                optional($.link_modifier),
                choice(
                    $.bold,
                    $.italic,
                    $.strikethrough,
                    $.underline,
                    $.spoiler,
                    $.superscript,
                    $.subscript,
                    $.inline_comment,
                    $.verbatim,
                    $.inline_math,
                    $.inline_macro,
                ),
                optional($.link_modifier),
            ),
        mod_conflict: $ =>
            alias(
            choice(
                $.bold_open,
                $.italic_open,
                $.strikethrough_open,
                $.underline_open,
                $.spoiler_open,
                $.superscript_open,
                $.subscript_open,
                $.inline_comment_open,
                $.verbatim_open,
                $.inline_math_open,
                $.inline_macro_open,
                $.free_bold_open,
                $.free_italic_open,
                $.free_strikethrough_open,
                $.free_underline_open,
                $.free_spoiler_open,
                $.free_superscript_open,
                $.free_subscript_open,
                $.free_inline_comment_open,
                $.free_verbatim_open,
                $.free_inline_math_open,
                $.free_inline_macro_open,
                $.link_modifier,
            ),
            "_punc"),
        bold: $ => gen_attached_modifier($, "bold", false),
        italic: $ => gen_attached_modifier($, "italic", false),
        strikethrough: $ => gen_attached_modifier($, "strikethrough", false),
        underline: $ => gen_attached_modifier($, "underline", false),
        spoiler: $ => gen_attached_modifier($, "spoiler", false),
        superscript: $ => gen_attached_modifier($, "superscript", false),
        subscript: $ => gen_attached_modifier($, "subscript", false),
        inline_comment: $ => gen_attached_modifier($, "inline_comment", false),
        verbatim: $ => gen_attached_modifier($, "verbatim", true),
        inline_math: $ => gen_attached_modifier($, "inline_math", true),
        inline_macro: $ => gen_attached_modifier($, "inline_macro", true),
    },
});

function gen_attached_modifier($, kind, verbatim) {
    let open = alias($[kind + "_open"], "_open");
    let close = alias($[kind + "_close"], "_close");
    let free_open = alias($["free_" + kind + "_open"], $.free_form_open);
    let free_close = alias($["free_" + kind + "_close"], $.free_form_close);
    // let open = $[kind + "_open"];
    // let close = $[kind + "_close"];
    // let free_open = $["free_" + kind + "_open"];
    // let free_close = $["free_" + kind + "_close"];
    let content = $._attached_mod_content;
    let free_form_content = $._attached_mod_content;
    let precedence = 2;

    if (verbatim) {
        content = $._verbatim_paragraph_content;
        free_form_content = $._free_form_verbatim_mod_content;
        precedence = 5;
    }
    return choice(
        prec.dynamic(precedence + 2, seq(
            free_open,
            prec.dynamic(precedence + 1, free_form_content),
            free_close,
        )),
        prec.dynamic(precedence, seq(
            open,
            content,
            close,
        ))
    )
}
