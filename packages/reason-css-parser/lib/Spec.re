open Standard;
open Combinator;
open Modifier;
open Rule.Match;
open Parser_helper;

let rec property_test = [%value.rec "'static' | 'absolute' | <color>"] 