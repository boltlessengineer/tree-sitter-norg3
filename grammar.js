let newline = choice("\n", "\r", "\r\n");
let newline_or_eof = choice("\n", "\r", "\r\n", "\0");

module.exports = grammar({
    name: "norg",

    extras: $ => [],
    externals: ($) => [
        $.whitespace,
        $.word,

        $._bold_open_ch,
        $._bold_close_ch,
        $._italic_open_ch,
        $._italic_close_ch,
        $._verbatim_open_ch,
        $._verbatim_close_ch,

        $.link_modifier,
        $.escape_sequence,

        $.punc,

        $.error_sentinel,
    ],
    conflicts: ($) => [
        [$.bold, $.mod_conflict],
        [$.italic, $.mod_conflict],
        [$.verbatim, $.mod_conflict],
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
            newline_or_eof,
        )),
        paragraph: ($) => prec.right(0, seq(
            repeat1(
                choice(
                    $.paragraph_segment,
                    newline,
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
            $._attached_modifier,
            alias($.mod_conflict, "_word"),
            alias($.punc, "_word"),
        ),
        _attached_modifier: $ =>
            seq(
                optional($.link_modifier),
                choice(
                    $.bold,
                    $.italic,
                    $.verbatim
                ),
                optional($.link_modifier),
            ),
        mod_conflict: $ =>
            choice(
                $._bold_open_ch,
                $._italic_open_ch,
                $._verbatim_open_ch,
                $.link_modifier,
            ),
        bold: $ =>
            seq(
                $._bold_open_ch,
                prec.right(0, 
                    repeat1(choice(
                        $._paragraph_element,
                    )),
                ),
                $._bold_close_ch,
            ),
        italic: $ =>
            seq(
                $._italic_open_ch,
                prec.right(0, 
                    repeat1(choice(
                        $._paragraph_element,
                    )),
                ),
                $._italic_close_ch,
            ),
        verbatim: $ =>
            seq(
                $._verbatim_open_ch,
                prec.right(0,
                    repeat1(
                        alias(choice(
                            $.word,
                            $.whitespace,
                            $.escape_sequence,
                            $.punc,
                            $.mod_conflict
                        ), "_word")
                    ),
                ),
                $._verbatim_close_ch,
            )
    },
});
