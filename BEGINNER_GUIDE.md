# TWS (Tiny Workshop System) - Beginner's Guide

## 🎯 What is TWS?

TWS is a **complete compiler construction toolkit** that helps you learn how compilers work by building one for a simple programming language called **Tiny**.

Think of it like this:
- **Tiny** = A simple programming language (like a baby version of C or Pascal)
- **TWS** = A workshop with all the tools to build a compiler for Tiny

---

## 📚 What is a Compiler?

A compiler translates code you write into instructions a computer can execute.

**The Journey of Your Code:**
```
Your Tiny Program  →  [Scanner]  →  [Parser]  →  [Constrainer]  →  [Code Generator]  →  [Interpreter]  →  Output
   (text file)         (tokens)      (tree)      (check errors)    (machine code)      (run it)       (results)
```

---

## 🗂️ Folder Structure Explained

### **ROOT LEVEL: `/tws/`**

```
tws/
├── bin/           # Compiled tools (executables)
├── code/          # Shared library code used by all components
├── header/        # Header files (.h) for C code
├── doc/           # Documentation for flex and yacc
├── manual/        # Manuals for each component
├── parser/        # Generic parser library
├── pgen/          # Parser Generator (creates parsers from grammar)
├── interpreter/   # The Tiny interpreter (runs the compiled code)
└── tiny/          # YOUR WORK GOES HERE! The Tiny compiler
```

---

## 🔧 Main Components Explained

### **1. `/bin/` - Pre-built Tools**

Contains executable programs that you'll use:

- **pgen**: Parser Generator
  - Takes grammar rules (parse.tiny) → Generates parser code
  - Think: "Recipe book" → "Cooking instructions"

- **interpret**: The Interpreter
  - Executes the compiled Tiny programs
  - Think: "CPU simulator"

- **print_tree**: Debug tool
  - Shows the parse tree (structure of your program)

---

### **2. `/code/` - Shared Library**

Common code used by ALL parts of the compiler:

- **Tree.c**: Handles syntax trees (program structure)
- **Table.c**: Symbol tables (tracks variables)
- **Text.c**: String manipulation
- **Error.c**: Error reporting
- **Code.c**: Code generation helpers
- **Dcln.c**: Declaration management

**Think of this as:** The standard library that everyone uses.

---

### **3. `/header/` - Header Files**

C header files (.h) that define interfaces for the shared code:

- **Tree.h**: Tree data structures
- **Table.h**: Symbol table functions
- **Error.h**: Error handling
- **CodeGenerator.h**: Code generation interface
- etc.

**Think of this as:** The "instruction manual" for using the shared code.

---

### **4. `/pgen/` - Parser Generator**

This is a tool that **generates parsers automatically** from grammar rules.

**What it does:**
- Input: Grammar rules in special syntax (parse.tiny)
- Output: C code for a parser (code.y → yacc → parser code)

**Files:**
- **Tokenizer.l**: Scanner for grammar files
- **Parser.y**: Parser for grammar files
- **BuildTables.c, Flatten.c, CodeGen.c**: Build the parser

**You don't modify this** - it's a tool you USE.

---

### **5. `/interpreter/` - The Tiny Interpreter**

This is the "virtual machine" that runs compiled Tiny programs.

**What it does:**
- Takes compiled code (_CODE file)
- Executes instructions
- Manages memory, stack, input/output

**Key files:**
- **Machine.c**: The virtual CPU
- **Interpret.c**: Main interpreter loop
- **Interpreter.c**: Instruction execution

**You don't modify this** - it's the runtime environment.

---

### **6. `/tiny/` - YOUR WORKSPACE! 🎯**

This is where **YOU work** to build the Tiny compiler!

```
tiny/
├── parser/
│   ├── lex.tiny          ← YOU EDIT: Define tokens (keywords, operators)
│   └── parse.tiny        ← YOU EDIT: Define grammar (syntax rules)
├── Constrainer.c         ← YOU EDIT: Type checking & semantic analysis
├── CodeGenerator.c       ← YOU EDIT: Generate executable code
├── test-progs/           ← Test programs to verify your work
├── tc                    ← Script to run the complete compiler
└── Makefile             ← Build instructions
```

---

## 🔄 How the Tiny Compiler Works (Step-by-Step)

### **Step 1: Lexical Analysis (Scanner)**
**File:** `tiny/parser/lex.tiny`

**What it does:** Breaks your program into "tokens"

```
Input:  "x := 5 + 10;"
Output: [IDENTIFIER:"x"] [ASSIGNMENT] [INTEGER:"5"] [PLUS] [INTEGER:"10"] [SEMICOLON]
```

**Your job:** Define what tokens exist (keywords, operators, numbers, etc.)

---

### **Step 2: Syntax Analysis (Parser)**
**File:** `tiny/parser/parse.tiny`

**What it does:** Checks if tokens follow grammar rules and builds a tree

```
Input:  [x] [:=] [5] [+] [10]
Output: 
        assign
        /    \
       x      +
             / \
            5  10
```

**Your job:** Define the grammar rules (what's a valid statement, expression, etc.)

---

### **Step 3: Semantic Analysis (Constrainer)**
**File:** `tiny/Constrainer.c`

**What it does:** Checks for logical errors

- Are variables declared?
- Do types match? (can't add integer + boolean)
- Are operators used correctly?

```
Example Error: 
x := 5 + true;  ← ERROR: Can't add integer and boolean!
```

**Your job:** Write C code to check these rules

---

### **Step 4: Code Generation**
**File:** `tiny/CodeGenerator.c`

**What it does:** Converts the tree into machine instructions

```
Input Tree:    assign
              /    \
             x      +
                   / \
                  5  10

Output Code:   LIT 5      (push 5)
               LIT 10     (push 10)
               BOP BPLUS  (add them)
               SGV 0      (store in variable x)
```

**Your job:** Write C code to generate these instructions

---

### **Step 5: Interpretation (Execution)**
**File:** Built-in interpreter (you don't modify)

**What it does:** Runs the generated code

```
Input: Machine instructions
Output: Program results (printed numbers, etc.)
```

---

## 🎓 What You Did in Lab 3

You **extended** the Tiny compiler to support new features:

### **1. Modified `lex.tiny`**
Added new tokens:
- Keywords: `and`, `or`, `not`, `mod`, `true`, `false`, `eof`
- Operators: `**`, `<>`, `>=`, `=`, `<`, `>`, `*`, `/`

### **2. Modified `parse.tiny`**
Added grammar rules for:
- New operators with correct precedence
- Optional `else` clause
- Multiple expressions in `output()`

### **3. Modified `Constrainer.c`**
Added type checking for:
- New arithmetic operators (`*`, `/`, `**`, `mod`)
- New logical operators (`and`, `or`, `not`)
- New comparison operators (`=`, `<>`, `>=`, `<`, `>`)
- Boolean constants (`true`, `false`)
- `eof` function

### **4. Modified `CodeGenerator.c`**
Added code generation for all the above operators

---

## 🛠️ How to Use the System

### **1. Write a Tiny Program**

Create `test.tiny`:
```tiny
program test:
var x, y: integer;
begin
   x := 5;
   y := 10;
   output(x * y);
end test.
```

### **2. Compile and Run**

```bash
cd /path/to/tws/tiny
./tc test.tiny
```

### **3. What Happens Behind the Scenes**

```bash
# Step 1: Parse
parser/parse < test.tiny > _TREE

# Step 2: Check semantics
./Constrain

# Step 3: Generate code
./CodeGen

# Step 4: Run
../../bin/interpret
```

The `tc` script does all this automatically!

---

## 📖 File Formats

### **`lex.tiny` Format (Scanner)**

```lex
"keyword"    { return rule(TOKEN_NAME); }
{PATTERN}    { return node(TOKEN_NAME); }
```

Example:
```lex
"while"      { return rule(WHILE); }
[0-9]+       { return node(INTEGER_NUM); }
```

---

### **`parse.tiny` Format (Grammar)**

```
%%
Nonterminal -> Terminal1 Terminal2  => "node_name"
            -> Terminal3             => "other_node";
```

Example:
```
%%
Statement -> IF Expression THEN Statement  => "if"
          -> WHILE Expression DO Statement => "while";
```

---

## 🎯 Key Concepts

### **Tokens**
The smallest meaningful units (keywords, numbers, operators)

### **Grammar**
Rules that define valid syntax (like English grammar)

### **Parse Tree**
A tree showing program structure

### **Abstract Syntax Tree (AST)**
Simplified tree used for compilation

### **Symbol Table**
Database of variables and their types

### **Type Checking**
Ensuring operations make sense (can't multiply text!)

### **Code Generation**
Converting tree → machine instructions

### **Abstract Machine**
A virtual CPU that runs the generated code

---

## 🚀 Testing Your Changes

### **Test Files Location**
```
tiny/test-progs/
├── pr1.c01, pr1.c02, ...  # Correct programs (should work)
├── pr1.e01, pr1.e02, ...  # Error programs (should fail)
```

### **Run Tests**
```bash
./tc test-progs/pr1.c01   # Should print: 1 2 3 4 5 \n -3
./tc test-progs/pr1.e01   # Should report errors
```

---

## 🔍 Debugging Tips

### **1. Parse Errors**
Problem: "syntax error @line X"
- Check `lex.tiny`: Is the token defined?
- Check `parse.tiny`: Is the grammar rule correct?

### **2. Constrainer Errors**
Problem: "CONSTRAINER ERROR"
- Check `Constrainer.c`: Did you add the case for the new operator?

### **3. Code Generator Errors**
Problem: "UNKNOWN NODE NAME"
- Check `CodeGenerator.c`: Did you add the case in Expression() or ProcessNode()?
- Check node definitions: Is the #define there?

### **4. Runtime Errors**
Problem: "MACHINE ERROR"
- Code generation might be wrong
- Check if you're using correct opcodes (BOPOP, UOPOP, etc.)

---

## 📚 Additional Resources

### **Manuals (in `/manual/` directory)**
- `parse`: Parser documentation
- `pgen`: Parser generator documentation  
- `constrainer`: Semantic analysis documentation
- `code_generator`: Code generation documentation
- `abs_machine`: Virtual machine documentation

### **Documentation (in `/doc/` directory)**
- `flex.1`: Flex (lexical analyzer) manual
- `yacc.1`: Yacc (parser generator) manual

---

## ✅ Summary: The Complete Flow

```
1. YOU write:           Tiny program (test.tiny)
                              ↓
2. lex.tiny scans:      Creates tokens
                              ↓
3. parse.tiny parses:   Builds syntax tree (_TREE)
                              ↓
4. Constrainer checks:  Type checking, semantic analysis
                              ↓
5. CodeGenerator:       Generates machine code (_CODE)
                              ↓
6. Interpreter:         Executes the code
                              ↓
7. OUTPUT:              Program results!
```

---

## 🎓 Why This Matters

Understanding TWS teaches you:

1. **How programming languages work** (from text → execution)
2. **Compiler design** (real-world software engineering)
3. **Language theory** (grammars, parsing, etc.)
4. **Problem decomposition** (breaking big problems into small ones)
5. **C programming** (systems programming)

---

## 🤝 Need Help?

1. Check manuals in `/manual/` directory
2. Look at existing test programs in `test-progs/`
3. Read the lab document carefully
4. Debug step-by-step (parse → constrain → codegen)

---

**Good luck with your compiler construction journey! 🚀**
