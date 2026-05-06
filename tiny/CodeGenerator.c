/*******************************************************************
          Copyright (C) 1986 by Manuel E. Bermudez
          Translated to C - 1991
          Lab 5 Extensions: Constants, Enums, Synonyms, LIMITOP
********************************************************************/

#include <stdio.h>
#include <header/CommandLine.h>
#include <header/Open_File.h>
#include <header/Table.h>
#include <header/Text.h>
#include <header/Error.h>
#include <header/String_Input.h>
#include <header/Tree.h>
#include <header/Code.h>
#include <header/CodeGenerator.h>  

#define LeftMode  0
#define RightMode 1

    /*  ABSTRACT MACHINE OPERATIONS  */
#define    NOP          1
#define    HALTOP       2
#define    LITOP        3
#define    LLVOP        4
#define    LGVOP        5
#define    SLVOP        6
#define    SGVOP        7
#define    LLAOP        8
#define    LGAOP        9
#define    UOPOP       10
#define    BOPOP       11
#define    POPOP       12
#define    DUPOP       13
#define    SWAPOP      14
#define    CALLOP      15
#define    RTNOP       16
#define    GOTOOP      17
#define    CONDOP      18
#define    CODEOP      19
#define    SOSOP       20
#define    LIMITOP     21

    /* ABSTRACT MACHINE OPERANDS */
#define    UNOT        22
#define    UNEG        23
#define    USUCC       24
#define    UPRED       25
#define    BAND        26
#define    BOR         27
#define    BPLUS       28
#define    BMINUS      29
#define    BMULT       30
#define    BDIV        31
#define    BEXP        32
#define    BMOD        33
#define    BEQ         34
#define    BNE         35
#define    BLE         36
#define    BGE         37
#define    BLT         38
#define    BGT         39
#define    TRACEX      40
#define    DUMPMEM     41
#define    OSINPUT     42
#define    OSINPUTC    43
#define    OSOUTPUT    44
#define    OSOUTPUTC   45
#define    OSOUTPUTL   46
#define    OSEOF       47

         /* TREE NODE NAMES */
#define    ProgramNode  48
#define    TypesNode    49
#define    TypeNode     50
#define    DclnsNode    51
#define    DclnNode     52
#define    IntegerTNode 53
#define    BooleanTNode 54
#define    BlockNode    55
#define    AssignNode   56
#define    OutputNode   57
#define    IfNode       58
#define    WhileNode    59
#define    NullNode     60
#define    LENode       61
#define    PlusNode     62
#define    MinusNode    63
#define    ReadNode     64   /* read as statement */
#define    IntegerNode  65
#define    IdentifierNode 66
#define    TrueNode       67
#define    FalseNode      68
#define    EQNode    69
#define    NENode    70
#define    GENode    71
#define    LTNode    72
#define    GTNode    73
#define    OrNode    74
#define    MultNode  75
#define    DivNode   76
#define    AndNode   77
#define    ModNode   78
#define    ExpNode   79
#define    NotNode   80
#define    EofNode   81
#define    ForToNode     82
#define    ForDowntoNode 83
#define    RepeatNode    84
#define    CaseNode      85
#define    CaseClauseNode      86
#define    CaseClauseRangeNode 87
#define    SwapNode      88
#define    LoopNode      89
#define    ExitNode      90
#define    OtherwiseNode 91
/* --- Lab 5 new nodes --- */
#define    ConstsNode    92   /* "consts" */
#define    ConstNode     93   /* "const"  */
#define    CharNode      94   /* "<char>" */
#define    LitNode       95   /* "lit"    */

#define NumberOfNodes 95

typedef int Mode;

FILE *CodeFile;
char *CodeFileName;
Clabel HaltLabel;

#define MAX_LOOP_NESTING 128
static Clabel LoopExitLabelStack[MAX_LOOP_NESTING];
static int LoopExitTop = -1;

void PushLoopExitLabel(Clabel L) {
   if (LoopExitTop < MAX_LOOP_NESTING - 1)
      LoopExitLabelStack[++LoopExitTop] = L;
}
void PopLoopExitLabel(void) {
   if (LoopExitTop >= 0) LoopExitTop--;
}
Clabel CurrentLoopExitLabel(void) {
   if (LoopExitTop >= 0) return LoopExitLabelStack[LoopExitTop];
   return NoLabel;
}

char *mach_op[] = 
    {"NOP","HALT","LIT","LLV","LGV","SLV","SGV","LLA","LGA",
     "UOP","BOP","POP","DUP","SWAP","CALL","RTN","GOTO","COND",
     "CODE","SOS","LIMIT","UNOT","UNEG","USUCC","UPRED","BAND",
     "BOR","BPLUS","BMINUS","BMULT","BDIV","BEXP","BMOD","BEQ",
     "BNE","BLE","BGE","BLT","BGT","TRACEX","DUMPMEM","INPUT",
     "INPUTC","OUTPUT","OUTPUTC","OUTPUTL","EOF"};

char *node_name[] =
    {"program","types","type","dclns","dcln","integer",
     "boolean","block","assign","output","if","while",
     "<null>","<=","+","-","read","<integer>","<identifier>",
     "<true>","<false>","=","<>",">=","<",">","or",
     "*","/","and","mod","**","not","eof","for_to",
     "for_downto","repeat","case","case_clause","case_clause_range","swap",
     "loop","exit","otherwise",
     "consts","const","<char>","lit"};


void CodeGenerate(int argc, char *argv[])
{
   int NumberTrees;
   InitializeCodeGenerator(argc,argv);
   Tree_File = Open_File("_TREE", "r"); 
   NumberTrees = Read_Trees();
   fclose(Tree_File);                     
   HaltLabel = ProcessNode(RootOfTree(1), NoLabel);
   CodeGen0(HALTOP, HaltLabel); 
   CodeFile = Open_File(CodeFileName, "w");
   DumpCode(CodeFile);
   fclose(CodeFile); 
   if (TraceSpecified) fclose(TraceFile);
   Tree_File = fopen("_TREE", "w");  
   Write_Trees();
   fclose(Tree_File);                   
}


void InitializeCodeGenerator(int argc, char *argv[])
{
   InitializeMachineOperations();
   InitializeNodeNames();
   FrameSize = 0;
   CurrentProcLevel = 0;
   LabelCount = 0;
   CodeFileName = System_Argument("-code", "_CODE", argc, argv); 
}


void InitializeMachineOperations(void)
{
   int i,j;
   for (i=0, j=1; i < 47; i++, j++)
      String_Array_To_String_Constant(mach_op[i], j);
}


void InitializeNodeNames(void)
{
   int i,j;
   for (i=0, j=48; j <= NumberOfNodes; i++, j++)
      String_Array_To_String_Constant(node_name[i], j);
}


String MakeStringOf(int Number)
{
   Stack Temp, Temp2;
   Temp = AllocateStack(50);
   ResetBufferInTextTable();
   if (Number == 0) {
      AdvanceOnCharacter('0');
   } else if (Number < 0) {
      AdvanceOnCharacter('-');
      Number = -Number;
      Temp2 = AllocateStack(20);
      while (Number > 0) { Push(Temp2,(Number%10)+48); Number/=10; }
      while (!IsEmpty(Temp2)) AdvanceOnCharacter((char)(Pop(Temp2)));
   } else {
      while (Number > 0) { Push(Temp,(Number%10)+48); Number/=10; }
      while (!IsEmpty(Temp)) AdvanceOnCharacter((char)(Pop(Temp)));
   }   
   return ConvertStringInBuffer();
}  


void Reference(TreeNode T, Mode M, Clabel L)
{
   int Addr, OFFSET;
   String Op;
   Addr = Decoration(Decoration(T));
   OFFSET = FrameDisplacement(Addr);
   switch (M) {
      case LeftMode:
         DecrementFrameSize();
         Op = (ProcLevel(Addr) == 0) ? SGVOP : SLVOP;
         break;
      case RightMode:
         IncrementFrameSize();
         Op = (ProcLevel(Addr) == 0) ? LGVOP : LLVOP;
         break;
   }
   CodeGen1(Op, MakeStringOf(OFFSET), L);
}


int NKids(TreeNode T) { return Rank(T); }


/* ------------------------------------------------------------------ */
/*  Expression code generator                                          */
/* ------------------------------------------------------------------ */
void Expression(TreeNode T, Clabel CurrLabel)
{
   int Kid;
   Clabel Label1;

   if (TraceSpecified) {
      fprintf(TraceFile, "<<< CODE GENERATOR >>> Processing Node ");
      Write_String(TraceFile, NodeName(T));
      fprintf(TraceFile, " , Label is  ");
      Write_String(TraceFile, CurrLabel);
      fprintf(TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case LENode:  
         Expression(Child(T,1), CurrLabel);
         Expression(Child(T,2), NoLabel);
         CodeGen1(BOPOP, BLE, NoLabel);
         DecrementFrameSize();
         break;

      case EQNode: case NENode: case GENode: case LTNode: case GTNode:
      case OrNode: case MultNode: case DivNode: case AndNode:
      case ModNode: case ExpNode:
         Expression(Child(T,1), CurrLabel);
         Expression(Child(T,2), NoLabel);
         if      (NodeName(T)==EQNode)   CodeGen1(BOPOP,BEQ,NoLabel);
         else if (NodeName(T)==NENode)   CodeGen1(BOPOP,BNE,NoLabel);
         else if (NodeName(T)==GENode)   CodeGen1(BOPOP,BGE,NoLabel);
         else if (NodeName(T)==LTNode)   CodeGen1(BOPOP,BLT,NoLabel);
         else if (NodeName(T)==GTNode)   CodeGen1(BOPOP,BGT,NoLabel);
         else if (NodeName(T)==OrNode)   CodeGen1(BOPOP,BOR,NoLabel);
         else if (NodeName(T)==MultNode) CodeGen1(BOPOP,BMULT,NoLabel);
         else if (NodeName(T)==DivNode)  CodeGen1(BOPOP,BDIV,NoLabel);
         else if (NodeName(T)==AndNode)  CodeGen1(BOPOP,BAND,NoLabel);
         else if (NodeName(T)==ModNode)  CodeGen1(BOPOP,BMOD,NoLabel);
         else if (NodeName(T)==ExpNode)  CodeGen1(BOPOP,BEXP,NoLabel);
         DecrementFrameSize();
         break;

      case PlusNode:
         Expression(Child(T,1), CurrLabel);
         if (Rank(T) == 2) {
            Expression(Child(T,2), NoLabel);
            CodeGen1(BOPOP, BPLUS, NoLabel);
            DecrementFrameSize();
         }
         break;

      case NotNode:
         Expression(Child(T,1), CurrLabel);
         CodeGen1(UOPOP, UNOT, NoLabel);
         break;

      case MinusNode:
         Expression(Child(T,1), CurrLabel);
         if (Rank(T) == 2) {
            Expression(Child(T,2), NoLabel);
            CodeGen1(BOPOP, BMINUS, NoLabel);
            DecrementFrameSize();
         } else {
            CodeGen1(UOPOP, UNEG, NoLabel);
         }
         break;

      case EofNode:
         CodeGen1(SOSOP, OSEOF, CurrLabel);
         IncrementFrameSize();
         break;

      case IntegerNode:
         CodeGen1(LITOP, NodeName(Child(T,1)), CurrLabel);
         IncrementFrameSize();
         break;

      case CharNode: {
         /* Extract the character's ASCII value from the literal string, e.g. '#' */
         String litStr = NodeName(Child(T,1));
         int charVal = (int) Character(litStr, 2);   /* skip leading quote */
         CodeGen1(LITOP, MakeStringOf(charVal), CurrLabel);
         IncrementFrameSize();
         break;
      }

      /* true/false kept for safety; normally now come as <identifier> looked up */
      case TrueNode:
         CodeGen1(LITOP, MakeStringOf(1), CurrLabel);
         IncrementFrameSize();
         break;

      case FalseNode:
         CodeGen1(LITOP, MakeStringOf(0), CurrLabel);
         IncrementFrameSize();
         break;

      case IdentifierNode: {
         /* Constrainer marks Child(T,1) decoration:
            < 0  → literal: ordinal = (-mark) - 1
            >= 0 → variable (0 = plain, >0 = enum-typed, handled in AssignNode) */
         int mark = Decoration(Child(T,1));
         if (mark < 0) {
            int ordinal = (-mark) - 1;
            CodeGen1(LITOP, MakeStringOf(ordinal), CurrLabel);
            IncrementFrameSize();
         } else {
            Reference(T, RightMode, CurrLabel);
         }
         break;
      }

      default:
         ReportTreeErrorAt(T);
         printf("<<< CODE GENERATOR >>> : UNKNOWN NODE NAME ");
         Write_String(stdout, NodeName(T));
         printf("\n");

   } /* end switch */
} /* end Expression */


/* ------------------------------------------------------------------ */
/*  ProcessNode – statement/declaration code generator                 */
/* ------------------------------------------------------------------ */
Clabel ProcessNode(TreeNode T, Clabel CurrLabel)
{
   int Kid, Num;
   Clabel Label1, Label2, Label3;

   if (TraceSpecified) {
      fprintf(TraceFile, "<<< CODE GENERATOR >>> Processing Node ");
      Write_String(TraceFile, NodeName(T));
      fprintf(TraceFile, " , Label is  ");
      Write_String(TraceFile, CurrLabel);
      fprintf(TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case ProgramNode:
         /* Children: id, [intrinsic_types], consts, types, dclns, block, id
            NKids-2 = dclns, NKids-1 = block  (works for any count) */
         CurrLabel = ProcessNode(Child(T, NKids(T)-2), CurrLabel);
         CurrLabel = ProcessNode(Child(T, NKids(T)-1), NoLabel);
         return CurrLabel;

      case TypesNode:
         /* No runtime code for type declarations */
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode(Child(T,Kid), CurrLabel);
         return CurrLabel;

      case TypeNode:
         return CurrLabel;   /* No runtime code */

      case ConstsNode:
         return CurrLabel;   /* No runtime code for constants */

      case ConstNode:
         return CurrLabel;   /* No runtime code for constants */

      case LitNode:
         return CurrLabel;   /* No runtime code */

      case DclnsNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode(Child(T,Kid), CurrLabel);
         if (NKids(T) > 0)
            return NoLabel;
         else
            return CurrLabel;

      case DclnNode:
         /* Children: <id>(x), <id>(z), ..., <id>(TypeName)
            Process all but the LAST child (which is the type identifier) */
         for (Kid = 1; Kid < NKids(T); Kid++) {
            if (Kid != 1)
               CodeGen1(LITOP, MakeStringOf(0), NoLabel);
            else
               CodeGen1(LITOP, MakeStringOf(0), CurrLabel);
            Num = MakeAddress();
            Decorate(Child(T,Kid), Num);
            IncrementFrameSize();
         }
         return NoLabel;

      case BlockNode:
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode(Child(T,Kid), CurrLabel);
         return CurrLabel;

      case AssignNode: {
         Expression(Child(T,2), CurrLabel);
         /* Emit LIMIT if LHS is an enum-typed variable.
            Constrainer stores lit-count (>0) on the leaf of the LHS identifier. */
         {
            int litCount = Decoration(Child(Child(T,1), 1));
            if (litCount > 0) {
               /* Stack: [value]  → push lower(0), upper(N-1), LIMIT */
               CodeGen1(LITOP, MakeStringOf(0), NoLabel);
               IncrementFrameSize();
               CodeGen1(LITOP, MakeStringOf(litCount - 1), NoLabel);
               IncrementFrameSize();
               CodeGen0(LIMITOP, NoLabel);
               /* LIMIT pops upper, lower, value and pushes value if in range */
               DecrementFrameSize();
               DecrementFrameSize();
            }
         }
         Reference(Child(T,1), LeftMode, NoLabel);
         return NoLabel;
      }

      case OutputNode: {
         /* First argument gets CurrLabel; emit OUTPUT or OUTPUTC per type */
         int first = 1;
         for (Kid = 1; Kid <= NKids(T); Kid++) {
            TreeNode arg = Child(T, Kid);
            Clabel lbl = first ? CurrLabel : NoLabel;
            first = 0;
            if (NodeName(arg) == CharNode) {
               /* char literal: emit LIT charVal, OUTPUTC */
               String litStr = NodeName(Child(arg, 1));
               int charVal = (int) Character(litStr, 2);
               CodeGen1(LITOP, MakeStringOf(charVal), lbl);
               IncrementFrameSize();
               CodeGen1(SOSOP, OSOUTPUTC, NoLabel);
               DecrementFrameSize();
            } else {
               Expression(arg, lbl);
               CodeGen1(SOSOP, OSOUTPUT, NoLabel);
               DecrementFrameSize();
            }
         }
         CodeGen1(SOSOP, OSOUTPUTL, NoLabel);
         return NoLabel;
      }

      case ReadNode:
         /* read(x, z) – for each variable, INPUT then store */
         for (Kid = 1; Kid <= NKids(T); Kid++) {
            Clabel lbl = (Kid == 1) ? CurrLabel : NoLabel;
            CodeGen1(SOSOP, OSINPUT, lbl);
            IncrementFrameSize();
            Reference(Child(T, Kid), LeftMode, NoLabel);
         }
         return NoLabel;

      case IfNode:
         Expression(Child(T,1), CurrLabel);
         Label1 = MakeLabel();
         Label2 = MakeLabel();
         Label3 = MakeLabel();
         CodeGen2(CONDOP, Label1, Label2, NoLabel);
         DecrementFrameSize();
         CodeGen1(GOTOOP, Label3, ProcessNode(Child(T,2), Label1));
         if (Rank(T) == 3)
            CodeGen0(NOP, ProcessNode(Child(T,3), Label2));
         else
            CodeGen0(NOP, Label2);
         return Label3;

      case WhileNode: {
         if (CurrLabel == NoLabel)
            Label1 = MakeLabel();
         else
            Label1 = CurrLabel;
         Label2 = MakeLabel();
         Label3 = MakeLabel();
         Expression(Child(T,1), Label1);
         CodeGen2(CONDOP, Label2, Label3, NoLabel);
         DecrementFrameSize();
         CodeGen1(GOTOOP, Label1, ProcessNode(Child(T,2), Label2));
         return Label3;
      }

      case ForToNode:
      case ForDowntoNode: {
         Clabel LoopTest  = MakeLabel();
         Clabel BodyStart = MakeLabel();
         Clabel LoopEnd   = MakeLabel();
         Clabel BodyExit;
         Expression(Child(T,2), CurrLabel);
         Reference(Child(T,1), LeftMode, NoLabel);
         CodeGen1(GOTOOP, LoopTest, NoLabel);
         BodyExit = ProcessNode(Child(T,4), BodyStart);
         if (BodyExit != NoLabel) CodeGen0(NOP, BodyExit);
         Reference(Child(T,1), RightMode, NoLabel);
         CodeGen1(LITOP, MakeStringOf(1), NoLabel);
         IncrementFrameSize();
         if (NodeName(T) == ForToNode)
            CodeGen1(BOPOP, BPLUS, NoLabel);
         else
            CodeGen1(BOPOP, BMINUS, NoLabel);
         DecrementFrameSize();
         Reference(Child(T,1), LeftMode, NoLabel);
         CodeGen0(NOP, LoopTest);
         Reference(Child(T,1), RightMode, NoLabel);
         Expression(Child(T,3), NoLabel);
         if (NodeName(T) == ForToNode)
            CodeGen1(BOPOP, BLE, NoLabel);
         else
            CodeGen1(BOPOP, BGE, NoLabel);
         DecrementFrameSize();
         CodeGen2(CONDOP, BodyStart, LoopEnd, NoLabel);
         DecrementFrameSize();
         return LoopEnd;
      }

      case RepeatNode: {
         Clabel LoopStart = MakeLabel();
         Clabel StmtLabel = NoLabel;
         Clabel LoopEnd;
         CodeGen0(NOP, LoopStart);
         for (Kid = 1; Kid < NKids(T); Kid++)
            StmtLabel = ProcessNode(Child(T,Kid), StmtLabel);
         Expression(Child(T,NKids(T)), StmtLabel);
         LoopEnd = MakeLabel();
         CodeGen2(CONDOP, LoopEnd, LoopStart, NoLabel);
         DecrementFrameSize();
         return LoopEnd;
      }

      case CaseNode: {
         Clabel EndLabel = MakeLabel();
         Clabel NextCaseLabel, ClauseExit;
         int i;
         TreeNode OtherwisePtr = Child(T, NKids(T));
         Expression(Child(T,1), CurrLabel);
         for (i = 2; i < NKids(T); i++) {
            TreeNode Clause = Child(T, i);
            NextCaseLabel = MakeLabel();
            Clabel MatchLabel = MakeLabel();
            if (NodeName(Clause) == CaseClauseRangeNode) {
               CodeGen0(DUPOP, NoLabel); IncrementFrameSize();
               CodeGen0(DUPOP, NoLabel); IncrementFrameSize();
               CodeGen1(LITOP, NodeName(Child(Clause,1)), NoLabel); IncrementFrameSize();
               CodeGen1(BOPOP, BGE, NoLabel); DecrementFrameSize();
               CodeGen0(SWAPOP, NoLabel);
               CodeGen1(LITOP, NodeName(Child(Clause,2)), NoLabel); IncrementFrameSize();
               CodeGen1(BOPOP, BLE, NoLabel); DecrementFrameSize();
               CodeGen1(BOPOP, BAND, NoLabel); DecrementFrameSize();
            } else {
               CodeGen0(DUPOP, NoLabel); IncrementFrameSize();
               CodeGen1(LITOP, NodeName(Child(Clause,1)), NoLabel); IncrementFrameSize();
               CodeGen1(BOPOP, BEQ, NoLabel); DecrementFrameSize();
            }
            CodeGen2(CONDOP, MatchLabel, NextCaseLabel, NoLabel);
            DecrementFrameSize();
            CodeGen0(NOP, MatchLabel);
            CodeGen0(POPOP, NoLabel); DecrementFrameSize();
            if (NodeName(Clause) == CaseClauseRangeNode)
               ClauseExit = ProcessNode(Child(Clause,3), NoLabel);
            else
               ClauseExit = ProcessNode(Child(Clause,2), NoLabel);
            if (ClauseExit != NoLabel) CodeGen0(NOP, ClauseExit);
            CodeGen1(GOTOOP, EndLabel, NoLabel);
            CodeGen0(NOP, NextCaseLabel);
         }
         CodeGen0(POPOP, NoLabel); DecrementFrameSize();
         if (NodeName(OtherwisePtr) != NullNode) {
            ClauseExit = ProcessNode(Child(OtherwisePtr,1), NoLabel);
            if (ClauseExit != NoLabel) CodeGen0(NOP, ClauseExit);
         }
         CodeGen0(NOP, EndLabel);
         return NoLabel;
      }

      case SwapNode:
         Reference(Child(T,1), RightMode, CurrLabel);
         Reference(Child(T,2), RightMode, NoLabel);
         Reference(Child(T,1), LeftMode,  NoLabel);
         Reference(Child(T,2), LeftMode,  NoLabel);
         return NoLabel;

      case LoopNode: {
         Clabel LoopStart = MakeLabel();
         Clabel LoopEnd   = MakeLabel();
         Clabel StmtLabel = CurrLabel;
         CodeGen0(NOP, LoopStart);
         PushLoopExitLabel(LoopEnd);
         for (Kid = 1; Kid <= NKids(T); Kid++)
            StmtLabel = ProcessNode(Child(T,Kid), StmtLabel);
         if (StmtLabel != NoLabel) CodeGen0(NOP, StmtLabel);
         PopLoopExitLabel();
         CodeGen1(GOTOOP, LoopStart, NoLabel);
         CodeGen0(NOP, LoopEnd);
         return NoLabel;
      }

      case ExitNode: {
         Clabel ExitLabel = CurrentLoopExitLabel();
         if (ExitLabel == NoLabel)
            printf("<<< CODE GENERATOR ERROR >>> : EXIT NOT INSIDE LOOP\n");
         else
            CodeGen1(GOTOOP, ExitLabel, CurrLabel);
         return NoLabel;
      }

      case NullNode: return CurrLabel;

      default:
         ReportTreeErrorAt(T);
         printf("<<< CODE GENERATOR >>> : UNKNOWN NODE NAME ");
         Write_String(stdout, NodeName(T));
         printf("\n");
         return NoLabel;

   } /* end switch */
}   /* end ProcessNode */