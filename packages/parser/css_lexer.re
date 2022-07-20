/** CSS lexer.
  * Reference:
  * https://www.w3.org/TR/css-syntax-3/
  * https://github.com/yahoo/css-js/blob/master/src/l/css.3.l */

module Sedlexing = Lex_buffer;
module Parser = Css_parser;
module Types = Css_types;

/** Signals a lexing error at the provided source location. */
exception LexingError((Lexing.position, string));

/** Signals a parsing error at the provided token and its start and end
 * locations. */
exception ParseError((Parser.token, Lexing.position, Lexing.position));

/** Signals a grammar error at the provided location. */
exception GrammarError((string, Location.t));

let position_to_string = pos =>
  Printf.sprintf(
    "[%d,%d+%d]",
    pos.Lexing.pos_lnum,
    pos.Lexing.pos_bol,
    pos.Lexing.pos_cnum - pos.Lexing.pos_bol,
  );

let location_to_string = loc =>
  Printf.sprintf(
    "%s..%s",
    position_to_string(loc.Location.loc_start),
    position_to_string(loc.Location.loc_end),
  );

let token_to_string =
  fun
  | Parser.EOF => ""
  | Parser.LEFT_BRACE => "{"
  | Parser.RIGHT_BRACE => "}"
  | Parser.LEFT_PAREN => "("
  | Parser.RIGHT_PAREN => ")"
  | Parser.LEFT_BRACKET => "["
  | Parser.RIGHT_BRACKET => "]"
  | Parser.COLON => ":"
  | Parser.DOUBLE_COLON => "::"
  | Parser.SEMI_COLON => ";"
  | Parser.PERCENTAGE => "%"
  | Parser.AMPERSAND => "&"
  | Parser.IMPORTANT => "!important"
  | Parser.IDENT(s) => s
  | Parser.STRING(s) => "'" ++ s ++ "'"
  | Parser.URI(s) => s
  | Parser.OPERATOR(s) => s
  | Parser.COMBINATOR(s)
  | Parser.DELIM(s) => s
  | Parser.AT_MEDIA(s)
  | Parser.AT_KEYFRAMES(s)
  | Parser.AT_RULE_STATEMENT(s)
  | Parser.AT_RULE(s) => "@" ++ s
  | Parser.HASH(s) => "#" ++ s
  | Parser.NUMBER(s) => s
  | Parser.UNICODE_RANGE(s) => s
  | Parser.FLOAT_DIMENSION((n, s, _)) => n ++ s
  | Parser.DIMENSION((n, d)) => n ++ "." ++ d
  | Parser.VARIABLE(v) => String.concat(".", v)
  | Parser.WS => " "
  | Parser.WS_DOUBLE_COLON => " ::"
  | Parser.WS_COLON => " :"
  | Parser.WS_HASH(h) => " #" ++ h
  | Parser.DOT => "."
  | Parser.COMMA => ","
;

let token_to_debug =
  fun
  | Parser.EOF => "EOF"
  | Parser.LEFT_BRACE => "LEFT_BRACE"
  | Parser.RIGHT_BRACE => "RIGHT_BRACE"
  | Parser.LEFT_PAREN => "LEFT_PAREN"
  | Parser.RIGHT_PAREN => "RIGHT_PAREN"
  | Parser.LEFT_BRACKET => "LEFT_BRACKET"
  | Parser.RIGHT_BRACKET => "RIGHT_BRACKET"
  | Parser.COLON => "COLON"
  | Parser.DOUBLE_COLON => "DOUBLE_COLON"
  | Parser.SEMI_COLON => "SEMI_COLON"
  | Parser.PERCENTAGE => "PERCENTAGE"
  | Parser.AMPERSAND => "AMPERSAND"
  | Parser.IMPORTANT => "IMPORTANT"
  | Parser.IDENT(s) => "IDENT('" ++ s ++ "')"
  | Parser.STRING(s) => "STRING('" ++ s ++ "')"
  | Parser.URI(s) => "URI('" ++ s ++ "')"
  | Parser.OPERATOR(s) => "OPERATOR('" ++ s ++ "')"
  | Parser.DELIM(s) => "DELIM('" ++ s ++ "')"
  | Parser.AT_RULE(s) => "AT_RULE('" ++ s ++ "')"
  | Parser.AT_RULE_STATEMENT(s) => "AT_RULE_STATEMENT('" ++ s ++ "')"
  | Parser.AT_MEDIA(s) => "AT_MEDIA('" ++ s ++ "')"
  | Parser.AT_KEYFRAMES(s) => "AT_KEYFRAMES('" ++ s ++ "')"
  | Parser.HASH(s) => "HASH('" ++ s ++ "')"
  | Parser.NUMBER(s) => "NUMBER('" ++ s ++ "')"
  | Parser.UNICODE_RANGE(s) => "UNICODE_RANGE('" ++ s ++ "')"
  | Parser.FLOAT_DIMENSION((n, s, _d)) =>
    "FLOAT_DIMENSION('" ++ n
    ++ ", "
    ++ s
    ++ "')"
  | Parser.DIMENSION((n, d)) => "DIMENSION('" ++ n ++ ", " ++ d ++ "')"
  | Parser.VARIABLE(v) => "VARIABLE('" ++ (String.concat(".", v)) ++ "')"
  | Parser.COMBINATOR(s) => "COMBINATOR(" ++ s ++ ")"
  | Parser.DOT => "DOT"
  | Parser.COMMA => "COMMA"
  | Parser.WS => "WS"
  | Parser.WS_DOUBLE_COLON => "WS_DOUBLE_COLON"
  | Parser.WS_COLON => "WS_COLON"
  | Parser.WS_HASH(h) => "WS_HASH(" ++ h ++ ")"
;

let () =
  Location.register_error_of_exn(
    fun
    | LexingError((pos, msg)) => {
        let loc = Sedlexing.make_loc(pos, pos);
        Some(Location.error(~loc, msg));
      }
    | ParseError((token, start_pos, end_pos)) => {
        let loc = Sedlexing.make_loc(start_pos, end_pos);
        let msg =
          Printf.sprintf(
            "Parse error while reading token '%s'",
            token_to_string(token),
          );
        Some(Location.error(~loc, msg));
      }
    | GrammarError((msg, loc)) => Some(Location.error(~loc, msg))
    | _exn => Some(Location.error("Unexpected error"))
  );

/* Regexes */
let newline = [%sedlex.regexp? '\n' | "\r\n" | '\r' | '\012'];

let whitespace = [%sedlex.regexp? " " | '\t' | newline];

let whitespaces = [%sedlex.regexp? Star(whitespace)];

let hex_digit = [%sedlex.regexp? '0' .. '9' | 'a' .. 'f' | 'A' .. 'F'];

let digit = [%sedlex.regexp? '0' .. '9'];

let non_ascii = [%sedlex.regexp? '\160' .. '\255'];

let up_to_6_hex_digits = [%sedlex.regexp? Rep(hex_digit, 1 .. 6)];

let unicode = [%sedlex.regexp? ('\\', up_to_6_hex_digits, Opt(whitespace))];

let unicode_range = [%sedlex.regexp?
  Rep(hex_digit | '?', 1 .. 6) |
  (up_to_6_hex_digits, '-', up_to_6_hex_digits)
];

let escape = [%sedlex.regexp?
  unicode | ('\\', Compl('\r' | '\n' | '\012' | hex_digit))
];

let ident_start = [%sedlex.regexp?
  '_' | 'a' .. 'z' | 'A' .. 'Z' | '$' | non_ascii | escape
];

let ident_char = [%sedlex.regexp?
  '_' | 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '-' | non_ascii | escape
];

let ident = [%sedlex.regexp? (Opt('-'), ident_start, Star(ident_char))];

let variable_name = [%sedlex.regexp? (Star(ident_char))];
let module_variable = [%sedlex.regexp? (variable_name, '.')];
let variable = [%sedlex.regexp? ('$', '(', Opt(Star(module_variable)), variable_name, ')')];

let string_quote = [%sedlex.regexp?
  (
    '"',
    Star(Compl('\n' | '\r' | '\012' | '"') | ('\\', newline) | escape),
    '"',
  )
];

let string_apos = [%sedlex.regexp?
  (
    '\'',
    Star(Compl('\n' | '\r' | '\012' | '\'') | ('\\', newline) | escape),
    '\'',
  )
];

let string = [%sedlex.regexp? string_quote | string_apos];

let name = [%sedlex.regexp? Plus(ident_char)];

let number = [%sedlex.regexp?
  (
    Opt('-'),
    Plus(digit),
    Opt('.', Plus(digit)),
    Opt('e' | 'E', '+' | '-', Plus(digit)),
  ) |
  (Opt('-'), '.', Plus(digit), Opt('e' | 'E', '+' | '-', Plus(digit)))
];

let non_printable = [%sedlex.regexp?
  '\000' .. '\b' | '\011' | '\014' .. '\031' | '\127'
];

let operator = [%sedlex.regexp? "~=" | "|=" | "^=" | "$=" | "*="| "="];

let combinator = [%sedlex.regexp? '>' | '+' | '~' | "||"];

let at_rule_without_body = [%sedlex.regexp? ("@", "charset" | "import" | "namespace")];
let at_rule = [%sedlex.regexp? ("@", ident)];
let at_media = [%sedlex.regexp? ("@", "media")];
let at_keyframes = [%sedlex.regexp? ("@", "keyframes")];

let _a = [%sedlex.regexp? 'A' | 'a'];
let _b = [%sedlex.regexp? 'B' | 'b'];
let _c = [%sedlex.regexp? 'C' | 'c'];
let _d = [%sedlex.regexp? 'D' | 'd'];
let _e = [%sedlex.regexp? 'E' | 'e'];
let _f = [%sedlex.regexp? 'F' | 'f'];
let _g = [%sedlex.regexp? 'G' | 'g'];
let _h = [%sedlex.regexp? 'H' | 'h'];
let _i = [%sedlex.regexp? 'I' | 'i'];
let _j = [%sedlex.regexp? 'J' | 'j'];
let _k = [%sedlex.regexp? 'K' | 'k'];
let _l = [%sedlex.regexp? 'L' | 'l'];
let _m = [%sedlex.regexp? 'M' | 'm'];
let _n = [%sedlex.regexp? 'N' | 'n'];
let _o = [%sedlex.regexp? 'O' | 'o'];
let _p = [%sedlex.regexp? 'P' | 'p'];
let _q = [%sedlex.regexp? 'Q' | 'q'];
let _r = [%sedlex.regexp? 'R' | 'r'];
let _s = [%sedlex.regexp? 'S' | 's'];
let _t = [%sedlex.regexp? 'T' | 't'];
let _u = [%sedlex.regexp? 'U' | 'u'];
let _v = [%sedlex.regexp? 'V' | 'v'];
let _w = [%sedlex.regexp? 'W' | 'w'];
let _x = [%sedlex.regexp? 'X' | 'x'];
let _y = [%sedlex.regexp? 'Y' | 'y'];
let _z = [%sedlex.regexp? 'Z' | 'z'];

let important = [%sedlex.regexp?
  ("!", whitespaces, _i, _m, _p, _o, _r, _t, _a, _n, _t)
];

let length = [%sedlex.regexp?
  (_c, _a, _p) |
  (_c, _h) |
  (_e, _m) |
  (_e, _x) |
  (_i, _c) |
  (_l, _h) |
  (_r, _e, _m) |
  (_r, _l, _h) |
  (_v, _h) |
  (_v, _w) |
  (_v, _i) |
  (_v, _b) |
  (_v, _m, _i, _n) |
  (_v, _m, _a, _x) |
  (_c, _m) |
  (_m, _m) |
  _q |
  (_i, _n) |
  (_p, _c) |
  (_p, _t) |
  (_p, _x) |
  (_f, _r)
];

let angle = [%sedlex.regexp?
  (_d, _e, _g) | (_g, _r, _a, _d) | (_r, _a, _d) | (_t, _u, _r, _n)
];

let time = [%sedlex.regexp? _s | (_m, _s)];

let frequency = [%sedlex.regexp? (_h, _z) | (_k, _h, _z)];

let ws_colon = [%sedlex.regexp? (whitespaces, ':')];
let ws_double_colon = [%sedlex.regexp? (whitespaces, "::")];
let ws_hash = [%sedlex.regexp? (whitespaces, '#', name)];
let hash = [%sedlex.regexp? ('#', name)];

let replace = (input, output) =>
  Str.global_replace(Str.regexp_string(input), output);

let eat_ws_hash = h => {
  h |> String.trim |> replace("#", "");
};

let rec get_next_token = (buf) => {
  open Css_parser;
  switch%sedlex (buf) {
  | eof => EOF
  | "/*" => discard_comments(buf)
  | '.' => DOT
  | ';' => SEMI_COLON
  | '}' => RIGHT_BRACE
  | '{' => LEFT_BRACE
  | "::" => DOUBLE_COLON
  | ':' => COLON
  | ws_double_colon => WS_DOUBLE_COLON
  | ws_colon => WS_COLON
  | '(' => LEFT_PAREN
  | ')' => RIGHT_PAREN
  | '[' => LEFT_BRACKET
  | ']' => RIGHT_BRACKET
  | '%' => PERCENTAGE
  | '&' => AMPERSAND
  | ',' => COMMA
  | variable => VARIABLE(Sedlexing.latin1(~skip=2, ~drop=1, buf) |> String.split_on_char('.'))
  | operator => OPERATOR(Sedlexing.latin1(buf))
  | combinator => COMBINATOR(Sedlexing.latin1(buf))
  | string => STRING(Sedlexing.latin1(~skip=1, ~drop=1, buf))
  | important => IMPORTANT
  | at_media => AT_MEDIA(Sedlexing.latin1(~skip=1, buf))
  | at_keyframes => AT_KEYFRAMES(Sedlexing.latin1(~skip=1, buf))
  | at_rule => AT_RULE(Sedlexing.latin1(~skip=1, buf))
  | at_rule_without_body => AT_RULE_STATEMENT(Sedlexing.latin1(~skip=1, buf))
  /* NOTE: should be placed above ident, otherwise pattern with
   * '-[0-9a-z]{1,6}' cannot be matched */
  | (_u, '+', unicode_range) => UNICODE_RANGE(Sedlexing.latin1(buf))
  | ident => IDENT(Sedlexing.latin1(buf))
  | ws_hash => WS_HASH(eat_ws_hash(Sedlexing.latin1(~skip=2, buf)))
  | hash => HASH(Sedlexing.latin1(~skip=1, buf))
  | whitespaces => WS
  | number => get_dimension(Sedlexing.latin1(buf), buf)
  | any => DELIM(Sedlexing.latin1(buf))
  | _ => assert(false)
  };
}
and get_dimension = (n, buf) =>
  switch%sedlex (buf) {
  | length => FLOAT_DIMENSION((n, Sedlexing.latin1(buf), Length))
  | angle => FLOAT_DIMENSION((n, Sedlexing.latin1(buf), Angle))
  | time => FLOAT_DIMENSION((n, Sedlexing.latin1(buf), Time))
  | frequency => FLOAT_DIMENSION((n, Sedlexing.latin1(buf), Frequency))
  | ident => DIMENSION((n, Sedlexing.latin1(buf)))
  | _ => NUMBER(n)
}
and discard_comments = buf => {
  switch%sedlex (buf) {
    | eof => raise(LexingError((buf.Sedlexing.pos, "Unterminated comment at the end of the string")))
    | "*/" => get_next_token(buf)
    | _ => discard_comments(buf)
  };
};

let get_next_token_with_location = buf => {
  let loc_start = Sedlexing.next_loc(buf);
  let token = get_next_token(buf);
  let loc_end = Sedlexing.next_loc(buf);
  (token, loc_start, loc_end);
};

let parse = (buf, parser) => {
  let last_token = ref((Parser.EOF, Lexing.dummy_pos, Lexing.dummy_pos));
  let next_token = () => {
    last_token := get_next_token_with_location(buf);
    last_token^;
  };

  try(MenhirLib.Convert.Simplified.traditional2revised(parser, next_token)) {
  | LexingError(_) as e => raise(e)
  | _ => raise(ParseError(last_token^))
  };
};

let parse_string = (~container_lnum=?, ~pos=?, parser, string) => {
  switch (container_lnum) {
  | None => ()
  | Some(lnum) => Sedlexing.container_lnum_ref := lnum
  };
  parse(Sedlexing.of_ascii_string(~pos?, string), parser);
};

let parse_declaration_list = (~container_lnum=?, ~pos=?, css) =>
  parse_string(~container_lnum?, ~pos?, Parser.declaration_list, css);

let parse_declaration = (~container_lnum=?, ~pos=?, css) =>
  parse_string(~container_lnum?, ~pos?, Parser.declaration, css);

let parse_stylesheet = (~container_lnum=?, ~pos=?, css) =>
  parse_string(~container_lnum?, ~pos?, Parser.stylesheet, css);
