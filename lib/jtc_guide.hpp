#include <iostream>

void guide_wp(void);
void guide_un(void);
void guide_ex(void);

int print_guide(void) {
 guide_wp();
 cout << endl;

 guide_un();
 cout << endl;

 guide_ex();
 cout << endl;

 return 0;
}

void guide_wp(void) {
 cout << R"(

  * Walk-path syntax:

Walk-paths made of various lexemes that instruct how to traverse JSON allow selecting a single or
multiple elements from the input JSON

There are only 2 types of lexemes (though there're a few variants of each type):
 a) Offset lexemes - provide addressing/subscripting JSON nodes
 b) Search lexemes - perform search down the JSON tree

a. Offset lexemes: enclosed into square braces '[', ']', their meaning depends on the content;
   following notations are possible: '[]', '[text]', '[n]', '[-n]', '[^n]', '[+n]', '[N:N]'
   - []: an empty offset, matches an empty label, like one in the following JSON:
     { "": "empty label example" }
   - [text]: the offset selects a child in the node by the label, e.g.: '[number]' selects element
     3.14 in the following JSON: { "number": 3.14, "false": true }
   - [n]: plain numerical offset selects indexed element within an iterable (an iterable is either
     a JSON array or an object), e.g.: '[1]' - selects a 2nd child of an array (or an object) node
     (all indices and quantifiers in walk-path are zero based); in the above JSON, element "false"
     will be selected (i.e. value true)
   - [-n]: a negative numerical offset, backs off n levels up the JSON tree (from given position);
     e.g.: a path like '[0][0][-2]' descends 2 levels down the JSON tree and then selects back a
     root node (i.e. ascends 2 levels/tiers up)
   - [^n] numerical offsets with preceding circumflex do a similar thing as negative offsets, but
     instead descend from the root; e.g.: following walk-paths' results are equal: '[0][1]',
     '[0][1][2][-1]', '[0][1][2][^2]'; n cannot go negative in this type of offset, though
     preceding '+' is allowed (e.g. '[^+3]');
   - [+n]: a positive numerical offset makes the path iterable, e.g.: [+1] selects all immediate
     children within a given JSON node (array or object) staring from the 2nd child
   - [N:N]: a range offset (same notation like in python) - selects all elements in the specified
     range. e.g.: [-3:] selects last 3 elements among JSON iterable's children; like in python,
     the '+','-' signs as well as the numerical indices of a range are optional, thus all of the
     following notations are valid: '[:]', '[1:]', '[-2:]', '[+2:]', '[:3]', '[:-3]', '[+4:-5]'
   Some notations may duplicate each other, e.g.:
     [+0] and [:], or [+2] and [2:] have the same effect

b. search lexemes: enclosed into angular braces '<', '>', instruct to perform a textual search
   under given JSON tree point; following notation forms are possible:
   '<txt>', '<txt>S', '<txt>N', '<txt>SN'
   and the respective non-recursive search lexemes:
   '>txt<', '>txt<S', '>txt<N', '>txt<SN'
   where txt - is any text to search for, S - is an optional one letter suffix, N - is an optional
   quantifier, which comes in several variants
   - if a lexeme is given using '<...>' encasement, then a *recursive* search off the current JSON
     point is applied, otherwise (i.e. '>...<' encasement given) - a *non-recursive* search is
     performed among immediate JSON node's children only
   - '<a text>': performs a search of "a text" under a JSON tree off the given node among JSON
     strings values only, it's a default behavior, which could be altered with an optional suffix
   S: an optional one letter suffix, either of these: [rRlLdDbnaoij], each one altering the search
     in the following way:
     r: apply exact match (default, may be omitted) while searching JSON string values only
     R: same as r, but expression in braces is a Regex (regex search applied)
     l: apply exact match while searching object labels only
     L: same as l, but expression in braces is a Regex (regex search applied)
     d: match a number (i.e. searches numeric JSON values only)
     D: same as d, but expression in braces is a Regex (value is treated as a string value here)
     b: match a boolean (i.e. searching only boolean values), true/false/any must be fully spelled
        e.g.: '<true>b', '<any>b'
     n: match null values only, the content within the encasement could be anything and is ignored
        e.g.: '<>n', '>null<n', etc
     a: match any atomic JSON value (string, numeric, boolean, null); the content within the
        encasements is ignored
     o: match any object JSON value (i.e. '{...}'); the content within the encasement is ignored
     i: match any array (iterable) JSON value (i.e. '[...]'); the content within the encasement is
        ignored
     j: match user specific JSON value, the content within the encasement should be a valid JSON
        value, e.g.: '<[]>j' - will find the first empty JSON array
     w: wide range match - match any JSON value (atomic, objects, arrays)
     e: end-node match (matches leaves only) - match any of: atomic, {}, []
   N: an integer quantifier specifying search match instance/range, comes in following variants
      n - a number (index), e.g. '<a text>3' - matches 4th encounter of a string "a text" within
        the JSON tree (off a given search point); quantifiers, as well as numerical subscripts are
        zero based
      +n - collects all matched encounters staring from index n, e.g.: '<a text>+2' will match 3rd
        encounter of "a text", 4th, 5th and so on until all matches found
      +n:+n - once ':' is sighted within a quantifier, then it specifies a range; the range
        quantifiers follow the same notation as offset's range with just one exception - the
        indices here cannot go negative

   - empty search lexemes '<>', '><' may only have either of suffixes: [rlna] - those may match
     empty JSON elements (or do not require the lexeme), while others require non-empty lexemes

Both lexeme types allow specifying inner brackets, however, the closing one has to be quoted with
the preceding backslash, e.g.: '[case[0\]]' - specifies an offset lexeme to the label "case[0]";
'<tag<a\>>' - denotes a search lexeme for the text "tag<a>"

Spaces *between* lexemes are allowed and ignored; for the offset lexemes even a more generic rule
is in action: if an offset lexeme violates any of the described above rules and does not represent
a valid offset, then it's treated as a textual type of offset; e.g.: '[ 1]' and '[1 ]' are in fact
respective textual offsets " 1" and "1 " rather than a numerical offset 1; '[^-3]', '[+-2]', are
also examples of textual offsets

There're cases when JSON values have to be searched where they attached only to a specific label,
a following lexeme syntax facilitates such case: '[a label]:<some text>' - in that example
"some text" will be searched (and matched) only among JSON values which are attached to the label
"a label"; in that notation all suffixes (and quantifiers) are allowed except 'l' and 'L', e.g.:
'[age]:<25>j:' matches all JSON numerical values 25, where it's attached to the label "age" only
)";
}



void guide_un(void) {
 cout << R"(

  * Usage notes:

multiple -)" STR(OPT_WLK) R"( usage:
 - if switch -)" STR(OPT_SEQ)
   R"( is given, then all walk paths will be processed sequentially, otherwise, results
   would be grouped by relevance and walks interleaved; the order of provided walks will be (if
   can be) honored

options -)" STR(OPT_JSN) R"( and -)" STR(OPT_LBL) R"( usage:
 - when -)" STR(OPT_JSN) R"( is given w/o -)" STR(OPT_LBL)
   R"(, then all walked elements will be collected into a JSON array; when
   use together, all walked elements will be grouped into *relevant* objects within a parent array
   options -)" STR(OPT_JSN) R"(, -)" STR(OPT_LBL)
   R"( only modify the output of walked items, i.e. they will ignored when either of
   -i, -u, -s, -p is given

mutually exclusive options:
 - options -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD) R"(, -)" STR(OPT_SWP) R"(, -)" STR(OPT_PRG)
   R"( normally are mutually exclusive; if found together, only one type will
   be executed (selected in the priority of the listed order); though there's one case usage of
   -)" STR(OPT_PRG) R"( together with options -i, -u, see notes below

options -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD) R"( usage with -)" STR(OPT_EXE) R"(:
 - the options accept one parameter which may be treated differently: initially a file name
   assumed; if the file is not found, then the parameter is treated as a JSON and its parsing is
   attempted; if parsing fails then a walk-path is assumed and if it fails an exception is thrown
 - when used together with  option -)" STR(OPT_EXE)
   R"(, the latter must precede option -)" STR(OPT_INS) R"( or option -)" STR(OPT_UPD) R"(; every
   occurrence of ')" INTRP_STR R"(', or ')" INTRP_NKD
   R"(' is interpolated with walked JSON entry using a raw format; if
   interpolation results in a string value, then for every occurrence ')" INTRP_NKD
   R"(' the outside quotation
   marks are dropped; the interpolated entry is completely escaped, thus does not require any
   additional quoting; all shell-specific chars (e.g.: '|', ';', '\"', etc) have to be quoted or
   escaped; terminate the cli with trailing semicolon (which has to be escaped): \;

option -)" STR(OPT_MDF) R"( usage with -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD) R"(:
 - option -)" STR(OPT_MDF) R"( modifies behaviors of the these options:
   for -)" STR(OPT_INS)
           R"(: toggles "merge" semantic; by default -)" STR(OPT_INS)
           R"( can insert its parameter only into iterables,
           while with -)" STR(OPT_MDF)
           R"(, the parameter is merged with the destined element(s), which could be any
           JSON type
   for -)" STR(OPT_UPD)
           R"(: toggles "merge" semantic instead of overwrite when updating;
 - due to a variety of combinations of sources -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD)
   R"( and destination -)" STR(OPT_WLK) R"(, the number of various
   operation possibilities is big, therefore it's best to track is in the following table:
 * insert operation (-)" STR(OPT_INS) R"() without merge:
   to \ from  |        [3,4]        |     {"a":3,"c":4}     |      "a":3,"c":4      |     3
 -------------+---------------------+-----------------------+-----------------------+-------------
    [1,2]     |      [1,2,[3,4]     |  [1,2,{"a":3,"c":4}]  | [1,2,{"a":3},{"c":4}] |  [1,2,3]
 {"a":1,"b":2}|    {"a":1,"b":2}    |  {"a":1,"b":2,"c":4}  |  {"a":1,"b":2,"c":4}  |{"a":1,"b":2}
     "a"      |         "a"         |          "a"          |          "a"          |    "a"
 * insert (-)" STR(OPT_INS) R"() with the merge (-)" STR(OPT_MDF) R"():
   to \ from  |        [3,4]        |     {"a":3,"c":4}     |      "a":3,"c":4      |     3
 -------------+---------------------+-----------------------+-----------------------+-------------
    [1,2]     |      [1,2,3,4]      |       [1,2,3,4]       |       [1,2,3,4]       |  [1,2,3]
 {"a":1,"b":2}|{"a":[1,3],"b":[2,4]}|{"a":[1,3],"b":2,"c":4}|{"a":[1,3],"b":2,"c":4}|{"a":1,"b":2}
     "a"      |      ["a",3,4]      |       ["a",3,4]       |       ["a",3,4]       |  ["a",3]
 * update operation (-)" STR(OPT_UPD) R"() without merge:
   to \ from  |        [3,4]        |     {"a":3,"c":4}     |         "a":3         |     3
--------------+---------------------+-----------------------+-----------------------+-------------
    [1,2]     |        [3,4]        |     {"a":3,"c":4}     |           3           |     3
 {"a":1,"b":2}|        [3,4]        |     {"a":3,"c":4}     |           3           |     3
     "a"      |        [3,4]        |     {"a":3,"c":4}     |           3           |     3
 * update (-)" STR(OPT_UPD) R"() with the merge (-)" STR(OPT_MDF) R"():
   to \ from  |        [3,4]        |     {"a":3,"c":4}     |         "a":3         |     3
--------------+---------------------+-----------------------+-----------------------+-------------
    [1,2]     |        [3,4]        |         [3,4]         |         [3,2]         |   [3,2]
 {"a":1,"b":2}|    {"a":3,"b":4}    |  {"a":3,"b":2,"c":4}  |     {"a":3,"b":2}     |{"a":3,"b":2}
     "a"      |        [3,4]        |     {"a":3,"c":4}     |         {"a":3}       |     3

option -)" STR(OPT_PRG) R"( usage with -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD) R"(:
  - if option -)" STR(OPT_PRG) R"( is sighted together with -)" STR(OPT_INS) R"(, or -)"
    STR(OPT_UPD) R"( when the latter use a walk-path as a parameter
    then the insert/update operations effectively turn into *move*: the source entries (parameters
    of the -)" STR(OPT_INS) R"(, -)" STR(OPT_UPD)
    R"() will be purged at the end of operation; if -)" STR(OPT_PRG) STR(OPT_PRG)
    R"( is sighted, then everything else
    is removed besides the destination locations (-)" STR(OPT_WLK) R"()

options -)" STR(OPT_CMN) R"( and -)" STR(OPT_PRT) R"( usage:
 - these options should be given together: one -)" STR(OPT_CMN) R"( per any number of -)"
   STR(OPT_PRT) R"( options; each argument of
   -)" STR(OPT_PRT) R"( will be prepended with preceding -)" STR(OPT_CMN)
   R"(, together they will form an equivalent of -)" STR(OPT_WLK) R"(, e.g.:
   -)" STR(OPT_CMN) R"(1 -)" STR(OPT_PRT) R"(A -)" STR(OPT_PRT) R"(B -)" STR(OPT_CMN) R"(2 -)"
   STR(OPT_CMN) R"(3 -)" STR(OPT_PRT) R"(C is converted into: -)" STR(OPT_WLK) R"(1A -)"
   STR(OPT_WLK) R"(1B -)" STR(OPT_WLK) R"(2 -)" STR(OPT_WLK) R"(3C; there's no syntax validation
   in -)" STR(OPT_CMN) R"( and -)" STR(OPT_PRT)
   R"( until they converted into the respective -)" STR(OPT_WLK)
   R"( options, then walk validation occurs
)";
}



void guide_ex(void) {
 cout << R"(

  * Examples with explanations:

 Consider following source JSON (stored in the file example.json):
    {
        "Relation": [
            {
                "parent": "John Smith",
                "age": 31,
                "city": "New York",
                "children": [ "Sophia", "Olivia" ]
            },
            {
                "parent": "Anna Johnson",
                "age": 28,
                "city": "Chicago",
                "children": [ "John" ]
            }
        ]
    }


- to select node "children" for the first record, run:
    jtc -)" STR(OPT_WLK) R"( [Relation][0][children] example.json
    [
       "Sophia",
       "Olivia"
    ]


- to select all children records individually (for the first record), run:
    jtc -)" STR(OPT_WLK) R"( '[Relation] [0] [children] [+0]' example.json
    "Sophia"
    "Olivia"


- to select all children (for all records) and their parents as well, run:
    jtc -)" STR(OPT_WLK) R"('[Relation][+0][parent]' -)" STR(OPT_WLK)
    R"('[Relation][+0][children][+0]' example.json
    "John Smith"
    "Sophia"
    "Olivia"
    "Anna Johnson"
    "John"

the same could be achieved through a bit more succinct syntax:
    jtc -)" STR(OPT_CMN) R"('[Relation][+0]' -)" STR(OPT_PRT) R"([parent] -)" STR(OPT_PRT)
    R"( '[children][+0]' example.json
Here, all concatenations of an option -)" STR(OPT_CMN) R"( with every option -)" STR(OPT_PRT)
R"( is transformed into -)" STR(OPT_WLK) R"( then compiled
and executed. That syntax is supposed to save the input space when a common path is present in
multiple walks


- to select all children, from the node whose parent's name starts with "John", run:
    jtc -)" STR(OPT_WLK) R"( '<^John>R[-1][children][+0]' example.json
    "Sophia"
    "Olivia"

- however, the above walk syntax is prone to false positives, as it finds any string, starting
with "John"; to improve it, we have to ensure that the search is attached to the label, i.e.,
'"parent": "John..."':
    jtc -)" STR(OPT_WLK) R"( '[parent]:<^John>R[-1][children][+0]' example.json


- to add (insert) a child "James" to a parent whose name starts with "John" and reflect the
changes right into the source file, run:
    jtc -)" STR(OPT_FRC) STR(OPT_WLK) R"('[parent]:<^John>R[-1][children]' -)"
    STR(OPT_INS) R"('"James"' example.json; jtc example.json
    {
       "Relation": [
          {
             "age": 31,
             "children": [
                "Sophia",
                "Olivia",
                "James"
             ],
             "city": "New York",
             "parent": "John Smith"
          },
          {
             "age": 28,
             "children": [
                "John"
             ],
             "city": "Chicago",
             "parent": "Anna Johnson"
          }
       ]
    }


- to alter the parent's name from "John Smith" into "Jane Smith" run:
    jtc -)" STR(OPT_FRC) STR(OPT_WLK) R"( '<John Smith>' -)" STR(OPT_UPD)
    R"( '"Jane Smith"' example.json; jtc example.json
    {
       "Relation": [
          {
             "age": 31,
             "children": [
                "Sophia",
                "Olivia",
                "James"
             ],
             "city": "New York",
             "parent": "Jane Smith"
          },
          {
             "age": 28,
             "children": [
                "John"
             ],
             "city": "Chicago",
             "parent": "Anna Johnson"
          }
       ]
    }


- to add a new record:
    jtc -)" STR(OPT_FRC) STR(OPT_WLK) R"('[parent]:<Jane Smith> [-1]' -)"
    STR(OPT_INS) R"('{"Y-chromosome": true}' example.json; jtc example.json
    {
       "Relation": [
          {
             "Y-chromosome": true,
             "age": 31,
             "children": [
                "Sophia",
                "Olivia",
                "James"
             ],
             "city": "New York",
             "parent": "Jane Smith"
          },
          {
             "age": 28,
             "children": [
                "John"
             ],
             "city": "Chicago",
             "parent": "Anna Johnson"
          }
       ]
    }


- it's possible to wrap walked results back into JSON, with help of -)" STR(OPT_JSN) R"( option:
    jtc -)" STR(OPT_WLK) R"('[Relation][+0][parent]' -)" STR(OPT_JSN) R"( example.json
    [
       "Jane Smith",
       "Anna Johnson"
    ]


- if we throw in an option -)" STR(OPT_LBL)
R"(, then output JSON format ensures that entries with labels will be
grouped and displayed accordingly:
    jtc -)" STR(OPT_WLK) R"('[Relation][+0][parent]' -)" STR(OPT_JSN) STR(OPT_LBL) R"( example.json
    [
       {
          "parent": [
             "Jane Smith",
             "Anna Johnson"
          ]
       }
    ]


- an insert and update options could be subjected for a shell cli evaluation, e.g., say we want to
capitalize all parent names in our JSON:
    jtc -)" STR(OPT_WLK) R"('<parent>l+0' -)" STR(OPT_EXE) STR(OPT_UPD)
    R"( echo {} \| tr "[:lower:]" "[:upper:]" \; example.json
    {
       "Relation": [
          {
             "Y-chromosome": true,
             "age": 31,
             "children": [
                "Sophia",
                "Olivia",
                "James"
             ],
             "city": "New York",
             "parent": "JANE SMITH"
          },
          {
             "age": 28,
             "children": [
                "John"
             ],
             "city": "Chicago",
             "parent": "ANNA JOHNSON"
          }
       ]
    }
)";
}






















