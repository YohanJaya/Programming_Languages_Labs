/*******************************************************************
          Copyright (C) 1986 by Manuel E. Bermudez
          Translated to C - 1991
          Lab 5 Extensions: char type, const, user-defined/enumerated
          types, LIMIT instruction, succ/pred/ord/chr, read statement,
          char/string output.
********************************************************************/

#include <stdio.h>
#include <header/CommandLine.h>
#include <header/Open_File.h>
#include <header/Table.h>
#include <header/Text.h>
#include <header/Error.h>
#include <header/String_Input.h>
#include <header/Tree.h>
#include <header/Dcln.h>
#include <header/Code.h>
#include <header/CodeGenerator.h>  

#define LeftMode  0
#define RightMode 1

    /*  ABSTRACT MACHINE OPERATIONS  */
#define    NOP          1   /* 'NOP'       */
#define    HALTOP       2   /* 'HALT'      */      
#define    LITOP        3   /* 'LIT'       */
#define    LLVOP        4   /* 'LLV'       */
#define    LGVOP        5   /* 'LGV'       */
#define    SLVOP        6   /* 'SLV'       */
#define    SGVOP        7   /* 'SGV'       */
#define    LLAOP        8   /* 'LLA'       */
#define    LGAOP        9   /* 'LGA'       */
#define    UOPOP       10   /* 'UOP'       */
#define    BOPOP       11   /* 'BOP'       */     
#define    POPOP       12   /* 'POP'       */  
#define    DUPOP       13   /* 'DUP'       */
#define    SWAPOP      14   /* 'SWAP'      */
#define    CALLOP      15   /* 'CALL'      */
#define    RTNOP       16   /* 'RTN'       */
#define    GOTOOP      17   /* 'GOTO'      */
#define    CONDOP      18   /* 'COND'      */
#define    CODEOP      19   /* 'CODE'      */
#define    SOSOP       20   /* 'SOS'       */
#define    LIMITOP     21   /* 'LIMIT'     */

    /* ABSTRACT MACHINE OPERANDS */
#define    UNOT        22   /* 'UNOT'     */
#define    UNEG        23   /* 'UNEG'     */
#define    USUCC       24   /* 'USUCC'    */
#define    UPRED       25   /* 'UPRED'    */
#define    BAND        26   /* 'BAND'     */
#define    BOR         27   /* 'BOR'      */
#define    BPLUS       28   /* 'BPLUS'    */
#define    BMINUS      29   /* 'BMINUS'   */
#define    BMULT       30   /* 'BMULT'    */
#define    BDIV        31   /* 'BDIV'     */
#define    BEXP        32   /* 'BEXP'     */
#define    BMOD        33   /* 'BMOD'     */
#define    BEQ         34   /* 'BEQ'      */
#define    BNE         35   /* 'BNE'      */
#define    BLE         36   /* 'BLE'      */
#define    BGE         37   /* 'BGE'      */
#define    BLT         38   /* 'BLT'      */
#define    BGT         39   /* 'BGT'      */
#define    TRACEX      40   /* 'TRACEX'   */
#define    DUMPMEM     41   /* 'DUMPMEM'  */
#define    OSINPUT     42   /* 'INPUT'    */
#define    OSINPUTC    43   /* 'INPUTC'   */
#define    OSOUTPUT    44   /* 'OUTPUT'   */
#define    OSOUTPUTC   45   /* 'OUTPUTC'  */
#define    OSOUTPUTL   46   /* 'OUTPUTL'  */
#define    OSEOF       47   /* 'EOF'      */
#define    ProgramNode    48
#define    TypesNode      49
#define    TypeNode       50
#define    DclnsNode      51
#define    DclnNode       52
#define    IntegerTNode   53
#define    BooleanTNode   54
#define    BlockNode      55
#define    AssignNode     56
#define    OutputNode     57
#define    IfNode         58
#define    WhileNode      59
#define    NullNode       60
#define    LENode         61
#define    PlusNode       62
#define    MinusNode      63
#define    ReadNode       64
#define    IntegerNode    65
#define    IdentifierNode 66
#define    TrueNode       67
#define    FalseNode      68
#define    EQNode         69
#define    NENode         70
#define    GENode         71
#define    LTNode         72
#define    GTNode         73
#define    OrNode         74
#define    MultNode       75
#define    DivNode        76
#define    AndNode        77
#define    ModNode        78
#define    ExpNode        79
#define    NotNode        80
#define    EofNode        81
#define    ForToNode      82
#define    ForDowntoNode  83
#define    RepeatNode     84
#define    CaseNode       85
#define    CaseClauseNode 86
#define    CaseClauseRangeNode 87
#define    SwapNode       88
#define    LoopNode       89
#define    ExitNode       90
#define    OtherwiseNode  91
#define    ConstsNode     92  /* "consts"    */
#define    ConstNode      93  /* "const"     */
#define    CharTNode      94  /* "char"      */
#define    LitNode        95  /* "lit"       */
#define    CharNode       96  /* "<char>"    */
#define    StringNode     97  /* "<string>"  */
#define    SuccNode       98  /* "succ"      */
#define    PredNode       99  /* "pred"      */
#define    OrdNode       100  /* "ord"       */
#define    ChrNode       101  /* "chr"       */
#define    ReadItemNode  102  /* "read_item" */

#define NumberOfNodes 102

#define DecorIntegerTNode 6
#define DecorBooleanTNode 7
#define DecorCharTNode    47

typedef int Mode;

FILE *CodeFile;
char *CodeFileName;
Clabel HaltLabel;
static int AddressIsChar[10000];
static TreeNode AddressType[10000];

#define MAX_LOOP_NESTING 128
static Clabel LoopExitLabelStack[MAX_LOOP_NESTING];
static int LoopExitTop = -1;

void PushLoopExitLabel(Clabel L)
{
   if (LoopExitTop < MAX_LOOP_NESTING - 1)
      LoopExitLabelStack[++LoopExitTop] = L;
}

void PopLoopExitLabel(void)
{
   if (LoopExitTop >= 0)
      LoopExitTop--;
}

Clabel CurrentLoopExitLabel(void)
{
   if (LoopExitTop >= 0)
      return LoopExitLabelStack[LoopExitTop];
   return NoLabel;
}

char *mach_op[] = 
    {"NOP","HALT","LIT","LLV","LGV","SLV","SGV","LLA","LGA",
     "UOP","BOP","POP","DUP","SWAP","CALL","RTN","GOTO","COND",
     "CODE","SOS","LIMIT","UNOT","UNEG","USUCC","UPRED","BAND",
     "BOR","BPLUS","BMINUS","BMULT","BDIV","BEXP","BMOD","BEQ",
     "BNE","BLE","BGE","BLT","BGT","TRACEX","DUMPMEM","INPUT",
     "INPUTC","OUTPUT","OUTPUTC","OUTPUTL","EOF"};

char *node_name[] = {
    "program","types","type","dclns","dcln","integer",
    "boolean","block","assign","output","if","while",
    "<null>","<=","+","-","read","<integer>","<identifier>",
    "<true>","<false>","=","<>",">=","<",">","or",
    "*","/","and","mod","**","not","eof","for_to","for_downto",
    "repeat","case","case_clause","case_clause_range",
    "swap","loop","exit","otherwise",
    /* Lab 5 */
    "consts","const","char","lit","<char>","<string>",
    "succ","pred","ord","chr","read_item"
};


void CodeGenerate(int argc, char *argv[])
{
   int NumberTrees;

   InitializeCodeGenerator(argc,argv);
   Tree_File = Open_File("_TREE", "r"); 
   NumberTrees = Read_Trees();
   fclose (Tree_File);                     

   HaltLabel = ProcessNode (RootOfTree(1), NoLabel);
   CodeGen0 (HALTOP, HaltLabel); 

   CodeFile = Open_File (CodeFileName, "w");
   DumpCode (CodeFile);
   fclose(CodeFile); 

   if (TraceSpecified)
      fclose (TraceFile);

   Tree_File = fopen("_TREE", "w");  
   Write_Trees();
   fclose (Tree_File);                   
}


void InitializeCodeGenerator(int argc,char *argv[])
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
      String_Array_To_String_Constant (mach_op[i],j);
}


void InitializeNodeNames(void)
{
   int i,j;
   for (i=0, j=48; j <= NumberOfNodes; i++, j++)
      String_Array_To_String_Constant (node_name[i],j);
}


String MakeStringOf(int Number)
{
   Stack Temp;

   Temp = AllocateStack (50);
   ResetBufferInTextTable();
   if (Number == 0)
      AdvanceOnCharacter ('0');
   else
   {
      while (Number > 0)
      {
         Push (Temp,(Number % 10) + 48);
         Number /= 10;
      }
      while ( !(IsEmpty (Temp)))
         AdvanceOnCharacter ((char)(Pop(Temp)));
   }   
   return (ConvertStringInBuffer()); 
}  

int EscapedCharValue(char C)
{
   switch (C)
   {
      case 'n'  : return '\n';
      case 't'  : return '\t';
      case 'r'  : return '\r';
      case 'b'  : return '\b';
      case 'f'  : return '\f';
      case '\\' : return '\\';
      case '\'' : return '\'';
      case '"'  : return '"';
      default   : return (unsigned char)C;
   }
}

int CharLiteralToInt(String S)
{
   char buf[8];
   int len = 0;

   FILE *tmp = tmpfile();
   if (tmp) {
       Write_String(tmp, S);
       rewind(tmp);
       while (len < 7 && (buf[len] = fgetc(tmp)) != EOF) len++;
       buf[len] = '\0';
       fclose(tmp);
   }

   if (len >= 4 && buf[1] == '\\')
      return EscapedCharValue(buf[2]);
   if (len >= 2) return (unsigned char)buf[1];
   return 0;
}


void Reference(TreeNode T, Mode M, Clabel L)
{
   int Addr,OFFSET;
   String  Op;

   Addr = Decoration(Decoration(T));
   OFFSET = FrameDisplacement (Addr) ;
   switch (M)
   {
      case LeftMode  :  DecrementFrameSize();
                        if (ProcLevel (Addr) == 0) 
                           Op = SGVOP;
                        else
                           Op = SLVOP;
                        break;
      case RightMode :  IncrementFrameSize();
                        if (ProcLevel (Addr) == 0) 
                           Op = LGVOP;
                        else
                           Op = LLVOP;
                        break;
   }
   CodeGen1 (Op,MakeStringOf(OFFSET),L);
}


int NKids (TreeNode T)
{
   return (Rank(T));
}

void Expression (TreeNode T, Clabel CurrLabel);


void EmitLimitCheck(int LowerBound, int UpperBound)
{
   CodeGen1(LITOP, MakeStringOf(LowerBound), NoLabel);
   IncrementFrameSize();
   CodeGen1(LITOP, MakeStringOf(UpperBound), NoLabel);
   IncrementFrameSize();
   CodeGen0(LIMITOP, NoLabel);
   DecrementFrameSize();
   DecrementFrameSize();
}


void EmitCaseValue(TreeNode T, Clabel L)
{
   if (NodeName(T) == CharNode)
      CodeGen1(LITOP, MakeStringOf(CharLiteralToInt(NodeName(Child(T,1)))), L);
   else if (NodeName(T) == TrueNode)
      CodeGen1(LITOP, MakeStringOf(1), L);
   else if (NodeName(T) == FalseNode)
      CodeGen1(LITOP, MakeStringOf(0), L);
   else if (NodeName(T) == IdentifierNode)
   {
      Expression(T, L);
      return;
   }
   else
      CodeGen1(LITOP, NodeName(Child(T,1)), L);
   IncrementFrameSize();
}


int IsEnumeratedType(TreeNode Type)
{
   return (Type > 0 &&
           NodeName(Type) == TypeNode &&
           NKids(Type) >= 2 &&
           NodeName(Child(Type,2)) == LitNode);
}


int IsIntegerType(TreeNode Type)
{
   return (!IsEnumeratedType(Type) &&
           (Type == IntegerTNode || Type == DecorIntegerTNode));
}


int IsBooleanType(TreeNode Type)
{
   return (!IsEnumeratedType(Type) &&
           (Type == BooleanTNode || Type == DecorBooleanTNode));
}


int IsCharType(TreeNode Type)
{
   return (!IsEnumeratedType(Type) &&
           (Type == CharTNode || Type == DecorCharTNode));
}


int EnumUpperBound(TreeNode Type)
{
   if (IsEnumeratedType(Type))
      return NKids(Child(Type,2)) - 1;
   return 0;
}


int EnumLiteralOrdinal(TreeNode Decl)
{
   TreeNode Type;
   TreeNode Lit;
   int Kid;

   if (Decl == NullDeclaration || NodeName(Decl) != IdentifierNode)
      return -1;

   Type = Decoration(Decl);
   if (!IsEnumeratedType(Type))
      return -1;

   Lit = Child(Type,2);
   for (Kid = 1; Kid <= NKids(Lit); Kid++)
      if (Child(Lit,Kid) == Decl)
         return Kid - 1;

   return -1;
}


TreeNode IdentifierType(TreeNode T)
{
   TreeNode Decl;
   int Addr;

   Decl = Decoration(T);
   if (Decl == NullDeclaration)
      return 0;

   if (NodeName(Decl) == ConstNode)
      return Decoration(Decl);

   if (EnumLiteralOrdinal(Decl) >= 0)
      return Decoration(Decl);

   Addr = Decoration(Decl);
   if (Addr >= 0)
      return AddressType[FrameDisplacement(Addr)];

   return 0;
}


TreeNode ExpressionType(TreeNode T)
{
   switch (NodeName(T))
   {
      case IntegerNode :
      case PlusNode :
      case MinusNode :
      case MultNode :
      case DivNode :
      case ModNode :
      case ExpNode :
      case OrdNode :
         return DecorIntegerTNode;

      case CharNode :
      case ChrNode :
         return DecorCharTNode;

      case TrueNode :
      case FalseNode :
      case EofNode :
      case LENode :
      case EQNode :
      case NENode :
      case GENode :
      case LTNode :
      case GTNode :
      case OrNode :
      case AndNode :
      case NotNode :
         return DecorBooleanTNode;

      case SuccNode :
      case PredNode :
         return ExpressionType(Child(T,1));

      case IdentifierNode :
         return IdentifierType(T);

      default :
         return 0;
   }
}


int IsCharIdentifier(TreeNode T)
{
   if (NodeName(T) != IdentifierNode)
      return 0;

   return IsCharType(IdentifierType(T));
}


int IsCharExpression(TreeNode T)
{
   return IsCharType(ExpressionType(T));
}


void Expression (TreeNode T, Clabel CurrLabel)
{
   int Kid;
   Clabel Label1;

   if (TraceSpecified)
   {
      fprintf (TraceFile, "<<< CODE GENERATOR >>> Processing Node ");
      Write_String (TraceFile, NodeName (T) );
      fprintf (TraceFile, " , Label is  ");
      Write_String (TraceFile, CurrLabel);
      fprintf (TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case LENode :  
         Expression ( Child(T,1) , CurrLabel);
         Expression ( Child(T,2) , NoLabel);
         CodeGen1 (BOPOP, BLE, NoLabel);
         DecrementFrameSize();
         break;

      case EQNode :
      case NENode :
      case GENode :
      case LTNode :
      case GTNode :
      case OrNode :
      case MultNode :
      case DivNode :
      case AndNode :
      case ModNode :
      case ExpNode :
         Expression ( Child(T,1) , CurrLabel);
         Expression ( Child(T,2) , NoLabel);
         if (NodeName(T) == EQNode) CodeGen1(BOPOP, BEQ, NoLabel);
         else if (NodeName(T) == NENode) CodeGen1(BOPOP, BNE, NoLabel);
         else if (NodeName(T) == GENode) CodeGen1(BOPOP, BGE, NoLabel);
         else if (NodeName(T) == LTNode) CodeGen1(BOPOP, BLT, NoLabel);
         else if (NodeName(T) == GTNode) CodeGen1(BOPOP, BGT, NoLabel);
         else if (NodeName(T) == OrNode) CodeGen1(BOPOP, BOR, NoLabel);
         else if (NodeName(T) == MultNode) CodeGen1(BOPOP, BMULT, NoLabel);
         else if (NodeName(T) == DivNode) CodeGen1(BOPOP, BDIV, NoLabel);
         else if (NodeName(T) == AndNode) CodeGen1(BOPOP, BAND, NoLabel);
         else if (NodeName(T) == ModNode) CodeGen1(BOPOP, BMOD, NoLabel);
         else if (NodeName(T) == ExpNode) CodeGen1(BOPOP, BEXP, NoLabel);
         DecrementFrameSize();
         break;

      case PlusNode :
         Expression ( Child(T,1) , CurrLabel);
         if (Rank(T) == 2)
         {
            Expression ( Child(T,2) , NoLabel);
            CodeGen1 (BOPOP, BPLUS, NoLabel);
            DecrementFrameSize();
         }
         break;

      case NotNode :
         Expression ( Child(T,1) , CurrLabel);
         CodeGen1 (UOPOP, UNOT, NoLabel);
         break;
      
      case MinusNode :
         Expression ( Child(T,1) , CurrLabel);
         if (Rank(T) == 2)
         {
            Expression ( Child(T,2) , NoLabel);
            CodeGen1 (BOPOP, BMINUS, NoLabel);
            DecrementFrameSize();
         }
         else
            CodeGen1 (UOPOP, UNEG, NoLabel);
         break;

      case EofNode :
         CodeGen1 (SOSOP, OSEOF, CurrLabel);
         IncrementFrameSize();
         break;

      case IntegerNode :
         CodeGen1 (LITOP, NodeName (Child(T,1)), CurrLabel);
         IncrementFrameSize();
         break;

      case CharNode :
      {
         String RawStr = NodeName(Child(T,1));
         int CharVal = CharLiteralToInt(RawStr);
         CodeGen1 (LITOP, MakeStringOf(CharVal), CurrLabel);
         IncrementFrameSize();
         break;
      }

      case TrueNode :
         CodeGen1 (LITOP, MakeStringOf(1), CurrLabel);
         IncrementFrameSize();
         break;

      case FalseNode :
         CodeGen1 (LITOP, MakeStringOf(0), CurrLabel);
         IncrementFrameSize();
         break;

      case SuccNode :
      {
         TreeNode ExprType = ExpressionType(Child(T,1));
         Expression (Child(T,1), CurrLabel);
         CodeGen1 (UOPOP, USUCC, NoLabel);
         if (IsCharType(ExprType))
            EmitLimitCheck(0, 255);
         else if (IsBooleanType(ExprType))
            EmitLimitCheck(0, 1);
         else if (IsEnumeratedType(ExprType))
            EmitLimitCheck(0, EnumUpperBound(ExprType));
         break;
      }

      case PredNode :
      {
         TreeNode ExprType = ExpressionType(Child(T,1));
         Expression (Child(T,1), CurrLabel);
         CodeGen1 (UOPOP, UPRED, NoLabel);
         if (IsCharType(ExprType))
            EmitLimitCheck(0, 255);
         else if (IsBooleanType(ExprType))
            EmitLimitCheck(0, 1);
         else if (IsEnumeratedType(ExprType))
            EmitLimitCheck(0, EnumUpperBound(ExprType));
         break;
      }

      case OrdNode :
         Expression (Child(T,1), CurrLabel);
         break;

      case ChrNode :
         Expression (Child(T,1), CurrLabel);
         break;

      case IdentifierNode :
      {
         TreeNode Decl = Decoration(T);
         int Ordinal;
         if (Decl != NullDeclaration && NodeName(Decl) == ConstNode)
            Expression(Child(Decl,2), CurrLabel);
         else if ((Ordinal = EnumLiteralOrdinal(Decl)) >= 0)
         {
            CodeGen1 (LITOP, MakeStringOf(Ordinal), CurrLabel);
            IncrementFrameSize();
         }
         else
            Reference (T,RightMode,CurrLabel);
         break;
      }

      default :
         ReportTreeErrorAt(T);
         printf ("<<< CODE GENERATOR >>> : UNKNOWN EXPRESSION NODE NAME ");
         Write_String (stdout,NodeName(T));
         printf ("\n");

   } /* end switch */
} /* end Expression */



Clabel ProcessNode (TreeNode T, Clabel CurrLabel)
{
   int Kid, Num;
   Clabel Label1, Label2, Label3;

   if (TraceSpecified)
   {
      fprintf (TraceFile, "<<< CODE GENERATOR >>> Processing Node ");
      Write_String (TraceFile, NodeName (T) );
      fprintf (TraceFile, " , Label is  ");
      Write_String (TraceFile, CurrLabel);
      fprintf (TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case ProgramNode :
         CurrLabel = ProcessNode (Child(T,NKids(T)-2),CurrLabel);
         CurrLabel = ProcessNode (Child(T,NKids(T)-1),NoLabel);
         return (CurrLabel);

      case ConstsNode :
         return (CurrLabel);

      case ConstNode :
         return (CurrLabel);

      case TypesNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode (Child(T,Kid), CurrLabel);
         return (CurrLabel);

      case TypeNode :
         return (CurrLabel);

      case DclnsNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode (Child(T,Kid), CurrLabel);
         if (NKids(T) > 0)
            return (NoLabel);
         else
            return (CurrLabel);

      case DclnNode :
      {
         TreeNode VarType;
         for (Kid = 1; Kid < NKids(T); Kid++)
         {
            VarType = Decoration(Child(T,Kid));
            if (Kid != 1)
               CodeGen1 (LITOP,MakeStringOf(0),NoLabel);
            else
               CodeGen1 (LITOP,MakeStringOf(0),CurrLabel);
            Num = MakeAddress();
            Decorate ( Child(T,Kid), Num);
            AddressType[FrameDisplacement(Num)] = VarType;
            AddressIsChar[FrameDisplacement(Num)] = IsCharType(VarType);
            IncrementFrameSize();
         }
         return (NoLabel);                 
      }

      case BlockNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode (Child(T,Kid), CurrLabel);
         return (CurrLabel); 

      case AssignNode :
         Expression (Child(T,2), CurrLabel);
         Reference (Child(T,1), LeftMode, NoLabel);
         return (NoLabel);

      case ReadNode :
      {
         int IsFirst = 1;
         for (Kid = 1; Kid <= NKids(T); Kid++)
         {
            TreeNode ReadItem = Child(T, Kid);
            TreeNode VarIdNode = Child(ReadItem, 1);

            if (IsCharIdentifier(VarIdNode))
                CodeGen1(SOSOP, OSINPUTC, IsFirst ? CurrLabel : NoLabel);
            else
                CodeGen1(SOSOP, OSINPUT, IsFirst ? CurrLabel : NoLabel);
            IncrementFrameSize();
            Reference(VarIdNode, LeftMode, NoLabel);
            IsFirst = 0;
         }
         return (NoLabel);
      }

      /* ---- output statement ---- */
      case OutputNode :
      {
         int IsFirst = 1;
         for (Kid = 1; Kid <= NKids(T); Kid++)
         {
            TreeNode OutChild = Child(T, Kid);
            Clabel L = IsFirst ? CurrLabel : NoLabel;
            if (NodeName(OutChild) == StringNode)
            {

               String RawStr = NodeName(Child(OutChild, 1));
               char buf[512];
               int slen = 0;
               FILE *tmp2 = tmpfile();
               if (tmp2) {
                   Write_String(tmp2, RawStr);
                   rewind(tmp2);
                   while (slen < 510 && (buf[slen] = fgetc(tmp2)) != EOF) slen++;
                   buf[slen] = '\0';
                   fclose(tmp2);
               }
               
               int ci;
               int CharVal;
               for (ci = 1; ci < slen - 1; ci++) {
                  if (buf[ci] == '\\' && ci + 1 < slen - 1)
                  {
                     ci++;
                     CharVal = EscapedCharValue(buf[ci]);
                  }
                  else
                     CharVal = (unsigned char)buf[ci];
                  CodeGen1(LITOP, MakeStringOf(CharVal), L);
                  IncrementFrameSize();
                  CodeGen1(SOSOP, OSOUTPUTC, NoLabel);
                  DecrementFrameSize();
                  L = NoLabel;
               }
            }
            else
            {
               int IsChar;
               Expression(OutChild, L);
               
               IsChar = IsCharExpression(OutChild);
               if (IsChar)
                   CodeGen1(SOSOP, OSOUTPUTC, NoLabel);
               else
                   CodeGen1(SOSOP, OSOUTPUT, NoLabel);
               DecrementFrameSize();
            }
            IsFirst = 0;
         }
         CodeGen1(SOSOP, OSOUTPUTL, NoLabel);
         return (NoLabel);
      }

      case IfNode :
         Expression (Child(T,1), CurrLabel);
         Label1 = MakeLabel();
         Label2 = MakeLabel();
         Label3 = MakeLabel();
         CodeGen2 (CONDOP,Label1,Label2, NoLabel);
         DecrementFrameSize();
         CodeGen1 (GOTOOP, Label3, ProcessNode (Child(T,2), Label1) );
         if (Rank(T) == 3)
            CodeGen0 (NOP, ProcessNode (Child(T,3),Label2));
         else
            CodeGen0 (NOP, Label2);
         return (Label3);                

      case WhileNode :
         if (CurrLabel == NoLabel) 
            Label1 = MakeLabel();
         else 
            Label1 = CurrLabel;
         Label2 = MakeLabel();
         Label3 = MakeLabel();
         Expression (Child(T,1), Label1);
         CodeGen2 (CONDOP, Label2, Label3, NoLabel);
         DecrementFrameSize();
         CodeGen1 (GOTOOP, Label1, ProcessNode (Child(T,2), Label2) );
         return (Label3);
      
      case ForToNode :
      case ForDowntoNode :
      {
         Clabel LoopTest  = MakeLabel();
         Clabel BodyStart = MakeLabel();
         Clabel LoopEnd   = MakeLabel();
         Clabel BodyExit;
         
         Expression(Child(T, 2), CurrLabel);
         Reference(Child(T, 1), LeftMode, NoLabel);
         
         CodeGen1(GOTOOP, LoopTest, NoLabel);
         BodyExit = ProcessNode(Child(T, 4), BodyStart);
         if (BodyExit != NoLabel)
            CodeGen0(NOP, BodyExit);
         
         Reference(Child(T, 1), RightMode, NoLabel);
         CodeGen1(LITOP, MakeStringOf(1), NoLabel);
         IncrementFrameSize();
         
         if (NodeName(T) == ForToNode)
             CodeGen1(BOPOP, BPLUS, NoLabel);
         else
             CodeGen1(BOPOP, BMINUS, NoLabel);
         DecrementFrameSize();
         Reference(Child(T, 1), LeftMode, NoLabel);
         
         CodeGen0(NOP, LoopTest);
         Reference(Child(T, 1), RightMode, NoLabel);
         Expression(Child(T, 3), NoLabel);
         
         if (NodeName(T) == ForToNode)
             CodeGen1(BOPOP, BLE, NoLabel);
         else
             CodeGen1(BOPOP, BGE, NoLabel);
         DecrementFrameSize();
         
         CodeGen2(CONDOP, BodyStart, LoopEnd, NoLabel);
         DecrementFrameSize();
         
         return (LoopEnd);
      }

      case RepeatNode :
      {
         Clabel LoopStart = MakeLabel();
         CodeGen0(NOP, LoopStart);
         for (Kid = 1; Kid < NKids(T); Kid++)
             ProcessNode(Child(T, Kid), NoLabel);
         Expression(Child(T, NKids(T)), NoLabel);
         Clabel LoopEnd = MakeLabel();
         CodeGen2(CONDOP, LoopEnd, LoopStart, NoLabel);
         DecrementFrameSize();
         return (LoopEnd);
      }

      case CaseNode :
      {
         Clabel EndLabel = MakeLabel();
         Clabel NextCaseLabel;
         int i;
         TreeNode OtherwiseNodePtr = Child(T, NKids(T));

         Expression(Child(T, 1), CurrLabel);
         
         for (i = 2; i < NKids(T); i++) {
             TreeNode Clause = Child(T, i);
             Clabel ClauseExit;
             NextCaseLabel  = MakeLabel();
             Clabel MatchLabel = MakeLabel();

             if (NodeName(Clause) == CaseClauseRangeNode)
             {
                CodeGen0(DUPOP, NoLabel);
                IncrementFrameSize();
                CodeGen0(DUPOP, NoLabel);
                IncrementFrameSize();

                EmitCaseValue(Child(Clause, 1), NoLabel);
                CodeGen1(BOPOP, BGE, NoLabel);
                DecrementFrameSize();

                CodeGen0(SWAPOP, NoLabel);

                EmitCaseValue(Child(Clause, 2), NoLabel);
                CodeGen1(BOPOP, BLE, NoLabel);
                DecrementFrameSize();

                CodeGen1(BOPOP, BAND, NoLabel);
                DecrementFrameSize();
             }
             else
             {
                CodeGen0(DUPOP, NoLabel);
                IncrementFrameSize();

                EmitCaseValue(Child(Clause, 1), NoLabel);
                CodeGen1(BOPOP, BEQ, NoLabel);
                DecrementFrameSize();
             }

             CodeGen2(CONDOP, MatchLabel, NextCaseLabel, NoLabel);
             DecrementFrameSize();

             CodeGen0(NOP, MatchLabel);
             CodeGen0(POPOP, NoLabel);
             DecrementFrameSize();
             if (NodeName(Clause) == CaseClauseRangeNode)
                ClauseExit = ProcessNode(Child(Clause, 3), NoLabel);
             else
                ClauseExit = ProcessNode(Child(Clause, 2), NoLabel);
             if (ClauseExit != NoLabel)
                CodeGen0(NOP, ClauseExit);
             CodeGen1(GOTOOP, EndLabel, NoLabel);

             CodeGen0(NOP, NextCaseLabel);
         }
         
         CodeGen0(POPOP, NoLabel);
         DecrementFrameSize();

         if (NodeName(OtherwiseNodePtr) != NullNode)
         {
             Clabel OtherwiseExit = ProcessNode(Child(OtherwiseNodePtr, 1), NoLabel);
             if (OtherwiseExit != NoLabel)
                CodeGen0(NOP, OtherwiseExit);
         }
         
         CodeGen0(NOP, EndLabel);
         return (NoLabel);
      }

      case SwapNode :
         Reference(Child(T,1), RightMode, CurrLabel); 
         Reference(Child(T,2), RightMode, NoLabel);   
         CodeGen0(SWAPOP, NoLabel);                   
         Reference(Child(T,2), LeftMode, NoLabel);    
         Reference(Child(T,1), LeftMode, NoLabel);    
         return (NoLabel);

      case LoopNode :
      {
         Clabel LoopStart = MakeLabel();
         Clabel LoopEnd   = MakeLabel();
         Clabel StmtLabel = NoLabel;
         CodeGen0(NOP, LoopStart);
         PushLoopExitLabel(LoopEnd);
         for (Kid = 1; Kid <= NKids(T); Kid++)
             StmtLabel = ProcessNode(Child(T, Kid), StmtLabel);
         if (StmtLabel != NoLabel)
            CodeGen0(NOP, StmtLabel);
         PopLoopExitLabel();
         CodeGen1(GOTOOP, LoopStart, NoLabel);
         CodeGen0(NOP, LoopEnd);
         return (NoLabel); 
      }

      case ExitNode :
      {
         Clabel ExitLabel = CurrentLoopExitLabel();
         if (ExitLabel == NoLabel)
             printf("<<< CODE GENERATOR ERROR >>> : EXIT STATEMENT NOT INSIDE A LOOP\n");
         else
             CodeGen1(GOTOOP, ExitLabel, CurrLabel);
         return (NoLabel);
      }

      case NullNode : return(CurrLabel);

      default :
         ReportTreeErrorAt(T);
         printf ("<<< CODE GENERATOR >>> : UNKNOWN NODE NAME ");
         Write_String (stdout,NodeName(T));
         printf ("\n");

   } /* end switch */
   return (NoLabel);
}   /* end ProcessNode */