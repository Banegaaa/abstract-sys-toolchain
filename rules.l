%{
    #include "define.h"
%}

%%

<<EOF>>                             return ENDOFFILE;
"\n"                                return NEWLINE;
:                                   return COLON;
,                                   return COMMA;
"#"                                 return COMMENT;
[a-zA-Z_][a-zA-Z_0-9]*":"         return LABEL;
"sp"                                return SP_REG;
"pc"                                return PC_REG;
"psw"                               return PSW_REG;

"status"                            return STATUS_REG;
"handler"                           return HANDLER_REG;
"cause"                             return CAUSE_REG;

"[%r"[0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15]"]"  return REG_IND_ADR;
"[%r"[0-15]" + "[a-zA-Z_][a-zA-Z_0-9]*"]"        return REG_POM_S_ADR;
"[%r"[0-15]" + "[0-9]+"]"             return REG_POM_L_ADR;
"[%r"[0-15]" + "[x0-xf]+"]"      return REG_POM_HEXL_ADR;


 
"r"[0-15]                            return REG;

"*"                                 return ADR;

"%"                                 return PC_REL_ADR;
"$"                                 return IMM_ADR;


".global"                           return GLOBAL;
".extern"                           return EXTERN;
".section"                          return SECTION;
".word"                             return WORD;
".skip"                             return SKIP;
".equ"                              return EQU;
".end"                              return ENDD;
"halt"                              return HALT;
"int"                               return INT;
"iret"                              return IRET;
"call"                              return CALL;
"ret"                               return RET;
"jmp"                               return JMP;
"beq"                               return BEQ;
"bne"                               return BNE;
"bgt"                               return BGT;
"push"                              return PUSH;
"pop"                               return POPP;
"xchg"                              return XCHG;
"add"                               return ADD;
"sub"                               return SUB;
"mul"                               return MUL;
"div"                               return DIV;
"cmp"                               return CMP;
"not"                               return NOT;
"and"                               return AND;
"or"                                return OR;
"xor"                               return XOR;
"test"                              return TEST;
"shl"                               return SHL;
"shr"                               return SHR;
"ld"                                return LD;
"st"                                return ST;
"csrrd"                             return CSRRD;
"csrwr"                             return CSRWR;
[a-zA-Z_][a-zA-Z_0-9]*              return NAME;
[0-9]+                              return DEC_LITERAL;
"0x"[a-fA-F0-9]+                    return HEX_LITERAL;
[ \t]                                ;
.                                    ;

%%

int yywrap(){return 1;}

