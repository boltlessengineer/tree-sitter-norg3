let newline = choice("\n", "\r", "\r\n");
let newline_or_eof = choice("\n", "\r", "\r\n", "\0");
// !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

module.exports = grammar({
    name: "norg",

    extras: $ => [],
    externals: ($) => [
        $.whitespace,
        $.word,

        $.bold_open,
        $.bold_close,
        $.italic_open,
        $.italic_close,
        $.strikethrough_open,
        $.strikethrough_close,
        $.underline_open,
        $.underline_close,
        $.spoiler_open,
        $.spoiler_close,
        $.superscript_open,
        $.superscript_close,
        $.subscript_open,
        $.subscript_close,
        $.inline_comment_open,
        $.inline_comment_close,
        $.verbatim_open,
        $.verbatim_close,
        $.inline_math_open,
        $.inline_math_close,
        $.inline_macro_open,
        $.inline_macro_close,

        $.free_form_mod_open,
        $.free_form_mod_close,
        $.link_modifier,
        $.escape_sequence,

        $._punc_ch,
        $._soft_break,
        $.para_break,

        $.error_sentinel,
    ],
    conflicts: ($) => [
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
        document: ($) => repeat(
            $.non_structural,
        ),
        non_structural: $ => prec.right(1, choice(
            $.paragraph,
            alias($.para_break, "_para_break"),
        )),
        paragraph: ($) => prec.right(0, seq(
            repeat1(
                choice(
                    $.paragraph_segment,
                    $._soft_break,
                )
            ),
        )),
        paragraph_segment: $ =>
            prec.right(0, 
                repeat1(choice(
                    $._paragraph_element,
                ))
            ),
        _paragraph_element: $ => choice(
            $.escape_sequence,
            alias($.word, "_word"),
            alias($.whitespace, "_whitespace"),
            alias($.punc, "_word"),
            alias($.mod_conflict, "_word"),
            // $.punc,
            // $.mod_conflict,
            $._attached_modifier,
        ),
        punc: $ => choice(
            $._punc_ch,
            ".",
            ",",
            "<",
            ">",
            "(",
            ")",
            "\\",
            ":",
            "|",
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
                        alias($.whitespace, "_whitespace"),
                        alias($.punc, "_word"),
                        alias($.mod_conflict, "_word"),
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
                            alias($.whitespace, "_whitespace"),
                            alias($.punc, "_word"),
                            alias($.mod_conflict, "_word"),
                            // alias($.free_form_mod_close, "_word"),
                        ),
                        $._soft_break,
                        prec(1, alias($.escape_sequence, "_word")),
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
                $.link_modifier,
                $.free_form_mod_open,
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
    // let open = $[kind + "_open"];
    // let close = $[kind + "_close"];
    let content = $._attached_mod_content;
    let free_form_content = $._attached_mod_content;

    if (verbatim) {
        content = $._verbatim_paragraph_content;
        // TODO: implement verbatim free form
        free_form_content = $._free_form_verbatim_mod_content;
    }
    return choice(
        prec.dynamic(2, seq(
            open,
            alias($.free_form_mod_open, $.free_form_open),
            free_form_content,
            alias($.free_form_mod_close, $.free_form_close),
            close,
        )),
        seq(
            open,
            content,
            close,
        )
    )
}
