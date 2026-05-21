# RPAL Interpreter — CS 3513 Programming Languages

An implementation of a lexical analyzer, parser, standardizer, and CSE machine
for the **RPAL** language, written in **C++** (no `lex`/`yacc` used). The program
reads an RPAL source file, builds its Abstract Syntax Tree (AST), converts it to
a Standardized Tree (ST), and evaluates it on a CSE (Control–Stack–Environment)
machine, producing output that matches `rpal.exe`.

## Building

A `Makefile` is provided directly under the project root.

```
make            # builds the executable ./rpal20
make clean      # removes object files and the executable
```

The build requires a C++17 compiler (`g++`).

## Running

```
./rpal20 file_name          # evaluate the program (CSE machine output)
./rpal20 -ast file_name      # print the Abstract Syntax Tree
./rpal20 -st  file_name      # print the Standardized Tree
```

`file_name` is a file containing an RPAL program.

### Example

Input file `rpal_test_programs/rpal_01`:

```
let Sum(A) = Psum (A,Order A )
where rec Psum (T,N) = N eq 0 -> 0
 | Psum(T,N-1)+T N
in Print ( Sum (1,2,3,4,5) )
```

Run:

```
./rpal20 rpal_test_programs/rpal_01
```

Output:

```
15
```

## Project Structure

```
.
├── Makefile                 # build script (produces ./rpal20)
├── rpal20.cpp               # main entry point and CLI handling
├── src/
│   ├── token.h              # token type definitions
│   ├── lexer.h / lexer.cpp        # lexical analyzer (RPAL_Lex.pdf)
│   ├── node.h  / node.cpp         # AST/ST node + pre-order printer
│   ├── parser.h / parser.cpp      # recursive-descent parser (RPAL_Grammar.pdf)
│   ├── standardizer.h / .cpp      # AST -> Standardized Tree rewrite rules
│   └── cse_machine.h / .cpp       # control-structure generation + CSE machine
├── rpal_test_programs/      # sample programs and expected outputs
└── REPORT.pdf               # project report
```

## Pipeline

1. **Lexical analysis** — `Lexer` scans the source into tokens following the
   RPAL lexicon (identifiers, integers, strings, operators, punctuation;
   whitespace and comments are discarded).
2. **Parsing** — `Parser` is a hand-written recursive-descent parser that
   implements the full RPAL phrase-structure grammar and builds the AST using a
   node stack and the standard build-tree operation.
3. **Standardization** — `Standardizer` rewrites the AST into the Standardized
   Tree by applying the canonical rules for `let`, `where`, `fcn_form`,
   multi-parameter `lambda`, `within`, `@`, `and`, and `rec`.
4. **CSE machine** — `CSEMachine` flattens the ST into control structures
   (delta_0 … delta_n) and evaluates them with the 13 CSE rules, including the
   `Y*`/eta rule for recursion. Built-in functions (`Print`, `Order`, `Stem`,
   `Stern`, `Conc`, `Isinteger`, `Istruthvalue`, `Isstring`, `Istuple`,
   `Isfunction`, `Isdummy`, `Null`, `ItoS`) are supported.

## Authors

- Student 1 — Name, Index Number
- Student 2 — Name, Index Number

(Replace the placeholders above with the actual group members' details before
submission.)
