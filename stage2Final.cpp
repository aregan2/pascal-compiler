// Andrew Regan and Leo Olvera
// CS 4301
// stage2

#include <stage2.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <stack>

char lastChar = '\n';
bool myFalseDeclared = false;
bool myTrueDeclared = false;
time_t now = time(NULL);
SymbolTableEntry myFalse("FALSE",BOOLEAN,CONSTANT,"0",YES,1);
SymbolTableEntry myTrue("TRUE",BOOLEAN,CONSTANT,"-1",YES,1);

using namespace std;

/* ****************************************************STAGE0**************************************************** */

Compiler::Compiler(char **argv) // constructor
{
   sourceFile.open(argv[1]);
   listingFile.open(argv[2]);
   objectFile.open(argv[3]);
}

Compiler::~Compiler() // destructor
{
   sourceFile.close();
   listingFile.close();
   objectFile.close();
}

void Compiler::createListingHeader()
{
   listingFile << "STAGE2:" << setw(3) << "" << right << "ANDREW REGAN, LEO OLVERA" << setw(7) << "" << right << ctime(&now) << endl;
   listingFile << "LINE NO." << setw(14) << "" << right << "SOURCE STATEMENT" << endl << endl;
}

void Compiler::parser()
{
   nextChar();
   if(nextToken() != "program")
   {
      processError("keyword \"program\" expected");
   }
   prog();
}

void Compiler::createListingTrailer()
{
   listingFile << endl << "COMPILATION TERMINATED" << setw(6) << "" << right << errorCount;
   errorCount == 0 ? (listingFile <<  " ERRORS ENCOUNTERED" << endl) : (listingFile << " ERROR ENCOUNTERED" << endl);
}

void Compiler::processError(string err)
{
   errorCount++;
   listingFile << endl << "Error: Line " << lineNo << ": " << err << endl;
   createListingTrailer();
   exit(0);
}

void Compiler::prog() //token should be "program"
{
   if(token != "program")
      processError("keyword \"program\" expected");
   
   progStmt();
   nextToken();
   if(token == "const")
     consts();
   if(token == "var")
     vars();
   if(token != "begin")
      processError("keyword \"begin\" expected");
   beginEndStmt();
   //cout << token << endl;
   if(token != "$")
       processError("no text may follow \"end\"");
}

void Compiler::progStmt() //token should be "program"
{  
   string x;
   if(token != "program")
      processError("keyboard \"program\" expected");
   x = nextToken();
   if(isNonKeyId(x) == false)
      processError("program name expected");
   if(nextToken() != ";")
      processError("semicolon expected");
   code("program", x);
   insert(x,PROG_NAME,CONSTANT,x,NO,0);
}

void Compiler::consts() //token should be "const"
{
   if(token != "const")
      processError("keyword \"const\" expected");
   if(!isNonKeyId(nextToken()))
      processError("non-keyword identifier must follow \"const\"");
   constStmts();
}

void Compiler::vars() //token should be "var"
{
   if(token != "var")
      processError("keyword \"var\" expected");
   if(!isNonKeyId(nextToken()))
      processError("non-keyword identifier must follow \"var\"");
   varStmts();
}

void Compiler::beginEndStmt() //token should be "begin"
{
   if(token != "begin")
      processError("keyword \"begin\" expected");
   
   //cout << "NEW BEGINNING" << endl;
   execStmts();
   
   //cout << "isEnd: " << token << endl;
   if(token != "end")
      processError("\'read\', \'write\', \'if\', \'while\', \';\', \'begin\', nonKeyId or \'repeat\' expected");
   
   if(nextToken() == ";")
   {
      code("end", ";");
      nextToken();
   }
   
   else if(token == ".")
   {
      //cout << "hello"<<endl;
      code("end", ".");
      nextToken();
      if(token != "$")
        processError("no text may follow final \"end\"");
   }
   
   else
      processError("period or semicolon expected");
   //cout << "AfterBeginEnd: " << token << endl;
}

void Compiler::constStmts() //token should be NON_KEY_ID
{
   string x,y;
   if(!isNonKeyId(token))
      processError("non-keyword identifier expected");
   x = token;  // const name
   if(nextToken() != "=")
      processError("\"=\" expected");
   y = nextToken();
   if(y != "+" && y != "-" && y != "not" && !isNonKeyId(y) && y != "false" && y != "true" && !isInteger(y))
      processError("token to right of \"=\" illegal");
   if(y == "+" || y == "-")
   {
      if(!isInteger(nextToken()))
         processError("digit expected after sign");
      y += token;
   }
   if(y == "not")
   {
      if(isNonKeyId(nextToken()))
      {
         map<string, SymbolTableEntry>::iterator itr = symbolTable.find(token);
         if(itr == symbolTable.end())
         {
            processError("reference to undefined constant");
         }
         if(itr->second.getDataType() != BOOLEAN)
         {
            processError("referenced symbol is not of type BOOLEAN, therefore it cannont precede \"not\"");
         }
      }
      else if(!isBoolean(token))
         processError("boolean expected after \"not\"");
      if(token == "true")
         y = "false";
      else
         y = "true";
   }
   if(nextToken() != ";")
      processError("semicolon expected");
   storeTypes wT = whichType(y);
   string wV = whichValue(y);
   if(wT != storeTypes(BOOLEAN) && wT != storeTypes(INTEGER))
   {
      processError("data type of token must be INTEGER or BOOLEAN");
   }
   insert(x,wT,CONSTANT,wV,YES,1);
   x = nextToken();
   if(x != "begin" && x != "var" && !isNonKeyId(x))
      processError("non-keyword identifier, \"begin\", or \"var\" expected");
   if(isNonKeyId(x))
      constStmts();
}

void Compiler::varStmts() //token should be NON_KEY_ID
{
   string x,y;
   if (!isNonKeyId(token))
      processError("non-keyword identifier expected");
   x = ids();
   if (token != ":")
      processError("\":\" expected");
   if (nextToken() != "integer" && token != "boolean")
      processError("illegal type follows \":\"");
   y = token;
   if (nextToken() != ";")
      processError("semicolon expected");
   if(y == "integer")
      insert(x,storeTypes(INTEGER),VARIABLE,"",YES,1);
   if(y == "boolean")
      insert(x,storeTypes(BOOLEAN),VARIABLE,"",YES,1);
   if (nextToken() != "begin" && !isNonKeyId(token))
      processError("non-keyword identifier or \"begin\" expected");
   if (isNonKeyId(token))
      varStmts();
}

string Compiler::ids() //token should be NON_KEY_ID
{
   string temp,tempString;
   if (!isNonKeyId(token))
      processError("non-keyword identifier expected");
   tempString = token;
   temp = token;
   if (nextToken() == ",")
   {
      if (!isNonKeyId(nextToken()))
         processError("non-keyword identifier expected");
      tempString = temp + "," + ids();
   }
   //cout << tempString << endl;
   return tempString;
   
}

/* ****************************************************END STAGE 0**************************************************** */

/* ****************************************************TABLE FUNCTIONS**************************************************** */

void Compiler::insert(string externalName, storeTypes inType, modes inMode,
              string inValue, allocation inAlloc, int inUnits)
{
   string name;
   vector<string> namesV;
   namesV.push_back("");
   if(inValue == "true")
   {
      inValue = "-1";
   }
   else if(inValue == "false")
   {
      inValue = "0";
   }
   
   for(uint i = 0, wordCount = 0; i < externalName.length(); i++)
   {
      if(externalName.at(i) == ',')
      {
         wordCount++;
         namesV.push_back("");
         continue;
      }
      namesV.at(wordCount) += externalName.at(i);
   }
   
   while(namesV.size() > 0)
   {
      map<string, SymbolTableEntry>::iterator itr;
      map<string, SymbolTableEntry>::iterator it;
      it = symbolTable.find(namesV.front().substr(0,15));
      if(it != symbolTable.end())
      {
         processError("symbol " + it->first + " is multiply defined");
      }
      if(symbolTable.begin() != symbolTable.end())
      {
         if(isKeyword(namesV.front()))
            processError("illegal use of keyword");
         else
         {
            if(symbolTable.size() >= 256)
            {
               processError("symbol table overflow");
            }
            SymbolTableEntry myST(genInternalName(inType),inType,inMode,inValue,inAlloc,inUnits);
            symbolTable.insert(pair<string,SymbolTableEntry>(namesV.front().substr(0,15),myST));
         }
         namesV.erase(namesV.begin());
      }
      else
      {
         SymbolTableEntry myST("P0",inType,inMode,inValue,inAlloc,inUnits);
         if(isKeyword(namesV.front()))
            processError("illegal use of keyword");
         else
         {
            symbolTable.insert(std::pair<string,SymbolTableEntry>(namesV.front().substr(0,15),myST));
         }
         namesV.erase(namesV.begin());
      }
   }

}

storeTypes Compiler::whichType(string name) //tells which data type a name has
{
   int dataType;
   map<string, SymbolTableEntry>::iterator itr;
   itr = symbolTable.find(name);
   if(isLiteral(name))
   {
      if(isBoolean(name))
         dataType = storeTypes(BOOLEAN);
      else
         dataType = storeTypes(INTEGER);
   }
   else
   {
      if(itr != symbolTable.end())
      {
         dataType = itr->second.getDataType();
      }
      else
         processError("reference to undefined constant"); 
   }
   return storeTypes(dataType);
}
   
string Compiler::whichValue(string name) //tells which value a name has
{
   string value;
   map<string, SymbolTableEntry>::iterator itr;
   itr = symbolTable.find(name);
   if(isLiteral(name))                                      // needs to be updated for stage1
   {
      value = name;  
   }
   else
   {
      if(itr != symbolTable.end())
         value = itr->second.getValue();
      else
         processError("reference to undefined constant");
   }

   return value;
}

string Compiler::genInternalName(storeTypes stype) const
{
   static int boolCount = 0;
   static int intCount = 0;
   string s;
   if(stype == storeTypes(UNKNOWN))
   {
      s = "T";
      s += to_string(currentTempNo);
   }
   else if(stype == storeTypes(BOOLEAN))
   {
      s = "B";
      s += to_string(boolCount++);
   }
   else if(stype == storeTypes(INTEGER))
   {
      s = "I";
      s += to_string(intCount++); 
      
   }
   return s;
}


/* ****************************************************END TABLE FUNCTIONS**************************************************** */

/* ****************************************************LEXICAL ROUTINES**************************************************** */

string Compiler::nextToken() //returns the next token or end of file marker
{
   token = "";
   while (token == "")
   {
      if(ch == '{')
      {
         while(nextChar() != '}')
         {
            if(ch == END_OF_FILE)
               processError("unexpected end of file");
         }
            nextChar();
      }
      else if(ch == '}')
      {
         processError("\"}\" cannot begin token");
      }
      else if(isspace(ch))
      {
         nextChar();
      }
      else if(isSpecialSymbol(ch))
      {
         token = ch;
         char x = nextChar();
         if(token == ":" && x == '=')
         {
            token += x;
            nextChar();
         }
         else if(token == "<" && x == '>')
         {
            token += x;
            nextChar();
         }
         else if(token == "<" && x == '=')
         {
            token += x;
            nextChar();
         }
         else if(token == ">" && x == '=')
         {
            token += x;
            nextChar();
         }
      }
      else if(islower(ch))
      {
         char prevChar;
         token = ch;
         while((islower(nextChar()) > 0 || isdigit(ch) > 0 || ch == '_') && ch != END_OF_FILE)
         {
            if(ch == '_' && prevChar == '_')
               processError("underscore must be followed by digit or number");
            token += ch;
            prevChar = ch;
         }
         if(ch == END_OF_FILE)
            processError("unexpected end of file");
         if(token.at(token.length() - 1) == '_')
         {
            processError("illegal identifier name: identifier cannont end in \"_\"");
         }
      }
      else if(isdigit(ch))
      {
         while(isdigit(ch))
         {
            token += ch;
            nextChar();
         }
      }
      else if(ch == END_OF_FILE)
      {
         token = ch;
      }
      else
      {
         processError("illegal symbol");
      }
   }
   //cout << token << endl;
   return token;
}

char Compiler::nextChar() //returns the next character or end of file marker
{
  sourceFile.get(ch);

  if(sourceFile.eof())
  {
     ch = END_OF_FILE;
     return ch;
  }
  else
  {
     if(lastChar == '\n')
     {
        listingFile << setw(5) << ++lineNo << '|';
     }
     listingFile << ch;
  }
  lastChar = ch;
  return ch;

}

/* ****************************************************END LEXICAL ROUTINES**************************************************** */

/* ****************************************************IS FUNCTIONS**************************************************** */

bool Compiler::isKeyword(string s) const  // determines if s is a keyword
{
   if(s == "program" || s == "begin" || s == "end" 
         || s == "var" || s == "const" || s == "integer" 
         || s == "boolean" || s == "true" || s == "false" || s == "not"
         || s == "read" || s == "write" || s == "and"
         || s == "if" || s == "then" || s == "else" || s == "while"
         || s == "do" || s == "repeat" || s == "until")
      return true;
   else 
      return false;
}

bool Compiler::isSpecialSymbol(char c) const // determines if c is a special symbol
{
   if(c == '=' || c == ':' || c == ',' || c == ';' || c == '.' || c == '+' || c == '-' 
      || c == '(' || c == ')' || c == '*' || c == '<' || c == '>')
      return true;
   else
      return false;
}

bool Compiler::isNonKeyId(string s) const // determines if s is a non_key_id
{
   if(!isKeyword(s))
   {
      if(islower(s.at(0)))
      {
         for(uint i = 1; i < s.length() ; i++)     //stage0no001
         {

            if(islower(s.at(i) == 0) && isdigit(s.at(i)) == 0)
            {
               return false; 
            }
         }
         return true;
      }
      else
         return false;
   }
   return false;
}

bool Compiler::isInteger(string s) const  // determines if s is an integer
{
   char c;
   for (uint i = 0; i < s.length(); i++)
   {
      c = s.at(i);
     if (isdigit(c) == 0)
        return false;
   }
   return true;
}

bool Compiler::isBoolean(string s) const  // determines if s is a boolean
{
   if(s == "true" || s == "false")
      return true;
   else
      return false;
}

bool Compiler::isLiteral(string s) const  // determines if s is a literal
{
   if(isInteger(s) || isBoolean(s))
      return true;
   else if((s.at(0) == '-' || s.at(0) == '+') && isInteger(s.substr(1, s.length())))
      return true;
   else if(s.substr(0,2) == "not" && isBoolean(s.substr(3, s.length())))
      return true;
   else
      return false;
}

bool Compiler::isTemporary(string s) const
{
   if(s.length() > 0)
   {
      if(s.at(0) == 'T')
         return true;
      else
         return false;
   }
   return false;
}

bool isAddLevel(string s)
{
   if(s == "+" || s == "-" || s == "or")
      return true;
   else
      return false;
}

bool isMultLevel(string s)
{
   if(s == "*" || s == "div" || s == "mod" || s == "and")
      return true;
   else
      return false;   
}

bool isRelational(string s)
{
   if(s == "=" || s == "<>" || s == "<=" || s == ">=" || s == "<" || s == ">")
      return true;
   else
      return false;   
}

/* ****************************************************END IS FUNCTIONS**************************************************** */

/* ****************************************************STACK FUNCTIONS**************************************************** */

void Compiler::pushOperator(string name) //push name onto operatorStk
{
   operatorStk.push(name); 
}

void Compiler::pushOperand(string name) //push name onto operandStk, if name is a literal, also create a symbol table entry for it
{
   /*
   if name is a literal and has no symbol table entry
   insert symbol table entry, call whichType to determine the data type of the literal
   push name onto stack; 
   */
   map<string, SymbolTableEntry>::iterator itr;
   itr = symbolTable.find(name.substr(0,15));
   if(isLiteral(name) && itr == symbolTable.end())
   {
      if(name == "true")
      {
         if(!myTrueDeclared)
         {
            if(symbolTable.size() >= 256)
            {
               processError("symbol table overflow");
            }
            symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
            myTrueDeclared = true;
         }
      }
      else if(name == "false")
      {
         if(!myFalseDeclared)
         {
            if(symbolTable.size() >= 256)
            {
               processError("symbol table overflow");
            }
            symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
            myFalseDeclared = true;
         }         
      }
      else
         insert(name, whichType(name), CONSTANT, whichValue(name), YES, 1);
   }
   else if(isNonKeyId(name) && itr == symbolTable.end())
   {
      processError("reference to undefined symbol");
   }
   operandStk.push(name);
}

string Compiler::popOperator() //pop name from operatorStk
{
   /*
   if operatorStk is not empty
   return top element removed from stack;
   else
   processError(compiler error; operator stack underflow) */
   
   if(!operatorStk.empty())
   {
      
      string x = operatorStk.top();
      operatorStk.pop();
      return x;
   }
   processError("compiler error: operator stack underflow"); 
   return "";
}

string Compiler::popOperand() //pop name from operandStk
{
   /*
   if operandStk is not empty
   return top element removed from stack;
   else
   processError(compiler error; operand stack underflow) */
   
   
   if(!operandStk.empty())
   {
      string x = operandStk.top();
      operandStk.pop();
      return x;
   }
   processError("compiler error: operator stack underflow"); 
   return "";
}

/* ****************************************************END STACK FUNCTIONS**************************************************** */

/* ***************************************************CODE AND EMIT FUNCTIONS **************************************************** */

void Compiler::code(string op, string operand1, string operand2)
{
   /*
   if (op == "program")
   emitPrologue(operand1)
   else if (op == "end")
   emitEpilogue()
   else if (op == "read")
   emit read code
   else if (op == "write")
   emit write code
   else if (op == "+") // this must be binary '+'
   emit addition code
   else if (op == "-") // this must be binary '-'
   emit subtraction code
   else if (op == "neg") // this must be unary '-'
   emit negation code;
   else if (op == "not")
   emit not code
   else if (op == "*")
   emit multiplication code
   else if (op == "div")
   emit division code
   else if (op == "mod")
   emit modulo code
   else if (op == "and")
   emit and code
   …
   else if (op == "=")
   emit equality code
   else if (op == ":=")
   emit assignment code
   else
   processError(compiler error since function code should not be called with
   illegal arguments)
   */

   if(op == "program")
      emitPrologue(operand1);
   
   else if(op == "end" && operand1 == ";")
      ;
   
   else if(op == "end" && operand1 == ".")
      emitEpilogue();
   
   else if (op == "read")
      emitReadCode(operand1);
   
   else if (op == "write")
      emitWriteCode(operand1);
   
   else if (op == "+") // this must be binary '+'
      emitAdditionCode(operand1, operand2);
      
   else if (op == "-") // this must be binary '-'
      emitSubtractionCode(operand1, operand2);
      
   else if (op == "neg") // this must be unary '-'
      emitNegationCode(operand1);
      
   else if (op == "not")
      emitNotCode(operand1);
   
   else if (op == "*")
      emitMultiplicationCode(operand1, operand2);
   
   else if (op == "div")
      emitDivisionCode(operand1, operand2);
   
   else if (op == "mod")
      emitModuloCode(operand1, operand2);
   
   else if (op == "and")
      emitAndCode(operand1, operand2);
   
   else if (op == "or")
      emitOrCode(operand1, operand2);
   
   else if (op == "<>")
      emitInequalityCode(operand1, operand2);
   
   else if (op == "=")
      emitEqualityCode(operand1, operand2);
   
   else if(op == "<")
      emitLessThanCode(operand1, operand2);
   
   else if(op == "<=")
      emitLessThanOrEqualToCode(operand1, operand2);
   
   else if(op == ">")
      emitGreaterThanCode(operand1, operand2);
   
   else if(op == ">=")
      emitGreaterThanOrEqualToCode(operand1, operand2);
   
   else if (op == ":=")
      emitAssignCode(operand1, operand2);
   
   else if (op == "then")
      emitThenCode(operand1);
   
   else if (op == "post_if")
      emitPostIfCode(operand1);
   
   else if (op == "else")
      emitElseCode(operand1);
   
   else if (op == "do")
      emitDoCode(operand1);
   
   else if (op == "repeat")
      emitRepeatCode();
   
   else if (op == "while")
      emitWhileCode();
   
   else if (op == "post_while")
      emitPostWhileCode(operand1, operand2);
   
   else if (op == "until")
      emitUntilCode(operand1, operand2);
   
   else
     processError("function code called with illegal arguments");
}

void Compiler::emit(string label, string instruction, string operands, string comment)
{
   if(!label.empty() || !instruction.empty() || !operands.empty())
   {
      objectFile << left << setw(8) << label;
      objectFile << setw(8) << instruction;
      objectFile << setw(24) << operands;
   }
   if(!comment.empty())
      objectFile << "; " << comment << "\n" << flush;
   else
      objectFile << "\n" << flush;
   
}

void Compiler::emitPrologue(string progName, string operand2)
{
   objectFile << "; ANDREW REGAN, LEO OLVERA" << setw(7) << "" << right << ctime(&now);
   objectFile << "%INCLUDE \"Along32.inc\"" << endl << "%INCLUDE \"Macros_Along.inc\"" << endl << endl;;
   emit("SECTION", ".text");
   emit("global", "_start", "", "program " + progName.substr(0,15) + "\n");
   emit("_start:");
}

void Compiler::emitEpilogue(string operand1, string operand2)
{
   emit("","Exit", "{0}");
   emitStorage();
}

void Compiler::emitStorage()
{
   objectFile << "\n";
   emit("SECTION", ".data");
   map<string, SymbolTableEntry>::iterator itr;
   for(itr = symbolTable.begin(); itr != symbolTable.end(); itr++)
   {
      if(itr->second.getAlloc() == allocation(YES) && itr->second.getMode() == modes(CONSTANT))
      {
         emit(itr->second.getInternalName().substr(0,15), "dd", itr->second.getValue(), itr->first);
      }
   }
   
   objectFile << "\n";
   emit("SECTION", ".bss");
   for(itr = symbolTable.begin(); itr != symbolTable.end(); itr++)
   {
      if(itr->second.getAlloc() == allocation(YES) && itr->second.getMode() == modes(VARIABLE))
      {
         emit(itr->second.getInternalName().substr(0,15), "resd", to_string(itr->second.getUnits()), itr->first);
      }
   }
}

void Compiler::emitAdditionCode(string operand1,string operand2) //add operand1 to operand2
{
   /*
   if type of either operand is not integer
   processError(illegal type)
   if the A Register holds a temp not operand1 nor operand2 then
   emit code to store that temp into memory
   change the allocate entry for the temp in the symbol table to yes
   deassign it
   if the A register holds a non-temp not operand1 nor operand2 then deassign it
   if neither operand is in the A register then
   emit code to load operand2 into the A register
   emit code to perform register-memory addition
   deassign all temporaries involved in the addition and free those names for reuse
   A Register = next available temporary name and change type of its symbol table entry to integer
   push the name of the result onto operandStk
   */
   
   //cout << endl << "helloAdd" << endl;
   //cout << "Evaluate: " << operand2 << " + " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
      processError("binary \'+\' requires integer operands");
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }

   if(contentsOfAReg == operand1)
      emit("", "add", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " + " + operand2);
   else if(contentsOfAReg == operand2)
      emit("", "add", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " + " + operand1);
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeAdd" << endl << endl;
}

void Compiler::emitDivisionCode(string operand1,string operand2) //divide operand2 by operand1
{
   /*
   if type of either operand is not integer
   processError(illegal type)
   if the A Register holds a temp not operand2 then
   emit code to store that temp into memory
   change the allocate entry for it in the symbol table to yes
   deassign it
   if the A register holds a non-temp not operand2 then deassign it
   if operand2 is not in the A register
   emit instruction to do a register-memory load of operand2 into the A register
   emit code to extend sign of dividend from the A register to edx:eax
   emit code to perform a register-memory division
   deassign all temporaries involved and free those names for reuse
   A Register = next available temporary name and change type of its symbol table entry to integer
   push the name of the result onto operandStk
   */
   
   //cout << endl << "helloDiv" << endl;
   //cout << "Evaluate: " << operand2 << " div " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
      processError("binary \'div\' requires integer operands");
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   emit("", "cdq", "", "sign extend dividend from eax to edx:eax");
   
   if(contentsOfAReg == operand2)
      emit("", "idiv", "dword [" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " div " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "idiv", "dword [" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " div " + operand2); 
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeDiv" << endl << endl;
}

void Compiler::emitAndCode(string operand1,string operand2) //and operand1 to operand2
{
   /*
   if type of either operand is not boolean
   processError(illegal type)
   if the A Register holds a temp not operand1 nor operand2 then
   emit code to store that temp into memory
   change the allocate entry for the temp in the symbol table to yes
   deassign it
   if the A register holds a non-temp not operand1 nor operand2 then deassign it
   if neither operand is in the A register then
   emit code to load operand2 into the A register
   emit code to perform register-memory and
   deassign all temporaries involved in the and operation and free those names for reuse
   A Register = next available temporary name and change type of its symbol table entry to boolean
   push the name of the result onto operandStk
   */
   
   //cout << endl << "helloAnd" << endl;
   //cout << "Evaluate: " << operand2 << " && " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != BOOLEAN || symbolTable.at(operand2).getDataType() != BOOLEAN)
      processError("binary \'and\' requires boolean operands");
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   if(contentsOfAReg == operand2)
      emit("", "and", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " and " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "and", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " and " + operand2);
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }

   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   pushOperand(contentsOfAReg);
   //cout << "goodbyeAnd" << endl << endl;
   
}

void Compiler::emitEqualityCode(string operand1,string operand2) //test whether operand2 equals operand1
{
   /*
   if types of operands are not the same
   processError(incompatible types)
   if the A Register holds a temp not operand1 nor operand2 then
   emit code to store that temp into memory
   change the allocate entry for it in the symbol table to yes
   deassign it
   if the A register holds a non-temp not operand2 nor operand1 then deassign it
   if neither operand is in the A register then
   emit code to load operand2 into the A register
   emit code to perform a register-memory compare
   emit code to jump if equal to the next available Ln (call getLabel)
   emit code to load FALSE into the A register
   insert FALSE in symbol table with value 0 and external name false
   emit code to perform an unconditional jump to the next label (call getLabel should be L(n+1))
   emit code to label the next instruction with the first acquired label Ln
   emit code to load TRUE into A register
   insert TRUE in symbol table with value -1 and external name true
   emit code to label the next instruction with the second acquired label L(n+1)
   deassign all temporaries involved and free those names for reuse
   A Register = next available temporary name and change type of its symbol table entry to boolean
   push the name of the result onto operandStk
   */
   
   //cout << endl << "helloEqual" << endl;
   //cout << "Evaluate: " << operand2 << " = " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != symbolTable.at(operand2).getDataType())
   {
      processError("binary \'=\' requires operands of the same type");
   }
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
   
   emit("", "je", firstLabel, "if " + operand2 + " = " + operand1 + " then jump to set eax to TRUE");
   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeEqual" << endl << endl;
}

void Compiler::emitAssignCode(string operand1,string operand2) //assign the value of operand1 to operand2
{
   /*
   if types of operands are not the same
   processError(incompatible types)
   if storage mode of operand2 is not VARIABLE
   processError(symbol on left-hand side of assignment must have a storage mode of VARIABLE)
   if operand1 = operand2 return
   if operand1 is not in the A register then
   emit code to load operand1 into the A register
   emit code to store the contents of that register into the memory location pointed to by
   operand2
   set the contentsOfAReg = operand2
   if operand1 is a temp then free its name for reuse
   operand2 can never be a temporary since it is to the left of ':='
   */
   
   //cout << endl << "helloAssign" << endl;
   //cout << "Evaluate: " << operand2 << " = " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;//" Type: " << symbolTable.at(operand1).getDataType() << endl;
   //cout << "rhs: " << operand2 << endl;//" Type: " << symbolTable.at(operand2).getDataType() << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != symbolTable.at(operand2).getDataType())
   {
      processError("incompatible types for operator \':=\'");
   }
   
   if(symbolTable.at(operand2).getMode() != VARIABLE)
   {
      processError("symbol on left-hand side of assignment must have a storage mode of VARIABLE");
   }
   
   if(operand1 == operand2)
   {
      return;
   }
   
   if(contentsOfAReg != operand1)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand1);
   }
   
   emit("", "mov", "[" + symbolTable.at(operand2).getInternalName() + "],eax", operand2 + " = AReg");
   contentsOfAReg = operand2;
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   //cout << "goodbyeAssign" << endl << endl;
}

void Compiler::emitReadCode(string operand, string operand2)
{
   /*
   string name
   while (name is broken from list (operand) and put in name != "")
   {
   if name is not in symbol table
   processError(reference to undefined symbol)
   if data type of name is not INTEGER
   processError(can't read variables of this type)
   if storage mode of name is not VARIABLE
   processError(attempting to read to a read-only location)
   emit code to call the Irvine ReadInt function
   emit code to store the contents of the A register at name
   set the contentsOfAReg = name
   }
   */
   
   //cout << endl << "helloRead" << endl;
   //cout << "reading: " << operand << endl;
   vector<string> myNames;
   map<string, SymbolTableEntry>::iterator itr;
   myNames.push_back("");
   
   for(uint i = 0, wordCount = 0; i < operand.length(); i++)
   {
      if(operand.at(i) == ',')
      {
         wordCount++;
         myNames.push_back("");
         continue;
      }
      myNames.at(wordCount) += operand.at(i);
   }
   
   while(myNames.size() > 0)
   {
      itr = symbolTable.find(myNames.front().substr(0,15));
      if(itr == symbolTable.end())
      {
         processError("reference to undefined symbol");
      }
      
      if(itr->second.getDataType() != INTEGER)
      {
         processError("cannot read variables of this type");
      }
      
      if(itr->second.getMode() != VARIABLE)
      {
         processError("attempting to read to a read-only location");
      }
      
      emit("", "call", "ReadInt", "read int; value placed in eax");
      emit("", "mov", "[" + itr->second.getInternalName() + "],eax", "store eax at " + myNames.front().substr(0,15));
      contentsOfAReg = myNames.front().substr(0,15);
      myNames.erase(myNames.begin());
   }
   //cout << "goodbyeRead" << endl << endl;
}

void Compiler::emitWriteCode(string operand, string operand2)
{
   /*
   string name
   static bool definedStorage = false
   while (name is broken from list (operand) and put in name != "")
   {
   if name is not in symbol table
   processError(reference to undefined symbol)
   if name is not in the A register
   emit the code to load name in the A register
   set the contentsOfAReg = name
   if data type of name is INTEGER
   emit code to call the Irvine WriteInt function
   else // data type is BOOLEAN
   {
   emit code to compare the A register to 0
   acquire a new label Ln
   emit code to jump if equal to the acquired label Ln
   emit code to load address of TRUE literal in the D register
   acquire a second label L(n + 1)
   emit code to unconditionally jump to label L(n + 1)
   emit code to label the next line with the first acquired label Ln
   emit code to load address of FALSE literal in the D register
   emit code to label the next line with the second acquired label L(n + 1)
   emit code to call the Irvine WriteString function
   if static variable definedStorage is false
   {
   set definedStorage to true
   output an endl to objectFile
   emit code to begin a .data SECTION
   emit code to create label TRUELIT, instruction db, operands 'TRUE',0
   emit code to create label FALSELIT, instruction db, operands 'FALSE',0
   output an endl to objectFile
   emit code to resume .text SECTION
   } // end if
   } // end else
   emit code to call the Irvine Crlf function
   } // end while
   */
   
   //cout << endl << "helloWrite" << endl;
   //cout << "writing: " << operand << endl;
   vector<string> myNames;
   static bool definedStorage = false;
   myNames.push_back("");
   map<string, SymbolTableEntry>::iterator itr;
   
   for(uint i = 0, wordCount = 0; i < operand.length(); i++)
   {
      if(operand.at(i) == ',')
      {
         wordCount++;
         myNames.push_back("");
         continue;
      }
      myNames.at(wordCount) += operand.at(i);
   }
   
   while(myNames.size() > 0)
   {
      itr = symbolTable.find(myNames.front().substr(0,15));
      if(itr == symbolTable.end())
      {
         processError("reference to undefined symbol");
      }
      
      if(contentsOfAReg != itr->first)
      {
         emit("", "mov", "eax,[" + itr->second.getInternalName() + "]", "load " + myNames.front() + " in eax");
         contentsOfAReg = myNames.front();
      }
      
      if(itr->second.getDataType() == INTEGER)
      {
         emit("", "call", "WriteInt", "write int in eax to standard out");
      }
      else
      {
         emit("", "cmp", "eax,0", "compare to 0");
         string firstLabel  = getLabel();
         emit("", "je", firstLabel, "jump if equal to print FALSE");
         emit("", "mov", "edx,TRUELIT", "load address of TRUE literal in edx");
         string secondLabel = getLabel();
         emit("", "jmp", secondLabel, "unconditionally jump to " + secondLabel);
         emit(firstLabel + ":");
         emit("", "mov", "edx,FALSLIT", "load address of FALSE literal in edx");
         emit(secondLabel + ":");
         emit("", "call", "WriteString", "write string to standard out");
         if(definedStorage == false)
         {
            definedStorage = true;
            emit();
            emit("SECTION", ".data");
            emit("TRUELIT", "db", "\'TRUE\',0", "literal string TRUE");
            emit("FALSLIT", "db", "\'FALSE\',0", "literal string FALSE");
            emit();
            emit("SECTION", ".text");
         }
      }
      emit("", "call", "Crlf", "write \\r\\n to standard out");
      myNames.erase(myNames.begin());
   }
   //cout << "goodbyeWrite" << endl << endl;
}

void Compiler::emitSubtractionCode(string operand1, string operand2)    // op2 -  op1
{
   //cout << endl << "helloSubtraction" << endl;
   //cout << "Evaluate: " << operand2 << " - " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
      processError("binary \'-\' requires integer operands");
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   if(contentsOfAReg == operand2)
      emit("", "sub", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " - " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "sub", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " - " + operand2);
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   pushOperand(contentsOfAReg);
   //cout << "goodbyeSubtraction" << endl << endl;
}

void Compiler::emitMultiplicationCode(string operand1, string operand2) // op2 *  op1
{
   //cout << endl << "" << endl;
   //cout << "Evaluate: " << operand2 << " * " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
      processError("binary \'*\' requires integer operands");
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   if(contentsOfAReg == operand2)
      emit("", "imul", "dword [" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " * " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "imul", "dword [" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " * " + operand2);
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeMult" << endl << endl;
}

void Compiler::emitModuloCode(string operand1, string operand2)         // op2 %  op1
{
   //cout << endl << "helloMod" << endl;
   //cout << "Evaluate: " << operand2 << " mod " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
      processError("binary \'mod\' requires integer operands");
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   emit("", "cdq", "", "sign extend dividend from eax to edx:eax");
   
   if(contentsOfAReg == operand2)
      emit("", "idiv", "dword [" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " div " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "idiv", "dword [" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " div " + operand2); 
   
   emit("", "xchg", "eax,edx", "exchange quotient and remainder");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeMod" << endl << endl;
}

void Compiler::emitNegationCode(string operand1, string operand2)           // -op1
{
   //cout << endl << "helloNegation" << endl;
   //cout << "Evaluate: -" << operand1 << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER)
      processError("unary \'-\' requires an integer operand");
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand1)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand1)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand1);
   }
   
   emit("", "neg", "eax", "AReg = -AReg");

   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }

   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(INTEGER);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeNegation" << endl << endl;
}

void Compiler::emitNotCode(string operand1, string operand2)               // !op1
{
   //cout << endl << "helloNot" << endl;
   //cout << "Evaluate: not " << operand1 << endl;
   
   if(symbolTable.at(operand1).getDataType() != BOOLEAN)
      processError("unary \'not\' requires a boolean operand");
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand1)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand1)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand1);
   }
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   emit("", "not", "eax", "AReg = !AReg");

   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbyeNot" << endl << endl; 
}

void Compiler::emitOrCode(string operand1, string operand2)             // op2 || op1
{
   //cout << endl << "helloOr" << endl;
   //cout << "Evaluate: " << operand2 << " || " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != BOOLEAN || symbolTable.at(operand2).getDataType() != BOOLEAN)
      processError("binary \'or\' requires boolean operands");
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   if(contentsOfAReg == operand2)
      emit("", "or", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2 + " or " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "or", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand1 + " or " + operand2);
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   pushOperand(contentsOfAReg);
   
   //cout << "goodbyeOr" << endl << endl;
}

void Compiler::emitInequalityCode(string operand1, string operand2)    // op2 != op1
{
   //cout << endl << "helloNotEqual" << endl;
   //cout << "Evaluate: " << operand2 << " <> " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
    if(symbolTable.at(operand1).getDataType() != symbolTable.at(operand2).getDataType())
   {
      processError("binary \'<>\' requires operands of the same type");
   }
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand1 && contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
   else if(contentsOfAReg == operand1)
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
   emit("", "jne", firstLabel, "if " + operand2  + " <> " + operand1 + " then jump to set eax to TRUE");
   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   pushOperand(contentsOfAReg);
   //cout << "goodbyeNotEqual" << endl;
}

void Compiler::emitLessThanCode(string operand1, string operand2)       // op2 <  op1
{
   //cout << endl << "hello<" << endl;
   //cout << "Evaluate: " << operand2 << " < " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
   {
      processError("binary \'<\' requires integer operands");
   }
   
   if(isTemporary(contentsOfAReg) && (contentsOfAReg != operand2))
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && (contentsOfAReg != operand2))
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
      emit("", "jl", firstLabel, "if " + operand2 + " < " + operand1 + " then jump to set eax to TRUE");
   }
   else if(contentsOfAReg == operand1)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
      emit("", "jl", firstLabel, "if " + operand1 + " < " + operand2 + " then jump to set eax to TRUE");
   }

   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbye<" << endl << endl;
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2) // op2 <= op1
{
   //cout << endl << "hello<=" << endl;
   //cout << "Evaluate: " << operand2 << " <= " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
   {
      processError("binary \'<=\' requires integer operands");
   }
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
      emit("", "jle", firstLabel, "if " + operand2 + " <= " + operand1 + " then jump to set eax to TRUE");
   }
   else if(contentsOfAReg == operand1)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
      emit("", "jle", firstLabel, "if " + operand1 + " <= " + operand2 + " then jump to set eax to TRUE");
   }

   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbye<=" << endl << endl;
}

void Compiler::emitGreaterThanCode(string operand1, string operand2)    // op2 >  op1
{
   //cout << endl << "hello>" << endl;
   //cout << "Evaluate: " << operand2 << " > " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
   {
      processError("binary \'>\' requires integer operands");
   }
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
      emit("", "jg", firstLabel, "if " + operand2 + " > " + operand1 + " then jump to set eax to TRUE");
   }
   else if(contentsOfAReg == operand1)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
      emit("", "jg", firstLabel, "if " + operand1 + " > " + operand2 + " then jump to set eax to TRUE");
   }

   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbye>" << endl << endl;
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2) // op2 >= op1
{
   //cout << endl << "hello>=" << endl;
   //cout << "Evaluate: " << operand2 << " >= " << operand1 << endl;
   //cout << "rhs: " << operand1 << endl;
   //cout << "lhs: " << operand2 << endl;
   //cout << "eax: " << contentsOfAReg << endl;
   
   if(symbolTable.at(operand1).getDataType() != INTEGER || symbolTable.at(operand2).getDataType() != INTEGER)
   {
      processError("binary \'>=\' requires integer operands");
   }
   
   if(isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      emit("", "mov", "[" + contentsOfAReg + "],eax", "deassign AReg");
      symbolTable.at(contentsOfAReg).setAlloc(YES);
      contentsOfAReg = "";
   }
   
   if(!isTemporary(contentsOfAReg) && contentsOfAReg != operand2)
   {
      contentsOfAReg = "";
   }
   
   if(contentsOfAReg != operand2)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "AReg = " + operand2);
      contentsOfAReg = operand2;
   }
   
   string firstLabel = getLabel();
   string secondLabel = getLabel();
   
   if(contentsOfAReg == operand2)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "compare " + operand2 + " and " + operand1);
      emit("", "jge", firstLabel, "if " + operand2 + " >= " + operand1 + " then jump to set eax to TRUE");
   }
   else if(contentsOfAReg == operand1)
   {
      emit("", "cmp", "eax,[" + symbolTable.at(operand2).getInternalName() + "]", "compare " + operand1 + " and " + operand2);
      emit("", "jge", firstLabel, "if " + operand1 + " >= " + operand2 + " then jump to set eax to TRUE");
   }

   emit("", "mov", "eax,[FALSE]", "else set eax to FALSE");
   
   if(!myFalseDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("false",myFalse));
      myFalseDeclared = true;
   }
   
   emit("", "jmp", secondLabel, "unconditionally jump");
   emit(firstLabel + ":");
   emit("", "mov", "eax,[TRUE]", "set eax to TRUE");
   
   if(!myTrueDeclared)
   {
      if(symbolTable.size() >= 256)
      {
         processError("symbol table overflow");
      }
      symbolTable.insert(pair<string,SymbolTableEntry>("true",myTrue));
      myTrueDeclared = true;
   }
   
   emit(secondLabel + ":");
   
   if(isTemporary(operand1) && contentsOfAReg != operand1)
   {
      freeTemp();
   }
   if(isTemporary(operand2) && contentsOfAReg != operand2)
   {
      freeTemp();
   }
   
   if(isTemporary(contentsOfAReg))
   {
      freeTemp();
      contentsOfAReg = "";
   }
   
   contentsOfAReg = getTemp();
   symbolTable.at(contentsOfAReg).setDataType(BOOLEAN);
   //cout << "Push: " << contentsOfAReg << endl;
   pushOperand(contentsOfAReg);
   //cout << "goodbye>=" << endl << endl;
}

void Compiler::emitThenCode(string operand1, string operand2)
{
   string tempLabel;
   if(symbolTable.at(operand1).getDataType() != BOOLEAN)
   {
      processError("if predicate must be type BOOLEAN");
   }
   
   tempLabel = getLabel();
   
   if(contentsOfAReg != operand1)
   {
      emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand1);
   }
   
   emit("","cmp","eax,0", "compare eax to 0");
   emit("","je", tempLabel, "if " + contentsOfAReg + " is false then jump to end of if");
   pushOperand(tempLabel);
   
   if(isTemporary(operand1))
   {
      freeTemp();
   }
   
   contentsOfAReg = ""; 
}

void Compiler::emitElseCode(string operand1, string operand2)
{
   string tempLabel;
   tempLabel = getLabel();
   
   emit("", "jmp", tempLabel, "jump to end if");
   emit(operand1 + ":", "", "", "else");
   pushOperand(tempLabel);
   contentsOfAReg = "";
}

void Compiler::emitPostIfCode(string operand1, string operand2)
{
   emit(operand1 + ":", "", "", "end if");
   contentsOfAReg = "";
}

void Compiler::emitWhileCode(string operand1, string operand2)
{
   string tempLabel;
   tempLabel = getLabel();
   
   emit(tempLabel + ":", "", "", "while");
   pushOperand(tempLabel);
   contentsOfAReg = ""; 
}

void Compiler::emitDoCode(string operand1, string operand2)
{
   string tempLabel;
   if(symbolTable.at(operand1).getDataType() != BOOLEAN)
      processError("while predicate must be of BOOLEAN type");
   
   tempLabel = getLabel();
   emit("", "cmp", "eax,0", "compare eax to 0");
   emit("", "je", tempLabel, "if " + contentsOfAReg + " is false then jump to end while");
   
   pushOperand(tempLabel);
   
   if(isTemporary(operand1))
      freeTemp();
   
   contentsOfAReg = "";
}

void Compiler::emitPostWhileCode(string operand1, string operand2)
{
   emit("", "jmp", operand2, "end while");
   emit(operand1 + ":");
   contentsOfAReg = "";
}

void Compiler::emitRepeatCode(string operand1, string operand2)
{
   string tempLabel;
   tempLabel = getLabel();
   emit(tempLabel + ":", "", "", "repeat");
   pushOperand(tempLabel);
   contentsOfAReg = "";
}

void Compiler::emitUntilCode(string operand1, string operand2)
{
   if(symbolTable.at(operand1).getDataType() != BOOLEAN)
      processError("until predicate must be of type BOOLEAN");
   
   if(contentsOfAReg != operand1)
      emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]", "AReg = " + operand2);
   
   emit("", "cmp", "eax,0", "compare eax to 0");
   emit("", "je", operand2, "until " + operand1 + " is true");
   
   if(isTemporary(operand1))
      freeTemp();
   contentsOfAReg = "";
}

/* ****************************************************END EMIT FUNCTIONS**************************************************** */

/* ****************************************************STAGE1**************************************************** */

void Compiler::freeTemp()
{
   /*
   currentTempNo--;
   if (currentTempNo < -1)
   processError(compiler error, currentTempNo should be ≥ –1)
   */
   
   currentTempNo--;
   if (currentTempNo < -1)
      processError("compiler error, currentTempNo should be >= -1");
}

string Compiler::getTemp()
{
   /*
   string temp;
   currentTempNo++;
   temp = "T" + currentTempNo;
   if (currentTempNo > maxTempNo)
   insert(temp, UNKNOWN, VARIABLE, "", NO, 1)
   maxTempNo++
   return temp
   */
   
   string temp;
   currentTempNo++;
   temp = "T" + to_string(currentTempNo);
   //cout << temp << endl;
   if(currentTempNo > maxTempNo)
   {
      insert(temp, UNKNOWN, VARIABLE, "", NO, 1);
      maxTempNo++;
   }
   return temp;
}

string Compiler::getLabel()
{
   static int labelNo = 0;
   string s = ".L";
   s = s + to_string(labelNo);
   labelNo++;
   return s;
}

void Compiler::execStmts()      // stage 1, production 2
{
   nextToken();
   //cout << "execStmts: " + token << endl;
   while(isNonKeyId(token) || token == "read" || token == "write"
      || token == "if" || token == "while" || token == "repeat"
      || token == ";" || token == "begin" )
   {
      //cout << token << endl;
      execStmt();
      //cout << token << endl;
      if(token == ";")
         nextToken();
      //cout << token << endl;
   }
}

void Compiler::execStmt()       // stage 1, production 3
{
   //cout << token << endl;
   if(isNonKeyId(token))
      assignStmt();
   else if(token == "read")
      readStmt();
   else if(token == "write")
      writeStmt();
   else if(token == "if")
      ifStmt();
   else if(token == "while")
      whileStmt();
   else if(token == "repeat")
      repeatStmt();
   else if(token == ";")
      nullStmt();
   else if(token == "begin")
      beginEndStmt();
}

void Compiler::assignStmt()     // stage 1, production 4
{
   //cout << token << endl;
   if(!isNonKeyId(token))
      processError("nonKeyId expected");
    //cout << "Push: " << token << endl;
   pushOperand(token);
   if(nextToken() != ":=")
      processError("assignment operator \":=\" expected");
   //cout << "Push: :=" << endl;
   pushOperator(":=");
   express();
   if(isNonKeyId(token))
      processError("one of \"*\", \"and\", \"div\", \"mod\", \")\", \"+\", \"-\", \";\", \"<\", \"<=\", \"<>\", \"=\", \">\", \">=\", or \"or\" expected");
   if(token != ";")
      processError("semicolon expected"); 
   string operand1 = popOperand();
   string operand2 = popOperand();
   code(popOperator(),operand1, operand2);
   //cout << token << endl;
}

void Compiler::readStmt()       // stage 1, production 5
{
   string x;
   if(token != "read")
      processError("keyword read expected");
   if(nextToken() == "(")
   {
      if(!isNonKeyId(nextToken()))
         processError("nonKeyId expected in read statement");
      
      x = ids();

      if(token != ")")
         processError("\',\' or \')\' expected after nonKeyId in read statement");
      
      code("read", x);
      
      if(nextToken() != ";")
         processError("semicolon expected");
   }
   else
      processError("opening parentheses expected in read statement");
}

void Compiler::writeStmt()      // stage 1, production 7
{
   string x;
   if(token != "write")
      processError("keyword read expected");
   if(nextToken() == "(")
   {
      if(!isNonKeyId(nextToken()))
         processError("nonKeyId expected in write statement");
      
      x = ids();
      
      if(token != ")")
         processError("\',\' or \')\' expected after nonKeyId in write statement");
      
      code("write", x);
      
      if(nextToken() != ";")
         processError("semicolon expected");
   }   
   else
      processError("opening parentheses expected in write statement");
}

void Compiler::express()        // stage 1, production 9
{
   term();
   expresses();
}

void Compiler::expresses()    // stage 1, production 10
{
   if(isRelational(token))
   {
      //cout << "Push: " << token << endl;
      pushOperator(token);
      term();
      string operand1 = popOperand();
      string operand2 = popOperand();
      string operator1 = popOperator();
      code(operator1,operand1.substr(0,15),operand2.substr(0,15));
      //cout << "Pop: " << operator1 << endl;
      //cout << "Pop: " << operand1 << endl;
      //cout << "Pop: " << operand2 << endl;
      expresses();
   }
}

void Compiler::term()           // stage 1, production 11
{
  factor();
  terms();
}

void Compiler::terms()          // stage 1, production 12
{
   if(isAddLevel(token))
   {
      //cout << "Push: " << token << endl;
      pushOperator(token);
      factor();
      string operand1 = popOperand();
      string operand2 = popOperand();
      string operator1 = popOperator();
      code(operator1,operand1.substr(0,15),operand2.substr(0,15));
      //cout << "Pop: " << operator1 << endl;
      //cout << "Pop: " << operand1 << endl;
      //cout << "Pop: " << operand2 << endl;
      terms();
   }
}

void Compiler::factor()        // stage 1, production 13
{
   part();
   factors();
}

void Compiler::factors()        // stage 1, production 14
{
   if(isMultLevel(nextToken()))
   {
      //cout << "Push: " << token << endl;
      pushOperator(token);
      part();
      string operand1 = popOperand();
      string operand2 = popOperand();
      string operator1 = popOperator();
      code(operator1,operand1.substr(0,15),operand2.substr(0,15));
      //cout << "Pop: " << operator1 << endl;
      //cout << "Pop: " << operand1 << endl;
      //cout << "Pop: " << operand2 << endl;
      factors();
   }
}

void Compiler::part()           // stage 1, production 15
{
   string y;
   string x = nextToken();
   //cout << x << endl;
   if(x == "not")
   {
      y = nextToken();
      if(y == "(")
      {
         express();
         if(token != ")")
            processError("end parentheses expected");
         code("not", popOperand());
      }
      else if(isBoolean(y))
      {
         if(y == "true")
            pushOperand("false");
         else
            pushOperand("true");
      }
      else if(isNonKeyId(y))
      {
         code("not", y);
      }
      else
         processError("expected \'(\', nonKeyId, or boolean");
   }
   else if(x == "+")
   {
      y = nextToken();
      if(y == "(")
      {
         express();
         if(token != ")")
            processError("end parentheses expected");
      }
      else if(isInteger(y) || isNonKeyId(y))
      {
         pushOperand(y);
      }
      else
         processError("expected \'(\', nonKeyId, or integer");
   }
   else if(x == "-")
   {
      y = nextToken();
      if(y == "(")
      {
         express();
         if(token != ")")
            processError("end parentheses expected");
         code("neg", popOperand());
      }
      else if(isInteger(y))
      {
         //cout << "push: -" << y << endl;
         pushOperand("-" + y);
      }
      else if(isNonKeyId(y))
      {
         code("neg", y);
      }
      else
      {
         processError("expected \'(\', nonKeyId, or integer");
      }
   }
   else if(isInteger(x) || isBoolean(x) || isNonKeyId(x))
   {
      //cout << x << endl;
      //cout << "Push: " << token << endl;
      pushOperand(x);
      //cout << symbolTable.at(token).getInternalName() << endl;
   }
   else if(x == "(")
   {
      express();
      if(token != ")")
         processError("end parentheses expected");
   }
   else
      processError("expected nonKeyId, integer, \"not\", \"true\", \"false\", \'(\', \'+\', or \'-\'");
}

/* ****************************************************END STAGE1**************************************************** */

/* ****************************************************START STAGE2**************************************************** */

void Compiler::ifStmt()         // stage 2, production 3
{
  if(token != "if")
     processError("keyword \'if\' expected");
  
  express();

  if(token != "then")
     processError("keyword \'then\' expected");
  
  nextToken();
  if(token == "else")
      processError("then clause may not be empty");
      
  //cout << token << endl;
  code("then", popOperand());
  
  execStmt();
  
  if(token == ";")
     nextToken();
  //cout << token << endl;
  elsePt();
  //cout << token << endl;
}

void Compiler::elsePt()         // stage 2, production 4
{
  //cout << "elsePt: " << token << endl;
  string operand1;
  if(token == "else")
  {
     operand1 = popOperand();
     code("else", operand1);
     nextToken();
     execStmt();
     operand1 = popOperand();
     code("post_if", operand1);
  }
  
  else if(isNonKeyId(token) || token == "read" || token == "write"
      || token == "if" || token == "while" || token == "repeat"
      || token == ";" || token == "until" || token == "begin" || token == "end")
  {
     operand1 = popOperand();
     code("post_if", operand1);
  }
  
  else
     processError("nonKeyId, \'read\', \'end\', \'write\', \'if\', \'while\', \';\', \'until\', \'begin\', \'else\' or \'repeat\' expected");
}

void Compiler::whileStmt()      // stage 2, production 5
{
  if(token != "while")
      processError("keyword \'while\' expected");
   
   code("while");
   
   express();
   
   if(token != "do")
      processError("keyword \'do\' expected");
   
   code("do", popOperand());
   
   nextToken();
   execStmt();
   
   string operand1 = popOperand();
   string operand2 = popOperand();
   code("post_while", operand1, operand2);
}

void Compiler::repeatStmt()     // stage 2, production 6
{
  if(token != "repeat")
     processError("keyword \'repeat\' expected");
  
  code("repeat");
  
  execStmts();
  
  
  if(token != "until")
     processError("keyword \'until\' expected");
  
  express();
  
  string operand1 = popOperand();
  string operand2 = popOperand();
  code("until", operand1, operand2);
  
  if(token != ";")
     processError("semicolon excepted");
}

void Compiler::nullStmt()       // stage 2, production 7
{
  if(token != ";")
     processError("semicolon expected");
}

/* ****************************************************END STAGE2**************************************************** */