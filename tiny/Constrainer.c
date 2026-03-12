/****************************************************************
              Copyright (C) 1986 by Manuel E. Bermudez
              Translated to C - 1991
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
#define PowNode        20
#define EqNode         21
#define NeNode         22
#define GeNode         23
#define LtNode         24
#define GtNode         25
#define AndNode        26
#define OrNode         27
#define NotNode        28
#define ModNode        29
#define MultNode       30
#define DivNode        31
#define TrueNode       32
#define FalseNode      33
#define EofNode        34
#define ForNode        35
#define DowntoNode     36
#define RepeatNode      37
#define CaseNode        38
#define LoopNode        39
#define ExitNode       40
#define SwapNode        41 


#define NumberOfNodes  41  

typedef TreeNode UserType;

/****************************************************************
 Add new node names to the end of the array, keeping in strict
  order with the define statements above, then adjust the i loop
  control variable in InitializeConstrainer().
*****************************************************************/
char *node[] = { "program", "types", "type", "dclns",
                 "dcln", "integer", "boolean", "block",
                 "assign", "output", "if", "while", 
                 "<null>", "<=", "+", "-", "read",
                 "<integer>", "<identifier>", "**", "=", "<>", ">=", "<", ">", "and", "or", "not", "mod", "*", "/", "true", "false", "eof","for", "downto",
                   "repeat", "case", "loop", "exit", "swap"
                };


UserType TypeInteger, TypeBoolean;
boolean TraceSpecified;
FILE *TraceFile;
char *TraceFileName;
int NumberTreesRead, index;


void Constrain(void)    
{
   int i;
   InitializeDeclarationTable();
   Tree_File = Open_File("_TREE", "r"); 
   NumberTreesRead = Read_Trees();
   fclose (Tree_File);                     
   AddIntrinsics();

   ProcessNode(RootOfTree(1)); 

#if 0
   PrintDclnTable(stdout);
   PrintAllStrings(stdout);
   PrintTree(stdout,RootOfTree(1));
#endif
    
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
   TreeNode TempTree;

   AddTree (TypesNode, RootOfTree(1), 2);

   TempTree = Child (RootOfTree(1), 2);
   AddTree (TypeNode, TempTree, 1);

   TempTree = Child (RootOfTree(1), 2);
   AddTree (TypeNode, TempTree, 1);

   TempTree = Child (Child (RootOfTree(1), 2), 1);
   AddTree (BooleanTNode, TempTree, 1);
 
   TempTree = Child (Child (RootOfTree(1), 2), 2);
   AddTree (IntegerTNode, TempTree, 1);

   TempTree = Child (RootOfTree(1), 2);
   TypeBoolean = Child(TempTree,1);
   TypeInteger = Child(TempTree,2);
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
   TreeNode Declaration, Temp1, Temp2;
   int NodeCount;
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
            printf ("ARGUMENTS OF '<=' MUST BE TYPE INTEGER\n");
            printf ("\n");
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
            printf ("ARGUMENTS OF '+', '-', '*', '/', mod ");
            printf ("MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         return (TypeInteger);
 
      case ReadNode :
         return (TypeInteger);

      case IntegerNode : 
         return (TypeInteger);

      case IdentifierNode :
         Declaration = Lookup (NodeName(Child(T,1)), T);
         if (Declaration != NullDeclaration)
         {
            Decorate (T, Declaration);
            return (Decoration(Declaration));
         }
         else
            return (TypeInteger);

        
      case EqNode :
      case NeNode :
      case GeNode :
      case LtNode :
      case GtNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != Type2)
         {
            ErrorHeader(T);
            printf ("ARGUMENTS OF RELATIONAL OPS MUST BE THE SAME TYPE\n");
         }
         return (TypeBoolean);

      /* 2. Arithmetic Operators (Arguments and Result are Integer) */
      case MultNode :
      case DivNode :
      case ModNode :
      case PowNode :
         Type1 = Expression (Child(T,1));
         if (Rank(T) == 2)
            Type2 = Expression (Child(T,2));
         else  
            Type2 = TypeInteger;
         if (Type1 != TypeInteger || Type2 != TypeInteger)
         {
            ErrorHeader(T);
            printf ("ARITHMETIC ARGUMENTS MUST BE TYPE INTEGER\n");
         }
         return (TypeInteger);

      /* 3. Logical Operators (Arguments and Result are Boolean) */
      case AndNode :
      case OrNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != TypeBoolean || Type2 != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("LOGICAL ARGUMENTS MUST BE TYPE BOOLEAN\n");
         }
         return (TypeBoolean);

      case NotNode :
         Type1 = Expression (Child(T,1));
         if (Type1 != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("'NOT' ARGUMENT MUST BE TYPE BOOLEAN\n");
         }
         return (TypeBoolean);

      /* 4. Constants and Intrinsics */
      case TrueNode :
      case FalseNode :
         return (TypeBoolean);

      case EofNode :
         return (TypeBoolean);

      default :
         ErrorHeader(T);
         printf ( "UNKNOWN NODE NAME ");
         Write_String (stdout,NodeName(T));
         printf ("\n");

   }  /* end switch */
}  /* end Expression */


void ProcessNode (TreeNode T) 
{
   int Kid, N;
   String Name1, Name2;
   TreeNode Type1, Type2, Type3;
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
           printf ("PROGRAM NAMES DO NOT MATCH\n");
           printf ("\n");
         }
         for (Kid = 2; Kid <= NKids(T)-1; Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case TypesNode :  
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case TypeNode :
         DTEnter (NodeName(Child(T,1)),T,T);
         break;

      case DclnsNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case DclnNode :

	 Name1 = NodeName (Child(T, NKids(T)));
         Type1 = Lookup (Name1,T);
         for (Kid  = 1; Kid < NKids(T); Kid++)
         {
            DTEnter (NodeName(Child(Child(T,Kid),1)), Child(T,Kid), T);
            Decorate (Child(T,Kid), Type1);
         }
         break;

      case BlockNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            ProcessNode (Child(T,Kid));
         break;

      case AssignNode :
         Type1 = Expression (Child(T,1));
         Type2 = Expression (Child(T,2));
         if (Type1 != Type2)
         {
            ErrorHeader(T);
            printf ("ASSIGNMENT TYPES DO NOT MATCH\n");
            printf ("\n");
         }
         break;

      case OutputNode :
         for (Kid = 1; Kid <= NKids(T); Kid++)
            if (Expression (Child(T,Kid)) != TypeInteger)
            {
               ErrorHeader(T);
               printf ("OUTPUT EXPRESSION MUST BE TYPE INTEGER\n");
               printf ("\n");
            }
         break;

      case IfNode :
         if (Expression (Child(T,1)) != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("CONTROL EXPRESSION OF 'IF' STMT");
            printf (" IS NOT TYPE BOOLEAN\n");
            printf ("\n");
         } 
         ProcessNode (Child(T,2));
         if (Rank(T) == 3)
            ProcessNode (Child(T,3));
         break;

      case WhileNode :
         if (Expression (Child(T,1)) != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("WHILE EXPRESSION NOT OF TYPE BOOLEAN\n");
            printf ("\n");
         }
         ProcessNode (Child(T,2));
         break;

      case ForNode:
         // Check that loop variable is declared and is integer type
         if (Expression(Child(T,1)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("FOR LOOP VARIABLE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         if (Expression(Child(T,2)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("FOR INITIAL VALUE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         if (Expression(Child(T,3)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("FOR FINAL VALUE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         ProcessNode(Child(T,4));
         break;
      
      case DowntoNode:
         // Check that loop variable is declared and is integer type
         if (Expression(Child(T,1)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("DOWNTO LOOP VARIABLE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         if (Expression(Child(T,2)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("DOWNTO INITIAL VALUE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         if (Expression(Child(T,3)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("DOWNTO FINAL VALUE MUST BE TYPE INTEGER\n");
            printf ("\n");
         }
         ProcessNode(Child(T,4));
         break;
      
      case RepeatNode:
         if (Expression(Child(T,2)) != TypeBoolean)
         {
            ErrorHeader(T);
            printf ("UNTIL EXPRESSION NOT OF TYPE BOOLEAN\n");
            printf ("\n");
         }
         ProcessNode(Child(T,1));
         break;
      
      case CaseNode:
         if (Expression(Child(T,1)) != TypeInteger)
         {
            ErrorHeader(T);
            printf ("CASE EXPRESSION NOT OF TYPE INTEGER\n");
            printf ("\n");
         }
         for (Kid = 2; Kid <= NKids(T); Kid++)
            ProcessNode(Child(T,Kid));
         break;
      
      case LoopNode:
         ProcessNode(Child(T,1));
         break;
      
      case ExitNode:
         break;

      case SwapNode:
         Type1 = Expression(Child(T,1));
         Type2 = Expression(Child(T,2));
         if (Type1 != Type2)
         {
            ErrorHeader(T);
            printf ("SWAP VARIABLES MUST BE OF THE SAME TYPE\n");
            printf ("\n");
         }
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
