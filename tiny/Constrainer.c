/****************************************************************
              Copyright (C) 1986 by Manuel E. Bermudez
              Translated to C - 1991
              Lab 5: Constants, Enum Types, Synonyms, Modes
*****************************************************************/

#include <stdio.h>
#include <header/Open_File.h>
#include <header/CommandLine.h>
#include <header/Table.h>
#include <header/Text.h>
#include <header/Error.h>
#include <header/String_Input.h> 
#include <header/Tree.h>
#include <header/Dcln.h>
#include <header/Constrainer.h>

#define ProgramNode    1
#define TypesNode      2
#define TypeNode       3
#define DclnsNode      4
#define DclnNode       5
#define IntegerTNode   6
#define BooleanTNode   7
#define BlockNode      8
#define AssignNode     9
#define OutputNode     10
#define IfNode         11
#define WhileNode      12
#define NullNode       13
#define LENode         14
#define PlusNode       15
#define MinusNode      16
#define ReadNode       17
#define IntegerNode    18
#define IdentifierNode 19
#define TrueNode       20
#define FalseNode      21
#define EQNode         22
#define NENode         23
#define GENode         24
#define LTNode         25
#define GTNode         26
#define OrNode         27
#define MultNode       28
#define DivNode        29
#define AndNode        30
#define ModNode        31
#define ExpNode        32
#define NotNode        33
#define EofNode        34
#define ForToNode      35
#define ForDowntoNode  36
#define RepeatNode     37
#define CaseNode       38
#define CaseClauseNode      39
#define CaseClauseRangeNode 40
#define SwapNode       41
#define LoopNode       42
#define ExitNode       43
#define OtherwiseNode  44
/* --- Lab 5 new nodes --- */
#define ConstsNode     45
#define ConstNode      46
#define CharNode       47
#define LitNode        48

#define NumberOfNodes  48

/* Extra string constant indices for intrinsic identifiers */
#define FalseStr (NumberOfNodes + 1)   /* 49 */
#define TrueStr  (NumberOfNodes + 2)   /* 50 */
#define CharStr  (NumberOfNodes + 3)   /* 51 */

/* Identifier modes */
#define ModeVariable  0
#define ModeConstant  1
#define ModeType      2
#define ModeLiteral   3

#define MAX_NODES 5000
static int NodeMode[MAX_NODES];

typedef TreeNode UserType;

char *node[] = { "program", "types", "type", "dclns",
                 "dcln", "integer", "boolean", "block",
                 "assign", "output", "if", "while",
                 "<null>", "<=", "+", "-", "read",
                 "<integer>", "<identifier>",
                 "<true>", "<false>", "=", "<>", ">=",
                 "<", ">", "or", "*", "/", "and",
                 "mod", "**", "not", "eof", "for_to", "for_downto",
                 "repeat", "case", "case_clause", "case_clause_range",
                 "swap", "loop", "exit", "otherwise",
                 "consts", "const", "<char>", "lit"
                };

UserType TypeInteger, TypeBoolean, TypeChar;
boolean TraceSpecified;
FILE *TraceFile;
char *TraceFileName;
int NumberTreesRead, trace_index;

/* ------------------------------------------------------------------ */
static void DTEnterWithMode(String name, TreeNode T, TreeNode Source, int mode)
{
   DTEnter(name, T, Source);
   if (T > 0 && T < MAX_NODES)
      NodeMode[T] = mode;
}

static int GetMode(TreeNode T)
{
   if (T > 0 && T < MAX_NODES)
      return NodeMode[T];
   return ModeVariable;
}

/* ------------------------------------------------------------------ */
void Constrain(void)
{
   InitializeDeclarationTable();
   Tree_File = Open_File("_TREE", "r");
   NumberTreesRead = Read_Trees();
   fclose(Tree_File);
   AddIntrinsics();
   ProcessNode(RootOfTree(1));
   Tree_File = fopen("_TREE", "w");
   Write_Trees();
   fclose(Tree_File);
   if (TraceSpecified) fclose(TraceFile);
}

/* ------------------------------------------------------------------ */
void InitializeConstrainer(int argc, char *argv[])
{
   int i, j;
   InitializeTextModule();
   InitializeTreeModule();
   for (i=0, j=1; i<NumberOfNodes; i++, j++)
      String_Array_To_String_Constant(node[i], j);
   /* Register extra strings for intrinsic identifiers */
   String_Array_To_String_Constant("false", FalseStr);
   String_Array_To_String_Constant("true",  TrueStr);
   String_Array_To_String_Constant("char",  CharStr);

   trace_index = System_Flag("-trace", argc, argv);
   if (trace_index) {
      TraceSpecified = true;
      TraceFileName = System_Argument("-trace", "_TRACE", argc, argv);
      TraceFile = Open_File(TraceFileName, "w");
   } else
      TraceSpecified = false;
}

/* ------------------------------------------------------------------ */
/*  AddIntrinsics: use ONLY position-1 AddTree inserts (reverse order)
 *  Final structure of intrinsic types node:
 *    child 1: char type  -> char leaf
 *    child 2: integer type -> integer leaf
 *    child 3: boolean type -> boolean leaf
 *                          -> lit -> false(ord 0), true(ord 1)
 * ------------------------------------------------------------------ */
void AddIntrinsics(void)
{
   TreeNode TempTree, BoolType, IntType, CharType, LitT, FalseId, TrueId;

   AddTree(TypesNode, RootOfTree(1), 2);
   TempTree = Child(RootOfTree(1), 2);

   /* Step A: boolean type (added first at pos 1) */
   AddTree(TypeNode, TempTree, 1);
   BoolType = Child(TempTree, 1);
   /* Build lit node using only pos-1 inserts */
   AddTree(LitNode, BoolType, 1);         /* lit at pos 1 */
   LitT = Child(BoolType, 1);
   AddTree(TrueStr, LitT, 1);             /* true at pos 1 of lit */
   AddTree(FalseStr, LitT, 1);            /* false at pos 1, pushes true to pos 2 */
   FalseId = Child(LitT, 1);             /* ordinal 0 */
   TrueId  = Child(LitT, 2);             /* ordinal 1 */
   AddTree(BooleanTNode, BoolType, 1);    /* boolean leaf at pos 1, lit pushed to pos 2 */

   /* Step B: integer type (pos 1, BoolType pushed to pos 2) */
   AddTree(TypeNode, TempTree, 1);
   IntType = Child(TempTree, 1);
   AddTree(IntegerTNode, IntType, 1);

   /* Step C: char type (pos 1, IntType to 2, BoolType to 3) */
   AddTree(TypeNode, TempTree, 1);
   CharType = Child(TempTree, 1);
   AddTree(CharStr, CharType, 1);

   TypeChar    = CharType;
   TypeInteger = IntType;
   TypeBoolean = BoolType;

   /* Register all intrinsic declarations */
   DTEnterWithMode(CharStr,      CharType, CharType, ModeType);
   DTEnterWithMode(IntegerTNode, IntType,  IntType,  ModeType);
   DTEnterWithMode(BooleanTNode, BoolType, BoolType, ModeType);
   DTEnterWithMode(FalseStr, FalseId, BoolType, ModeLiteral);
   Decorate(FalseId, 0);
   DTEnterWithMode(TrueStr, TrueId, BoolType, ModeLiteral);
   Decorate(TrueId, 1);
}

/* ------------------------------------------------------------------ */
void ErrorHeader(TreeNode T)
{
   printf("<<< CONSTRAINER ERROR >>> AT ");
   Write_String(stdout, SourceLocation(T));
   printf(" : \n");
}

int NKids(TreeNode T) { return Rank(T); }

/* ------------------------------------------------------------------ */
UserType Expression(TreeNode T)
{
   UserType Type1, Type2;
   TreeNode Declaration;
   if (TraceSpecified) {
      fprintf(TraceFile, "<<< CONSTRAINER >>> : Expn Processor Node ");
      Write_String(TraceFile, NodeName(T));
      fprintf(TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case LENode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != Type2) {
            ErrorHeader(Child(T,1));
            printf("ARGUMENTS OF '<=' MUST BE SAME TYPE\n\n");
         }
         return TypeBoolean;

      case PlusNode:
      case MinusNode:
         Type1 = Expression(Child(T,1));
         Type2 = (Rank(T)==2) ? Expression(Child(T,2)) : TypeInteger;
         if (Type1 != TypeInteger || Type2 != TypeInteger) {
            ErrorHeader(Child(T,1));
            printf("ARGUMENTS OF '+', '-' MUST BE TYPE INTEGER\n\n");
         }
         return TypeInteger;

      case EofNode:
         return TypeBoolean;

      case IntegerNode:
         return TypeInteger;

      case CharNode:
         return TypeChar;

      case EQNode: case NENode: case GENode: case LTNode: case GTNode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (NodeName(T)==EQNode || NodeName(T)==NENode) {
            if (Type1 != Type2) {
               ErrorHeader(Child(T,1));
               printf("ARGUMENTS OF '=', '<>' MUST HAVE MATCHING TYPES\n\n");
            }
         } else {
            if (Type1 != TypeInteger || Type2 != TypeInteger) {
               ErrorHeader(Child(T,1));
               printf("ARGUMENTS OF '>=','<','>' MUST BE TYPE INTEGER\n\n");
            }
         }
         return TypeBoolean;

      case MultNode: case DivNode: case ModNode: case ExpNode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != TypeInteger || Type2 != TypeInteger) {
            ErrorHeader(Child(T,1));
            printf("ARGUMENTS OF '*','/','mod','**' MUST BE TYPE INTEGER\n\n");
         }
         return TypeInteger;

      case AndNode: case OrNode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != TypeBoolean || Type2 != TypeBoolean) {
            ErrorHeader(Child(T,1));
            printf("ARGUMENTS OF 'and','or' MUST BE TYPE BOOLEAN\n\n");
         }
         return TypeBoolean;

      case NotNode:
         Type1 = Expression(Child(T,1));
         if (Type1 != TypeBoolean) {
            ErrorHeader(Child(T,1));
            printf("ARGUMENT OF 'not' MUST BE TYPE BOOLEAN\n\n");
         }
         return TypeBoolean;

      case TrueNode:  return TypeBoolean;
      case FalseNode: return TypeBoolean;

      case IdentifierNode: {
         String name = NodeName(Child(T,1));
         Declaration = Lookup(name, T);
         if (Declaration != NullDeclaration) {
            int mode = GetMode(Declaration);
            Decorate(T, Declaration);
            UserType typeNode = Decoration(Declaration);

            if (mode == ModeLiteral) {
               /* Store -(ordinal+1) on leaf so code gen emits LIT ordinal */
               int ordinal = Decoration(Declaration);
               Decorate(Child(T,1), -(ordinal + 1));
               return TypeInteger;
            }
            if (mode == ModeType) {
               ErrorHeader(T);
               printf("TYPE NAME USED AS VALUE\n\n");
               return TypeInteger;
            }
            /* Variable/Constant: if enum-typed, store lit count on leaf for LIMIT */
            if (typeNode != 0 && NodeName(typeNode) == TypeNode &&
                NKids(typeNode) >= 2 && NodeName(Child(typeNode,2)) == LitNode) {
               Decorate(Child(T,1), NKids(Child(typeNode,2)));
            }
            return typeNode;
         }
         return TypeInteger;
      }

      default:
         ErrorHeader(T);
         printf("UNKNOWN EXPRESSION NODE ");
         Write_String(stdout, NodeName(T));
         printf("\n");
         return TypeInteger;
   }
}

/* ------------------------------------------------------------------ */
void ProcessNode(TreeNode T)
{
   int Kid;
   String Name1, Name2;
   TreeNode Type1, Type2;
   if (TraceSpecified) {
      fprintf(TraceFile, "<<< CONSTRAINER >>> : Stmt Processor Node ");
      Write_String(TraceFile, NodeName(T));
      fprintf(TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case ProgramNode:
         Name1 = NodeName(Child(Child(T,1),1));
         Name2 = NodeName(Child(Child(T,NKids(T)),1));
         if (Name1 != Name2) {
            ErrorHeader(T);
            printf("PROGRAM NAMES DO NOT MATCH\n\n");
         }
         for (Kid = 2; Kid <= NKids(T)-1; Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case TypesNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case TypeNode: {
         /* Skip intrinsic types (already registered by AddIntrinsics) */
         TreeNode child1 = Child(T,1);
         if (NodeName(child1) != IdentifierNode) break;

         String typeName = NodeName(Child(child1,1));
         DTEnterWithMode(typeName, T, T, ModeType);

         if (NKids(T) >= 2) {
            TreeNode child2 = Child(T,2);
            if (NodeName(child2) == LitNode) {
               /* Enumerated type: register each literal */
               int ord;
               for (ord = 1; ord <= NKids(child2); ord++) {
                  TreeNode litId = Child(child2, ord);
                  String litName = NodeName(Child(litId,1));
                  DTEnterWithMode(litName, litId, T, ModeLiteral);
                  Decorate(litId, ord-1);
               }
            } else if (NodeName(child2) == IdentifierNode) {
               /* Synonym: type num = integer */
               String refName = NodeName(Child(child2,1));
               TreeNode refType = Lookup(refName, T);
               if (refType != NullDeclaration)
                  Decorate(T, refType);
            }
         }
         break;
      }

      case ConstsNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case ConstNode: {
         String constName = NodeName(Child(Child(T,1),1));
         TreeNode valNode = Child(T,2);
         UserType constType = TypeInteger;
         if (NodeName(valNode) == CharNode)
            constType = TypeChar;
         else if (NodeName(valNode) == IdentifierNode) {
            TreeNode decl = Lookup(NodeName(Child(valNode,1)), T);
            if (decl != NullDeclaration)
               constType = Decoration(decl);
         }
         DTEnterWithMode(constName, T, T, ModeConstant);
         Decorate(T, constType);
         break;
      }

      case DclnsNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case DclnNode: {
         /* Last child is the type <identifier> node */
         TreeNode typeIdNode = Child(T, NKids(T));
         Name1 = NodeName(Child(typeIdNode, 1));
         Type1 = Lookup(Name1, T);
         if (Type1 == NullDeclaration) {
            ErrorHeader(T);
            printf("UNKNOWN TYPE ");
            Write_String(stdout, Name1);
            printf("\n\n");
            Type1 = TypeInteger;
         } else {
            /* Resolve synonyms */
            TreeNode dec = Decoration(Type1);
            if (dec != 0 && NodeName(dec) == TypeNode)
               Type1 = dec;
         }
         for (Kid = 1; Kid < NKids(T); Kid++) {
            TreeNode varId = Child(T, Kid);
            DTEnterWithMode(NodeName(Child(varId,1)), varId, T, ModeVariable);
            Decorate(varId, Type1);
         }
         break;
      }

      case BlockNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case AssignNode: {
         TreeNode lhsId = Child(T,1);
         TreeNode lhsDecl = Lookup(NodeName(Child(lhsId,1)), T);
         if (lhsDecl != NullDeclaration && GetMode(lhsDecl) != ModeVariable) {
            ErrorHeader(T);
            printf("LEFT SIDE OF ASSIGNMENT MUST BE A VARIABLE\n\n");
         }
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != Type2) {
            ErrorHeader(T);
            printf("ASSIGNMENT TYPES DO NOT MATCH\n\n");
         }
         break;
      }

      case OutputNode:
         for (Kid = 1; Kid <= NKids(T); Kid++) {
            TreeNode arg = Child(T,Kid);
            if (NodeName(arg) == CharNode) continue;
            UserType t = Expression(arg);
            if (t != TypeInteger && t != TypeChar) {
               ErrorHeader(T);
               printf("OUTPUT EXPRESSION MUST BE TYPE INTEGER OR CHAR\n\n");
            }
         }
         break;

      case ReadNode:
         /* read(x,z) as statement */
         for (Kid = 1; Kid <= NKids(T); Kid++) {
            TreeNode nameNode = Child(T,Kid);
            TreeNode decl = Lookup(NodeName(Child(nameNode,1)), T);
            if (decl != NullDeclaration) {
               Decorate(nameNode, decl);
               if (GetMode(decl) != ModeVariable) {
                  ErrorHeader(T);
                  printf("READ ARGUMENT MUST BE A VARIABLE\n\n");
               }
            }
         }
         break;

      case IfNode:
         if (Expression(Child(T,1)) != TypeBoolean) {
            ErrorHeader(T);
            printf("CONTROL EXPRESSION OF 'IF' STMT IS NOT TYPE BOOLEAN\n\n");
         }
         ProcessNode(Child(T,2));
         if (Rank(T) == 3) ProcessNode(Child(T,3));
         break;

      case WhileNode:
         if (Expression(Child(T,1)) != TypeBoolean) {
            ErrorHeader(T);
            printf("WHILE EXPRESSION NOT OF TYPE BOOLEAN\n\n");
         }
         ProcessNode(Child(T,2));
         break;

      case ForToNode:
      case ForDowntoNode:
         if (Expression(Child(T,1)) != TypeInteger) {
            ErrorHeader(T); printf("FOR CONTROL VARIABLE MUST BE INTEGER\n\n");
         }
         if (Expression(Child(T,2)) != TypeInteger) {
            ErrorHeader(T); printf("FOR START EXPRESSION MUST BE INTEGER\n\n");
         }
         if (Expression(Child(T,3)) != TypeInteger) {
            ErrorHeader(T); printf("FOR END EXPRESSION MUST BE INTEGER\n\n");
         }
         ProcessNode(Child(T,4));
         break;

      case RepeatNode:
         for (Kid = 1; Kid < NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         if (Expression(Child(T,NKids(T))) != TypeBoolean) {
            ErrorHeader(T);
            printf("UNTIL EXPRESSION OF 'REPEAT' MUST BE TYPE BOOLEAN\n\n");
         }
         break;

      case CaseNode: {
         Expression(Child(T,1));
         int i;
         for (i = 2; i < NKids(T); i++) {
            TreeNode Clause = Child(T,i);
            if (Rank(Clause) == 3) ProcessNode(Child(Clause,3));
            else                   ProcessNode(Child(Clause,2));
         }
         if (NodeName(Child(T,NKids(T))) != NullNode)
            ProcessNode(Child(Child(T,NKids(T)),1));
         break;
      }

      case SwapNode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != Type2) {
            ErrorHeader(T); printf("SWAP TYPES DO NOT MATCH\n\n");
         }
         break;

      case LoopNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;

      case ExitNode: break;
      case NullNode: break;

      default:
         ErrorHeader(T);
         printf("UNKNOWN NODE NAME ");
         Write_String(stdout, NodeName(T));
         printf("\n");
   }
}