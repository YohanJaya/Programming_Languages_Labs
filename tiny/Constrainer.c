/****************************************************************
              Copyright (C) 1986 by Manuel E. Bermudez
              Translated to C - 1991
              Lab 5 Extensions: char type, const, user-defined types,
              enumerated types, succ/pred/ord/chr, read statement,
              string output.
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
#define CaseClauseNode 39
#define CaseClauseRangeNode 40
#define SwapNode       41
#define LoopNode       42
#define ExitNode       43
#define OtherwiseNode  44
#define ConstsNode     45  /* "consts"  */
#define ConstNode      46  /* "const"   */
#define CharTNode      47  /* "char"   */
#define LitNode        48  /* "lit"   */
#define CharNode       49  /* "<char>" */
#define StringNode     50  /* "<string>"  */
#define SuccNode       51  /* "succ"   */
#define PredNode       52  /* "pred"   */
#define OrdNode        53  /* "ord"    */
#define ChrNode        54  /* "chr"    */
#define ReadItemNode   55  /* "read_item" */

#define NumberOfNodes  55

typedef TreeNode UserType;

char *node[] = {
    "program", "types", "type", "dclns",
    "dcln", "integer", "boolean", "block",
    "assign", "output", "if", "while",
    "<null>", "<=", "+", "-", "read",
    "<integer>", "<identifier>",
    "<true>", "<false>", "=", "<>", ">=",
    "<", ">", "or", "*", "/", "and",
    "mod", "**", "not", "eof", "for_to", "for_downto",
    "repeat", "case", "case_clause", "case_clause_range",
    "swap", "loop", "exit", "otherwise",
    "consts", "const", "char", "lit", "<char>", "<string>",
    "succ", "pred", "ord", "chr", "read_item"
};

UserType TypeInteger, TypeBoolean, TypeChar;
boolean TraceSpecified;
FILE *TraceFile;
char *TraceFileName;
int NumberTreesRead, index;

int IsEnumeratedType(UserType Type)
{
   return (Type > 0 &&
           NodeName(Type) == TypeNode &&
           NKids(Type) >= 2 &&
           NodeName(Child(Type,2)) == LitNode);
}

int IsScalarType(UserType Type)
{
   return (Type == TypeInteger ||
           Type == TypeBoolean ||
           Type == TypeChar ||
           IsEnumeratedType(Type));
}


void Constrain(void)    
{
   int i;
   InitializeDeclarationTable();
   Tree_File = Open_File("_TREE", "r"); 
   NumberTreesRead = Read_Trees();
   fclose (Tree_File);                     
   AddIntrinsics();

   ProcessNode(RootOfTree(1)); 

   Tree_File = fopen("_TREE", "w");  
   Write_Trees();
   fclose (Tree_File);                   
   if (TraceSpecified) fclose(TraceFile);    
}


void InitializeConstrainer (int argc, char *argv[])
{          
   int i, j;
   InitializeTextModule();
   InitializeTreeModule();
   for (i=0, j=1; i<NumberOfNodes; i++, j++)
      String_Array_To_String_Constant (node[i], j); 
   index = System_Flag ("-trace", argc, argv);
   if (index)                                       
   {
      TraceSpecified = true; 
      TraceFileName = System_Argument ("-trace", "_TRACE", argc, argv);
      TraceFile = Open_File(TraceFileName, "w");
   }
   else
      TraceSpecified = false;          
}                        

void AddIntrinsics (void)
{
   TypeInteger = IntegerTNode;
   TypeBoolean = BooleanTNode;
   TypeChar = CharTNode;
}

void ErrorHeader (TreeNode T)
{ 
   printf ("<<< CONSTRAINER ERROR >>> AT ");
   Write_String (stdout,SourceLocation(T));
   printf (" : ");
   printf ("\n");
}

int NKids (TreeNode T)
{
   return (Rank(T));
}

UserType Expression (TreeNode T)
{
   UserType Type1, Type2;
   TreeNode Declaration;

   if (TraceSpecified)
   {
      fprintf (TraceFile, "<<< CONSTRAINER >>> : Expn Processor Node ");
      Write_String (TraceFile, NodeName(T));
      fprintf (TraceFile, "\n");
   }
     
   switch (NodeName(T))
   {
      case LENode :    
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != Type2)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENTS OF '<=' MUST HAVE THE SAME TYPE\n\n");
         }
         return (TypeBoolean);

      case PlusNode :
      case MinusNode : 
         Type1 = Expression (Child(T,1));
         if (Rank(T) == 2)
            Type2 = Expression (Child(T,2));
         else  
            Type2 = TypeInteger;
         if (Type1 != TypeInteger || Type2 != TypeInteger)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENTS OF '+', '-' MUST BE TYPE INTEGER\n\n");
         }
         return (TypeInteger);

      case EofNode :
         return (TypeBoolean);

      case IntegerNode : 
         return (TypeInteger);

      case CharNode :
         return (TypeChar);

      case EQNode :
      case NENode :
      case GENode :
      case LTNode :
      case GTNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != Type2)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENTS OF COMPARISON OPERATORS MUST HAVE THE SAME TYPE\n\n");
         }
         return (TypeBoolean);

      case MultNode :
      case DivNode :
      case ModNode :
      case ExpNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != TypeInteger || Type2 != TypeInteger)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENTS OF '*', '/', 'mod', '**' MUST BE TYPE INTEGER\n\n");
         }
         return (TypeInteger);

      case AndNode :
      case OrNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != TypeBoolean || Type2 != TypeBoolean)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENTS OF 'and', 'or' MUST BE TYPE BOOLEAN\n\n");
         }
         return (TypeBoolean);

      case NotNode :
         Type1 = Expression (Child(T,1));
         if (Type1 != TypeBoolean)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENT OF 'not' MUST BE TYPE BOOLEAN\n\n");
         }
         return (TypeBoolean);

      case TrueNode :
      case FalseNode :
         return (TypeBoolean);

      case SuccNode :
      case PredNode :
         Type1 = Expression (Child(T,1));
         if (!IsScalarType(Type1))
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENT OF 'succ'/'pred' MUST BE INTEGER, CHAR, OR ENUMERATED TYPE\n\n");
         }
         return (Type1);

      case OrdNode :
         Type1 = Expression (Child(T,1));
         if (!IsScalarType(Type1))
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENT OF 'ord' MUST BE INTEGER, BOOLEAN, CHAR, OR ENUMERATED TYPE\n\n");
         }
         return (TypeInteger);

      case ChrNode :
         Type1 = Expression (Child(T,1));
         if (Type1 != TypeInteger)
         {
            ErrorHeader(Child(T,1));
            printf ("ARGUMENT OF 'chr' MUST BE TYPE INTEGER\n\n");
         }
         return (TypeChar);

      case IdentifierNode :
         Declaration = Lookup (NodeName(Child(T,1)), T);
         if (Declaration != NullDeclaration)
         {
            Decorate (T, Declaration);
            return (Decoration(Declaration));
         }
         else
         {
            ErrorHeader(T);
            printf ("UNDECLARED IDENTIFIER: ");
            Write_String(stdout, NodeName(Child(T,1)));
            printf("\n\n");
            return (TypeInteger);
         }

      default :
         ErrorHeader(T);
         printf ( "UNKNOWN EXPRESSION NODE NAME ");
         Write_String (stdout,NodeName(T));
         printf ("\n");
         return (TypeInteger);

   }  /* end switch */
}  /* end Expression */


void ProcessNode (TreeNode T) 
{
   int Kid, N;
   String Name1, Name2;
   TreeNode Type1, Type2, Type3, Declaration;
   UserType ExprType;
   int LitIndex;

   if (TraceSpecified)
   {
      fprintf (TraceFile,
               "<<< CONSTRAINER >>> : Stmt Processor Node ");
      Write_String (TraceFile, NodeName(T));
      fprintf (TraceFile, "\n");
   }

   switch (NodeName(T))
   {
      case ProgramNode : 
         Name1 = NodeName(Child(Child(T,1),1));
         Name2 = NodeName(Child(Child(T,NKids(T)),1));
         if (Name1 != Name2)
         {
           ErrorHeader(T);
           printf ("PROGRAM NAMES DO NOT MATCH\n\n");
         }
         for (Kid = 2; Kid <= NKids(T)-1; Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case ConstsNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case ConstNode :
      {
         String ConstName = NodeName(Child(Child(T,1),1));
         TreeNode ValueNode = Child(T,2);
         UserType ConstType;
         if (NodeName(ValueNode) == IntegerNode)
            ConstType = TypeInteger;
         else if (NodeName(ValueNode) == CharNode)
            ConstType = TypeChar;
	         else if (NodeName(ValueNode) == TrueNode || NodeName(ValueNode) == FalseNode)
	            ConstType = TypeBoolean;
	         else
	         {
	            Declaration = Lookup(NodeName(Child(ValueNode,1)), T);
	            if (Declaration != NullDeclaration)
	            {
	               Decorate(ValueNode, Declaration);
	               ConstType = Decoration(Declaration);
	            }
	            else
	               ConstType = TypeInteger;
	         }
         DTEnter(ConstName, T, T);
         Decorate(T, ConstType);
         break;
      }

      case TypesNode :  
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case TypeNode :
      {
         String TypeName2;
         TreeNode TypeDef;
         if (NKids(T) < 2)
            break;

         TypeName2 = NodeName(Child(Child(T,1),1));
         TypeDef = Child(T,2);
         DTEnter(TypeName2, T, T);

         if (NodeName(TypeDef) == LitNode)
         {

            Decorate(T, T);
            for (LitIndex = 1; LitIndex <= NKids(TypeDef); LitIndex++)
            {
               String MemberName = NodeName(Child(Child(TypeDef,LitIndex),1));
               DTEnter(MemberName, Child(TypeDef,LitIndex), Child(TypeDef,LitIndex));
               Decorate(Child(TypeDef,LitIndex), T);
            }
         }
         else if (NodeName(TypeDef) == IntegerTNode)
            Decorate(T, TypeInteger);
         else if (NodeName(TypeDef) == BooleanTNode)
            Decorate(T, TypeBoolean);
         else if (NodeName(TypeDef) == CharTNode)
            Decorate(T, TypeChar);
         else
         {
            Declaration = Lookup(NodeName(Child(TypeDef,1)), T);
            if (Declaration != NullDeclaration)
               Decorate(T, Decoration(Declaration));
            else
               Decorate(T, TypeInteger);
         }
         break;
      }

      case DclnsNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case DclnNode :
      {

         TreeNode TypeNameNode = Child(T, NKids(T));
         if (NodeName(TypeNameNode) == IntegerTNode)
            Type1 = TypeInteger;
         else if (NodeName(TypeNameNode) == BooleanTNode)
            Type1 = TypeBoolean;
         else if (NodeName(TypeNameNode) == CharTNode)
            Type1 = TypeChar;
         else
         {
            String TypeNameStr = NodeName(Child(TypeNameNode,1));
            Declaration = Lookup(TypeNameStr, T);
            if (Declaration == NullDeclaration)
            {
               ErrorHeader(T);
               printf("UNKNOWN TYPE: ");
               Write_String(stdout, TypeNameStr);
               printf("\n\n");
               Type1 = TypeInteger;
            }
            else
               Type1 = Decoration(Declaration);
         }

         for (Kid = 1; Kid < NKids(T); Kid++)
         {
            DTEnter(NodeName(Child(Child(T,Kid),1)), Child(T,Kid), T);
            Decorate(Child(T,Kid), Type1);
         }
         break;
      }

      case BlockNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case AssignNode :
      {
         UserType LType = Expression (Child(T,1));
         UserType RType = Expression (Child(T,2));
         if (LType != RType)
         {
            ErrorHeader(T);
            printf ("ASSIGNMENT TYPES DO NOT MATCH\n\n");
         }
         break;
      }

      /* ---- read statement: read(a, b, c) ---- */
      case ReadNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
         {
            /* Each child is a read_item node containing an identifier */
            TreeNode ReadItem = Child(T, Kid);
            UserType VarType = Expression(Child(ReadItem, 1));
            if (VarType != TypeInteger && VarType != TypeChar)
            {
               ErrorHeader(ReadItem);
               printf("READ ARGUMENT MUST BE TYPE INTEGER OR CHAR\n\n");
            }
         }
         break;

      case OutputNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
         {
            TreeNode OutChild = Child(T, Kid);
            if (NodeName(OutChild) == StringNode)
               ; /* string literal: always OK */
            else
            {
               UserType OutType = Expression(OutChild);
               if (OutType != TypeInteger && OutType != TypeChar)
               {
                  ErrorHeader(T);
                  printf ("OUTPUT EXPRESSION MUST BE TYPE INTEGER OR CHAR\n\n");
               }
            }
         }
         break;

      case IfNode :
         if (Expression (Child(T,1)) != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("CONTROL EXPRESSION OF 'IF' STMT IS NOT TYPE BOOLEAN\n\n");
         } 
         ProcessNode (Child(T,2));
         if (Rank(T) == 3)
            ProcessNode (Child(T,3));
         break;

      case WhileNode :
         if (Expression (Child(T,1)) != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("WHILE EXPRESSION NOT OF TYPE BOOLEAN\n\n");
         }
         ProcessNode (Child(T,2));
         break;
      
      case ForToNode :
      case ForDowntoNode :
      {
         UserType ControlType = Expression(Child(T, 1));
         UserType StartType = Expression(Child(T, 2));
         UserType EndType = Expression(Child(T, 3));
         
         if (ControlType != TypeInteger && ControlType != TypeChar && !IsEnumeratedType(ControlType)) {
             ErrorHeader(T);
             printf("CONTROL VARIABLE OF 'FOR' STMT MUST BE TYPE INTEGER, CHAR, OR ENUMERATED TYPE\n\n");
         }
         if (StartType != ControlType) {
             ErrorHeader(T);
             printf("STARTING EXPRESSION OF 'FOR' STMT MUST MATCH CONTROL VARIABLE TYPE\n\n");
         }
         if (EndType != ControlType) {
             ErrorHeader(T);
             printf("ENDING EXPRESSION OF 'FOR' STMT MUST MATCH CONTROL VARIABLE TYPE\n\n");
         }
         ProcessNode(Child(T, 4));
         break;
      }
      
      case RepeatNode :
         for (Kid = 1; Kid < NKids(T); Kid++) {
             ProcessNode(Child(T, Kid));
         }
         if (Expression(Child(T, NKids(T))) != TypeBoolean)
         {
            ErrorHeader(T);
            printf("UNTIL EXPRESSION OF 'REPEAT' STMT MUST BE TYPE BOOLEAN\n\n");
         }
         break;

      case CaseNode :
      {
         UserType CaseType = Expression(Child(T, 1));
         if (!IsScalarType(CaseType))
         {
            ErrorHeader(T);
            printf("CASE EXPRESSION MUST BE TYPE INTEGER, CHAR, BOOLEAN, OR ENUMERATED TYPE\n\n");
         }
         for (Kid = 2; Kid < NKids(T); Kid++) {
             TreeNode Clause = Child(T, Kid);
             if (NodeName(Clause) == CaseClauseRangeNode)
             {
                Type1 = Expression(Child(Clause, 1));
                Type2 = Expression(Child(Clause, 2));
                if (Type1 != CaseType || Type2 != CaseType)
                {
                   ErrorHeader(Clause);
                   printf("CASE RANGE VALUES MUST MATCH CASE EXPRESSION TYPE\n\n");
                }
                ProcessNode(Child(Clause, 3));
             }
             else
             {
                Type1 = Expression(Child(Clause, 1));
                if (Type1 != CaseType)
                {
                   ErrorHeader(Clause);
                   printf("CASE VALUE MUST MATCH CASE EXPRESSION TYPE\n\n");
                }
                ProcessNode(Child(Clause, 2));
             }
         }
         if (NodeName(Child(T, NKids(T))) != NullNode) {
             ProcessNode(Child(Child(T, NKids(T)), 1));
         }
         break;
      }

      case SwapNode :
      {
         UserType ST1 = Expression(Child(T,1));
         UserType ST2 = Expression(Child(T,2));
         if (ST1 != ST2)
         {
            ErrorHeader(T);
            printf ("SWAP TYPES DO NOT MATCH\n\n");
         }
         break;
      }

      case LoopNode :
         for (Kid = 1; Kid <= NKids(T); Kid++) {
             ProcessNode(Child(T, Kid));
         }
         break;

      case ExitNode :
         break;

      case NullNode : 
         break;

      default :
         ErrorHeader(T);
         printf ("UNKNOWN NODE NAME ");
         Write_String (stdout,NodeName(T));
         printf ("\n");

   }  /* end switch */
}  /* end ProcessNode */