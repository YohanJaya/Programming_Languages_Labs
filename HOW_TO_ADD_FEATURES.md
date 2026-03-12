# How to Add New Features to Tiny Compiler

## 🎯 The Complete Procedure

When adding a new feature to Tiny, you need to modify files in a specific order. Think of it as a **pipeline** where each stage processes the code.

---

## 📋 Quick Overview

```
Step 1: lex.tiny       → Define tokens (what words/symbols exist)
Step 2: parse.tiny     → Define grammar (how to combine tokens)
Step 3: Constrainer.c  → Add semantic checks (is it valid?)
Step 4: CodeGenerator.c → Generate machine code (how to execute)
```

---

## 🔧 Detailed Step-by-Step Guide

### **STEP 1: Add Tokens in `lex.tiny`**

**Location:** `tiny/parser/lex.tiny`

**What:** Define the lexical tokens (keywords, operators, literals)

#### **Example: Adding a `for` loop**

```lex
"for"     { return rule(FOR); }
"to"      { return rule(TO); }
```

#### **Important Rules:**

1. **Keywords MUST come BEFORE identifiers** (otherwise they'll be treated as identifiers)
2. **Multi-character operators MUST come BEFORE single-character ones**
   ```lex
   ":="    { return rule(ASSIGNMENT); }  // Must be before ':'
   ":"     { return rule(yytext[0]); }
   ```
3. **Order matters!** Flex matches the longest pattern first, but if equal length, it matches the first one defined.

#### **Ordering Template:**

```lex
{WHITE}         { column += yyleng; }
\n              { column = 1; line++; }

/* Keywords (alphabetical order is helpful) */
"and"           { return rule(AND); }
"begin"         { return rule(BEGINX); }
"boolean"       { return rule(BOOLEAN); }
"do"            { return rule(DO); }
"else"          { return rule(ELSE); }
"end"           { return rule(END); }
"false"         { return rule(FALSE_KWD); }
"for"           { return rule(FOR); }        // NEW
"if"            { return rule(IF); }
/* ... more keywords ... */

/* Multi-character operators */
":="            { return rule(ASSIGNMENT); }
"**"            { return rule(POW); }
"<="            { return rule(LTE); }
"<>"            { return rule(NEQ); }
">="            { return rule(GE); }

/* Identifiers and numbers */
{INT}           { return node(INTEGER_NUM); }
{IDENT}         { return node(IDENTIFIER); }

/* Comments */
"{"             { column += yyleng; BEGIN(COMM1); }
<COMM1>[^}\n]+  { column += yyleng; }
<COMM1>"}"      { column += yyleng; BEGIN(INITIAL); }
<COMM1>\n       { column = 1; line++; }

/* Single-character tokens */
":"             { return rule(yytext[0]); }
";"             { return rule(yytext[0]); }
"+"             { return rule(yytext[0]); }
/* ... more single chars ... */

/* Catch-all for errors */
.               { yyerror("unrecognized char");
                  printf("-->%s<--\n",yytext);
                  column++; }
```

---

### **STEP 2: Add Grammar Rules in `parse.tiny`**

**Location:** `tiny/parser/parse.tiny`

**What:** Define syntax rules (how tokens combine into valid statements)

#### **Grammar Format:**

```
%%
Nonterminal -> Symbol1 Symbol2 Symbol3  => "node_name"
            -> AnotherProduction         => "other_name"
            ->                           => "empty";
```

#### **Example: Adding `for` loop**

```
Statement -> FOR Name ASSIGNMENT Expression TO Expression DO Statement  => "for"
          -> IF Relational THEN Statement (ELSE Statement)?             => "if"
          -> WHILE Relational DO Statement                               => "while"
          -> Name ASSIGNMENT Relational                                  => "assign"
          -> OUTPUT '(' Relational list ',' ')'                          => "output"
          -> Body
          ->                                                             => "<null>";
```

#### **Grammar Guidelines:**

1. **Precedence is built into the structure**
   ```
   Expression -> Term                    // Lower precedence
   Term       -> Factor                  // Higher precedence  
   Factor     -> Primary                 // Highest precedence
   ```

2. **Associativity:**
   - **Left associative:** `A -> A '+' B` (recursion on left)
   - **Right associative:** `A -> B '+' A` (recursion on right)

3. **Optional elements:** Use `?`
   ```
   Statement -> IF Expr THEN Statement (ELSE Statement)?  => "if"
   ```

4. **Repetition:** Use `*` or `+`
   ```
   Dclns -> VAR (Dcln ';')*  => "dclns"
   ```

5. **Lists:** Use `list` keyword
   ```
   OUTPUT '(' Expression list ',' ')'  => "output"
   ```

#### **Node Names:**
- Choose descriptive names: `"for"`, `"while"`, `"assign"`
- These become the node types in the syntax tree
- Will be used in Constrainer and CodeGenerator

---

### **STEP 3: Add Semantic Checking in `Constrainer.c`**

**Location:** `tiny/Constrainer.c`

**What:** Check if the program makes logical sense (types, declarations, etc.)

#### **Three Things to Add:**

#### **3A. Define the Node Number**

At the top of the file:

```c
#define ProgramNode    1
#define TypesNode      2
// ... existing nodes ...
#define ForNode        35   // NEW - next available number
#define NumberOfNodes  35   // UPDATE this to match highest number
```

#### **3B. Add Node Name to Array**

```c
char *node[] = { "program", "types", "type", "dclns",
                 "dcln", "integer", "boolean", "block",
                 "assign", "output", "if", "while", 
                 "<null>", "<=", "+", "-", "read",
                 "<integer>", "<identifier>", "**", 
                 "=", "<>", ">=", "<", ">", 
                 "and", "or", "not", "mod", "*", "/",
                 "true", "false", "eof", "for"  // NEW
                };
```

#### **3C. Add Case in `ProcessNode()` or `Expression()`**

**For Statements** (if, while, for, assign, etc.) → Add to `ProcessNode()`:

```c
void ProcessNode (TreeNode T) 
{
   int Kid, N;
   String Name1, Name2;
   TreeNode Type1, Type2, Type3;
   
   switch (NodeName(T))
   {
      case ProgramNode:
         // ... existing code ...
         break;

      case ForNode:                           // NEW
         // Check loop variable is declared
         Type1 = Expression(Child(T,1));
         
         // Check start expression is integer
         Type2 = Expression(Child(T,2));
         if (Type2 != TypeInteger)
         {
            ErrorHeader(T);
            printf("FOR START EXPRESSION MUST BE INTEGER\n");
         }
         
         // Check end expression is integer
         Type3 = Expression(Child(T,3));
         if (Type3 != TypeInteger)
         {
            ErrorHeader(T);
            printf("FOR END EXPRESSION MUST BE INTEGER\n");
         }
         
         // Process loop body
         ProcessNode(Child(T,4));
         break;

      // ... other cases ...
   }
}
```

**For Expressions** (operators, functions, etc.) → Add to `Expression()`:

```c
UserType Expression (TreeNode T)
{
   UserType Type1, Type2;
   
   switch (NodeName(T))
   {
      case PlusNode:
      case MinusNode:
         Type1 = Expression(Child(T,1));
         if (Rank(T) == 2)
            Type2 = Expression(Child(T,2));
         else  
            Type2 = TypeInteger;
         if (Type1 != TypeInteger || Type2 != TypeInteger)
         {
            ErrorHeader(T);
            printf("ARITHMETIC ARGUMENTS MUST BE INTEGER\n");
         }
         return (TypeInteger);

      // ... other cases ...
   }
}
```

#### **Type Checking Patterns:**

```c
// For binary operators (both operands same type)
Type1 = Expression(Child(T,1));
Type2 = Expression(Child(T,2));
if (Type1 != Type2)
{
   ErrorHeader(T);
   printf("TYPES MUST MATCH\n");
}
return (TypeBoolean);  // or TypeInteger

// For arithmetic (both must be integer)
if (Type1 != TypeInteger || Type2 != TypeInteger)
{
   ErrorHeader(T);
   printf("MUST BE INTEGER\n");
}
return (TypeInteger);

// For logical (both must be boolean)
if (Type1 != TypeBoolean || Type2 != TypeBoolean)
{
   ErrorHeader(T);
   printf("MUST BE BOOLEAN\n");
}
return (TypeBoolean);

// For comparisons (same type in, boolean out)
if (Type1 != Type2)
{
   ErrorHeader(T);
   printf("COMPARISON TYPES MUST MATCH\n");
}
return (TypeBoolean);
```

---

### **STEP 4: Add Code Generation in `CodeGenerator.c`**

**Location:** `tiny/CodeGenerator.c`

**What:** Generate abstract machine instructions

#### **Four Things to Add:**

#### **4A. Define Node Number (Same as Constrainer)**

```c
#define    ProgramNode  48
#define    TypesNode    49
// ... existing ...
#define    ForNode      82   // NEW - continues from last node
#define    NumberOfNodes 82  // UPDATE
```

#### **4B. Add Node Name to Array**

```c
char *node_name[] =
    {"program","types","type","dclns","dcln","integer",
     "boolean","block","assign","output","if","while",
     "<null>","<=","+","-","read","<integer>","<identifier>",
     "**","=","<>",">=","<",">","and","or","not","mod","*","/",
     "true","false","eof","for"};  // NEW
```

#### **4C. Add Case in `ProcessNode()` or `Expression()`**

**For Statements** → Add to `ProcessNode()`:

```c
Clabel ProcessNode (TreeNode T, Clabel CurrLabel)
{
   int Kid, Num;
   Clabel Label1, Label2, Label3, Label4;

   switch (NodeName(T))
   {
      case ForNode:                                    // NEW
         // Example for: for i := 1 to 10 do <body>
         
         // Generate: i := start_value
         Expression(Child(T,2), CurrLabel);            // start expression
         Reference(Child(T,1), LeftMode, NoLabel);     // store in loop var
         
         // Label1: Loop condition check
         Label1 = MakeLabel();
         Label2 = MakeLabel();
         Label3 = MakeLabel();
         
         // Load loop variable and end value
         Reference(Child(T,1), RightMode, Label1);     // load i
         Expression(Child(T,3), NoLabel);              // load end value
         
         // Check if i <= end
         CodeGen1(BOPOP, BLE, NoLabel);
         DecrementFrameSize();
         
         // Branch
         CodeGen2(CONDOP, Label2, Label3, NoLabel);
         DecrementFrameSize();
         
         // Label2: Loop body
         ProcessNode(Child(T,4), Label2);              // execute body
         
         // Increment loop variable
         Reference(Child(T,1), RightMode, NoLabel);    // load i
         CodeGen1(LITOP, MakeStringOf(1), NoLabel);    // push 1
         IncrementFrameSize();
         CodeGen1(BOPOP, BPLUS, NoLabel);              // i + 1
         DecrementFrameSize();
         Reference(Child(T,1), LeftMode, NoLabel);     // store back to i
         
         // Jump back to condition
         CodeGen1(GOTOOP, Label1, NoLabel);
         
         // Label3: Exit loop
         return (Label3);

      // ... other cases ...
   }
}
```

**For Expressions** → Add to `Expression()`:

```c
void Expression (TreeNode T, Clabel CurrLabel)
{
   switch (NodeName(T))
   {
      case MinusNode:
         Expression(Child(T,1), CurrLabel);
         if (Rank(T) == 2)
         {
            Expression(Child(T,2), NoLabel);
            CodeGen1(BOPOP, BMINUS, NoLabel);
            DecrementFrameSize();
         }
         else
            CodeGen1(UOPOP, UNEG, NoLabel);
         break;

      // ... other cases ...
   }
}
```

#### **Common Code Generation Patterns:**

```c
// Binary arithmetic operator (+, -, *, /, mod, **)
Expression(Child(T,1), CurrLabel);      // Left operand
Expression(Child(T,2), NoLabel);        // Right operand
CodeGen1(BOPOP, BPLUS, NoLabel);        // Operation (BPLUS, BMINUS, BMULT, etc.)
DecrementFrameSize();                   // Result is one value

// Unary operator (-, +, not)
Expression(Child(T,1), CurrLabel);
CodeGen1(UOPOP, UNEG, NoLabel);         // UNEG, UNOT

// Comparison operator (=, <>, <, >, <=, >=)
Expression(Child(T,1), CurrLabel);
Expression(Child(T,2), NoLabel);
CodeGen1(BOPOP, BEQ, NoLabel);          // BEQ, BNE, BLT, BGT, BLE, BGE
DecrementFrameSize();

// Literal value
CodeGen1(LITOP, MakeStringOf(value), CurrLabel);
IncrementFrameSize();

// Variable reference (load)
Reference(T, RightMode, CurrLabel);

// Variable assignment (store)
Expression(Child(T,2), CurrLabel);      // Compute value
Reference(Child(T,1), LeftMode, NoLabel); // Store it

// If statement
Expression(Child(T,1), CurrLabel);      // Condition
Label1 = MakeLabel();                   // True branch
Label2 = MakeLabel();                   // False branch
Label3 = MakeLabel();                   // After if
CodeGen2(CONDOP, Label1, Label2, NoLabel);
DecrementFrameSize();
CodeGen1(GOTOOP, Label3, ProcessNode(Child(T,2), Label1));  // Then
if (Rank(T) == 3)
   CodeGen0(NOP, ProcessNode(Child(T,3), Label2));          // Else
else
   CodeGen0(NOP, Label2);
return (Label3);

// While loop
if (CurrLabel == NoLabel) 
   Label1 = MakeLabel();
else 
   Label1 = CurrLabel;
Label2 = MakeLabel();                   // Loop body
Label3 = MakeLabel();                   // After loop
Expression(Child(T,1), Label1);         // Condition
CodeGen2(CONDOP, Label2, Label3, NoLabel);
DecrementFrameSize();
CodeGen1(GOTOOP, Label1, ProcessNode(Child(T,2), Label2)); // Body
return (Label3);
```

#### **Machine Instructions Reference:**

```c
// Stack operations
LITOP    // Push literal value
LLVOP    // Load local variable
LGVOP    // Load global variable
SLVOP    // Store local variable
SGVOP    // Store global variable

// Arithmetic/Logic
BOPOP    // Binary operation (needs operand: BPLUS, BMINUS, BMULT, etc.)
UOPOP    // Unary operation (needs operand: UNEG, UNOT)

// Control flow
GOTOOP   // Unconditional jump
CONDOP   // Conditional branch (2 labels: true, false)

// I/O
SOSOP    // System operation (OSINPUT, OSOUTPUT, OSEOF, etc.)
```

---

## 🎯 Complete Example: Adding `repeat-until` Loop

Let's add: `repeat <body> until <condition>`

### **Step 1: lex.tiny**

```lex
"repeat"    { return rule(REPEAT); }
"until"     { return rule(UNTIL); }
```

### **Step 2: parse.tiny**

```
Statement -> REPEAT Statement UNTIL Expression  => "repeat"
          -> /* other statements... */
```

### **Step 3: Constrainer.c**

```c
// At top
#define RepeatNode     35
#define NumberOfNodes  35

// In node array
char *node[] = { /* ... */, "repeat" };

// In ProcessNode()
case RepeatNode:
   ProcessNode(Child(T,1));              // Check body
   if (Expression(Child(T,2)) != TypeBoolean)
   {
      ErrorHeader(T);
      printf("REPEAT CONDITION MUST BE BOOLEAN\n");
   }
   break;
```

### **Step 4: CodeGenerator.c**

```c
// At top
#define RepeatNode     82
#define NumberOfNodes  82

// In node_name array
char *node_name[] = { /* ... */, "repeat" };

// In ProcessNode()
case RepeatNode:
   if (CurrLabel == NoLabel)
      Label1 = MakeLabel();
   else
      Label1 = CurrLabel;
   Label2 = MakeLabel();
   
   // Body
   ProcessNode(Child(T,1), Label1);
   
   // Condition
   Expression(Child(T,2), NoLabel);
   
   // If false, repeat; if true, exit
   CodeGen2(CONDOP, Label2, Label1, NoLabel);
   DecrementFrameSize();
   
   return (Label2);
```

---

## ✅ Testing Your New Feature

1. **Write test program:**
   ```tiny
   program test:
   var i: integer;
   begin
      i := 1;
      repeat
         output(i);
         i := i + 1
      until i > 5;
   end test.
   ```

2. **Compile:**
   ```bash
   cd tiny
   make clean
   make
   ```

3. **Run:**
   ```bash
   ./tc test.tiny
   ```

4. **Expected output:**
   ```
   1
   2
   3
   4
   5
   ```

---

## 🐛 Common Errors and Solutions

### **1. "unrecognized char" Error**
- **Cause:** Token not defined in lex.tiny
- **Fix:** Add the token definition

### **2. "syntax error" Error**
- **Cause:** Grammar rule missing or incorrect in parse.tiny
- **Fix:** Check grammar syntax, ensure `%%` is present

### **3. "UNKNOWN NODE NAME" in Constrainer**
- **Cause:** Node not defined or case missing
- **Fix:** Add #define, update NumberOfNodes, add node name to array, add case

### **4. "UNKNOWN NODE NAME" in CodeGenerator**
- **Cause:** Same as above but in CodeGenerator.c
- **Fix:** Same as above

### **5. "MACHINE ERROR" at Runtime**
- **Cause:** Incorrect code generation
- **Fix:** Check stack balance (push/pop), verify labels, check operand types

---

## 📚 Summary Checklist

When adding a new feature:

- [ ] **Step 1:** Add tokens in `lex.tiny` (proper order!)
- [ ] **Step 2:** Add grammar rules in `parse.tiny`
- [ ] **Step 3a:** Add node #define in `Constrainer.c`
- [ ] **Step 3b:** Add node name to array in `Constrainer.c`
- [ ] **Step 3c:** Add case in ProcessNode/Expression in `Constrainer.c`
- [ ] **Step 4a:** Add node #define in `CodeGenerator.c`
- [ ] **Step 4b:** Add node name to array in `CodeGenerator.c`
- [ ] **Step 4c:** Add case in ProcessNode/Expression in `CodeGenerator.c`
- [ ] **Test:** Write test program and verify it works

---

## 🎓 Key Principles

1. **Order matters in lex.tiny** - longest/most specific first
2. **Precedence is structure in parse.tiny** - higher = deeper in tree
3. **Type checking is safety** - catch errors before runtime
4. **Code generation is translation** - tree → machine instructions
5. **Test incrementally** - one feature at a time

---

**Good luck adding features to your compiler! 🚀**
