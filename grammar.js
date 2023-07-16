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
        $.__in_bold,
        $._italic_open_ch,
        $._italic_close_ch,
        $.__in_italic,
        $._verbatim_open_ch,
        $._verbatim_close_ch,
        $.__in_verbatim,

        $.link_modifier,

        $.punc,

        $.error_sentinel,
    ],
    conflicts: ($) => [
        [$.bold, $.mod_conflict],
        [$.italic, $.mod_conflict],
        [$.verbatim, $.mod_conflict],
        [$.paragraph_segment],
        [$._paragraph_element],
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
            $.word,
            $.whitespace,
            seq(
                optional($.link_modifier),
                $._attached_modifier,
                optional($.link_modifier),
            ),
            $.mod_conflict,
            $.punc,
        ),
        _attached_modifier: $ =>
            choice(
                $.bold,
                $.italic,
                $.verbatim
            ),
        mod_conflict: $ =>
            choice(
                $._bold_open_ch,
                $._italic_open_ch,
                $._verbatim_open_ch,
            ),
        bold: $ =>
            seq(
                $._bold_open_ch,
                prec.right(0, 
                    repeat1(choice(
                        $._paragraph_element,
                        $.__in_bold,
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
                        $.__in_italic,
                    )),
                ),
                $._italic_close_ch,
            ),
        verbatim: $ =>
            prec.dynamic(5,
                seq(
                    $._verbatim_open_ch,
                    prec.right(0,
                        repeat1(
                            choice(
                                $.word,
                                $.whitespace,
                                $.punc,
                                $.mod_conflict
                            )
                        ),
                    ),
                    $._verbatim_close_ch,
                )
            )
    },
});
