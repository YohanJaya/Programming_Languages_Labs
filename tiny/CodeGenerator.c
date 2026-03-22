/*******************************************************************
          Copyright (C) 1986 by Manuel E. Bermudez
          Translated to C - 1991
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
#define LeftMode 0
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

         /* UNARY OPERANDS */
#define    UNOT        22   /* 'UNOT'     */
#define    UNEG        23   /* 'UNEG'     */
#define    USUCC       24   /* 'USUCC'    */
#define    UPRED       25   /* 'UPRED'    */
         /* BINARY OPERANDS */
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
         /* OS SERVICE CALL OPERANDS */
#define    TRACEX      40   /* 'TRACEX'   */
#define    DUMPMEM     41   /* 'DUMPMEM'  */
#define    OSINPUT     42   /* 'INPUT'    */
#define    OSINPUTC    43   /* 'INPUT'    */
#define    OSOUTPUT    44   /* 'OUTPUT'   */
#define    OSOUTPUTC   45   /* 'OUTPUT'   */
#define    OSOUTPUTL   46   /* 'OUTPUTL'  */
#define    OSEOF       47   /* 'EOF'      */

         /* TREE NODE NAMES */
#define    ProgramNode  48   /* 'program'  */
#define    TypesNode    49   /* 'types'    */
#define    TypeNode     50   /* 'type'     */
#define    DclnsNode    51   /* 'dclns'    */
#define    DclnNode     52   /* 'dcln'     */
#define    IntegerTNode 53   /* 'integer'  */
#define    BooleanTNode 54   /* 'boolean'  */
#define    BlockNode    55   /* 'block'    */
#define    AssignNode   56   /* 'assign'   */
#define    OutputNode   57   /* 'output'   */ 
#define    IfNode       58   /* 'if'       */
#define    WhileNode    59   /* 'while'    */
#define    NullNode     60   /* '<null>'   */
#define    LENode       61   /* '<='       */
#define    PlusNode     62   /* '+'        */
#define    MinusNode    63   /* '-'        */
#define    ReadNode     64   /* 'read'     */
#define    IntegerNode  65   /* '<integer>'*/
#define    IdentifierNode 66 /* '<identifier>'*/
#define    PowNode      67   /* '**'       */   
#define    EqNode    68   /* '='        */
#define    NeNode 69   /* '<>'       */
#define    GeNode 70 /* '>='      */
#define    LtNode     71   /* '<'        */
#define    GtNode  72   /* '>'        */
#define    AndNode     73   /* 'and'      */
#define    OrNode      74   /* 'or'       */    
#define    NotNode     75   /* 'not'      */
#define    ModNode     76   /* 'mod'      */
#define    MultNode    77   /* '*'        */
#define    DivNode     78   /* '/'        */
#define    TrueNode    79   /* 'true'     */
#define    FalseNode   80   /* 'false'    */
#define    EofNode      81   /* 'eof'      */
#define ForToNode 82 
#define ForDowntoNode 83  
#define RepeatNode 84 
#define CaseNode 85 
#define CaseClauseNode 86 
#define CaseClauseRangeNode 87 
#define SwapNode 88
#define LoopNode 89 
#define ExitNode 90 
#define OtherwiseNode 91 

#define NumberOfNodes 91
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
   if (LoopExitTop >= 0)
      LoopExitTop--;
}

Clabel CurrentLoopExitLabel(void) {
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
     "INPUTC","OUTPUT","OUTPUTC","OUTPUTL","EOF","FOR","DOWN"};

/****************************************************************** 
   add new node names to the end of the array, keeping in strict order
   as defined above, then adjust the j loop control variable in
   InitializeNodeNames(). 
*******************************************************************/
char *node_name[] =
    {"program","types","type","dclns","dcln","integer",
     "boolean","block","assign","output","if","while",
     "<null>","<=","+","-","read","<integer>","<identifier>",
   "**","=","<>",">=","<",">","and","or","not","mod","*","/","true","false","eof","for","downto","repeat","case","loop","exit","swap"};


void CodeGenerate(int argc, char *argv[])
{
   int NumberTrees;

   InitializeCodeGenerator(argc,argv);
   Tree_File = Open_File("_TREE", "r"); 
   NumberTrees = Read_Trees();
   fclose (Tree_File);                     

   HaltLabel = ProcessNode (RootOfTree(1), NoLabel);
   CodeGen0 (HALTOP, HaltLabel); 

#if 0
   PrintAllStrings(stdout);
   PrintTree(stdout,RootOfTree(1));
#endif

   CodeFile = Open_File (CodeFileName, "w");
   DumpCode (CodeFile);
   fclose(CodeFile); 

   if (TraceSpecified)
      fclose (TraceFile);

/****************************************************************** 
  enable this code to write out the tree after the code generator
  has run.  It will show the new decorations made with MakeAddress().
*******************************************************************/


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

      case PlusNode :
         Expression ( Child(T,1) , CurrLabel);
         if (Rank(T) == 2)
         {
            Expression ( Child(T,2) , NoLabel);
            CodeGen1 (BOPOP, BPLUS, NoLabel);
            DecrementFrameSize();
         }
         /* Unary plus: just evaluate the expression, no operation needed */
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

      case ReadNode :
         CodeGen1 (SOSOP, OSINPUT, CurrLabel);
         IncrementFrameSize();
         break;

      case IntegerNode :
         CodeGen1 (LITOP, NodeName (Child(T,1)), CurrLabel);
         IncrementFrameSize();
         break;

      case IdentifierNode :
         Reference (T,RightMode,CurrLabel);
         break;
         
      case PowNode :
      case EqNode :
      case NeNode :
      case GeNode :
      case LtNode :
      case GtNode :
      case AndNode :
      case OrNode :
      case ModNode :
      case MultNode :
      case DivNode :
         Expression (Child(T,1), CurrLabel);
         Expression (Child(T,2), NoLabel);
         if (NodeName(T) == PowNode)      CodeGen1 (BOPOP, BEXP, NoLabel);
         else if (NodeName(T) == EqNode)  CodeGen1 (BOPOP, BEQ, NoLabel);
         else if (NodeName(T) == NeNode)  CodeGen1 (BOPOP, BNE, NoLabel);
         else if (NodeName(T) == GeNode)  CodeGen1 (BOPOP, BGE, NoLabel);
         else if (NodeName(T) == LtNode)  CodeGen1 (BOPOP, BLT, NoLabel);
         else if (NodeName(T) == GtNode)  CodeGen1 (BOPOP, BGT, NoLabel);
         else if (NodeName(T) == AndNode) CodeGen1 (BOPOP, BAND, NoLabel);
         else if (NodeName(T) == OrNode)  CodeGen1 (BOPOP, BOR, NoLabel);
         else if (NodeName(T) == ModNode) CodeGen1 (BOPOP, BMOD, NoLabel);
         else if (NodeName(T) == MultNode) CodeGen1 (BOPOP, BMULT, NoLabel);
         else if (NodeName(T) == DivNode)  CodeGen1 (BOPOP, BDIV, NoLabel);
         DecrementFrameSize();
         break;

      
      case NotNode:
         Expression (Child(T,1), CurrLabel);
         CodeGen1 (UOPOP, UNOT, NoLabel);
         break;

      
      case TrueNode:
         CodeGen1 (LITOP, MakeStringOf(1), CurrLabel); 
         IncrementFrameSize();
         break;

      case FalseNode:
         CodeGen1 (LITOP, MakeStringOf(0), CurrLabel);
         IncrementFrameSize();
         break;

      
      case EofNode:
         CodeGen1 (SOSOP, OSEOF, CurrLabel); 
         IncrementFrameSize();
         break;


      default :
         ReportTreeErrorAt(T);
         printf ("<<< CODE GENERATOR >>> : UNKNOWN NODE NAME ");
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
         for (Kid = 1; Kid < NKids(T); Kid++)
         {
            if (Kid != 1)
                CodeGen1 (LITOP,MakeStringOf(0),NoLabel);
            else
                CodeGen1 (LITOP,MakeStringOf(0),CurrLabel);
            Num = MakeAddress();
            Decorate ( Child(T,Kid), Num);
            IncrementFrameSize();
         }
         return (NoLabel);                

      case BlockNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            CurrLabel = ProcessNode (Child(T,Kid), CurrLabel);
         return (CurrLabel); 


      case AssignNode :
         Expression (Child(T,2), CurrLabel);
         Reference (Child(T,1), LeftMode, NoLabel);
         return (NoLabel);

      case SwapNode :
         Reference(Child(T,1), RightMode, CurrLabel); 
         Reference(Child(T,2), RightMode, NoLabel);                      
         Reference(Child(T,1), LeftMode, NoLabel);    
         Reference(Child(T,2), LeftMode, NoLabel);    
         return (NoLabel);


      case OutputNode :
         Expression (Child(T,1), CurrLabel);
         CodeGen1 (SOSOP, OSOUTPUT, NoLabel);
         DecrementFrameSize();
         for (Kid = 2; Kid <= NKids(T); Kid++)
         {
            Expression (Child(T,Kid), NoLabel);
            CodeGen1 (SOSOP, OSOUTPUT, NoLabel);
            DecrementFrameSize();
         }
         CodeGen1 (SOSOP, OSOUTPUTL, NoLabel);
         return (NoLabel);


      case IfNode :
         Expression (Child(T,1), CurrLabel);
         Label1 = MakeLabel();  // THEN part
         Label2 = MakeLabel();  // ELSE part  
         Label3 = MakeLabel();  // END
         CodeGen2 (CONDOP,Label1,Label2, NoLabel);
         DecrementFrameSize();
         
         // Process THEN part
         ProcessNode (Child(T,2), Label1);
         
         if (Rank(T) == 3)
         {
            // Has ELSE part - jump over it after THEN
            CodeGen1 (GOTOOP, Label3, NoLabel);
            // Process ELSE part
            ProcessNode (Child(T,3), Label2);
            // Place END label
            CodeGen0 (NOP, Label3);
         }
         else
         {
            // No ELSE part - ELSE label is the END
            CodeGen0 (NOP, Label2);
         }
         return (NoLabel);                


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
         // for i := start to end do body
         // Child(1) = loop variable, Child(2) = start, Child(3) = end, Child(4) = body
         
         // Evaluate and store start value in loop variable
         Expression(Child(T,2), CurrLabel);
         Reference(Child(T,1), LeftMode, NoLabel);
         
         // Create labels
         Label1 = MakeLabel();  // condition check
         Label2 = MakeLabel();  // loop body
         Label3 = MakeLabel();  // exit
         
         // Label1: Check condition (i <= end)
         Reference(Child(T,1), RightMode, Label1);  // Load i
         Expression(Child(T,3), NoLabel);            // Load end value
         CodeGen1(BOPOP, BLE, NoLabel);              // i <= end ?
         CodeGen2(CONDOP, Label2, Label3, NoLabel);  // if true goto body, else exit
         DecrementFrameSize();
         DecrementFrameSize();
         DecrementFrameSize();
         
         // Label2: Execute loop body
         ProcessNode(Child(T,4), Label2);
         
         // Increment loop variable: i := i + 1
         Reference(Child(T,1), RightMode, NoLabel);  // Load i
         CodeGen1(LITOP, MakeStringOf(1), NoLabel);  // Load 1
         CodeGen1(BOPOP, BPLUS, NoLabel);            // i + 1
         Reference(Child(T,1), LeftMode, NoLabel);   // Store to i
         DecrementFrameSize();
         
         // Jump back to condition check
         CodeGen1(GOTOOP, Label1, NoLabel);
         
         return (Label3);
         
      case ForDowntoNode :
         // for i := start downto end do body
         // Child(1) = loop variable, Child(2) = start, Child(3) = end, Child(4) = body
         
         // Evaluate and store start value in loop variable
         Expression(Child(T,2), CurrLabel);
         Reference(Child(T,1), LeftMode, NoLabel);
         
         // Create labels
         Label1 = MakeLabel();  // condition check
         Label2 = MakeLabel();  // loop body
         Label3 = MakeLabel();  // exit
         
         // Label1: Check condition (i >= end)
         Reference(Child(T,1), RightMode, Label1);  // Load i
         Expression(Child(T,3), NoLabel);            // Load end value
         CodeGen1(BOPOP, BGE, NoLabel);              // i >= end ?
         CodeGen2(CONDOP, Label2, Label3, NoLabel);  // if true goto body, else exit
         DecrementFrameSize();
         DecrementFrameSize();
         DecrementFrameSize();
         
         // Label2: Execute loop body
         ProcessNode(Child(T,4), Label2);
         
         // Decrement loop variable: i := i - 1
         Reference(Child(T,1), RightMode, NoLabel);  // Load i
         CodeGen1(LITOP, MakeStringOf(1), NoLabel);  // Load 1
         CodeGen1(BOPOP, BMINUS, NoLabel);           // i - 1
         Reference(Child(T,1), LeftMode, NoLabel);   // Store to i
         DecrementFrameSize();
         
         // Jump back to condition check
         CodeGen1(GOTOOP, Label1, NoLabel);
         
         return (Label3);

      case RepeatNode :
      {
          Clabel LoopStart = MakeLabel();
          Clabel StmtLabel = NoLabel;
          Clabel LoopEnd = MakeLabel();
          
          CodeGen0(NOP, LoopStart); 

          for (Kid = 1; Kid < NKids(T); Kid++) {
              StmtLabel = ProcessNode(Child(T, Kid), StmtLabel);
          }

          Expression(Child(T, NKids(T)), StmtLabel); 

          CodeGen2(CONDOP, LoopEnd, LoopStart, NoLabel);
          DecrementFrameSize();
          
          return (LoopEnd);
      }


      case LoopNode :
         // loop <statements> pool (infinite loop with exit statement)
         // Child(1) = statement block
         // Need to track exit label for exit statements
         
         if (CurrLabel == NoLabel)
            Label1 = MakeLabel();
         else
            Label1 = CurrLabel;
         Label2 = MakeLabel();  // loop start
         Label3 = MakeLabel();  // exit label
         
         // Set up exit label for this loop (would need global tracking in real implementation)
         // For now, we'll process statements and jump back
         ProcessNode(Child(T,1), Label2);
         
         // Jump back to loop start
         CodeGen1(GOTOOP, Label2, NoLabel);
         
         return (Label3);

      case ExitNode :
         // exit statement - breaks out of enclosing loop
         // In a real implementation, would need to track the current loop's exit label
         // For now, generate a GOTO that would need to be patched
         Label1 = MakeLabel();  // This should be the loop's exit label
         CodeGen1(GOTOOP, Label1, CurrLabel);
         return (Label1);

      case CaseNode :
         // case <expression> of <branches> end
         // Child(1) = expression, Child(2..N) = case branches
         // This is a simplified version without full case branch handling
         
         Expression(Child(T,1), CurrLabel);  // Evaluate case expression
         
         // Generate comparison and jump code for each branch
         // This would need proper case branch node handling
         Label1 = MakeLabel();  // exit label
         
         for (Kid = 2; Kid <= NKids(T); Kid++)
         {
            ProcessNode(Child(T,Kid), NoLabel);
         }
         
         CodeGen0(POPOP, NoLabel);  // Pop the case expression value
         DecrementFrameSize();
         
         return (Label1);

       case NullNode : return(CurrLabel);

 
      default :
              ReportTreeErrorAt(T);
              printf ("<<< CODE GENERATOR >>> : UNKNOWN NODE NAME ");
              Write_String (stdout,NodeName(T));
              printf ("\n");

   } /* end switch */
}   /* end ProcessNode */
