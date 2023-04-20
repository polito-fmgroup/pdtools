%{
/**CFile***********************************************************************

  FileName    [expr.l]

  PackageName [expr]

  Synopsis    [Lexical analyzer for PdTrav expressions.]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi and Stefano Quer]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: pdtrav@polito.it. ]

******************************************************************************/


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

int state; 

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


%}

/*---------------------------------------------------------------------------*/
/* Lexical analyzer rules                                                    */
/*---------------------------------------------------------------------------*/


%%

[ \n\t]                 ;
"#".*\n                 ;

"SPEC"                  return(SPEC);
"INVARSPEC"             return(INVARSPEC);

"EX"                    return(EX);
"AX"                    return(AX);
"EF"                    return(EF);
"AF"                    return(AF);
"EG"                    return(EG);
"AG"                    return(AG);
"E"                     return(EE);
"A"                     return(AA);
"U"                     return(UNTIL);
"BU"                    return(BUNTIL);
"EBF"                   return(EBF);
"ABF"                   return(ABF);
"EBG"                   return(EBG);
"ABG"                   return(ABG);
"MIN"                   return(MMIN);
"MAX"                   return(MMAX);
"("                     return(LP);
")"                     return(RP);

%"["                     return(LB);
%"]"                     return(RB);
%"{"                     return(LCB);
%"}"                     return(RCB);

%":="                    return(EQDEF);
%"+"                     return(PLUS);
%"-"                     return(MINUS);
%"*"                     return(TIMES);
%"/"                     return(DIVIDE);
%"mod"                   return(MOD);
%"="                     return(EQUAL);
%"<="                    return(LE);
%">="                    return(GE);
%"<"                     return(LT);
%">"                     return(GT);

"next"                  return(NEXT);
"sigma"                 return(SIGMA);
"self"                  return(SELF);
"union"                 return(UNION);
"in"                    return(SETIN);
"->"                    return(IMPLIES);
"<->"                   return(IFF);
"|"                     return(OR);
"&"                     return(AND);
"!"                     return(NOT);

[A-Za-z_][A-Za-z0-9\|\^\?\/\+\*\$\<\>~@\_#\$%\:\"\'\.-]*  { 
/*                             ExprYylval.node = new_node(ATOM,(node_ptr)find_string(yytext),Nil); */
/*                             return(ATOM); */
                          yylval.id = strdup(yytext);
                          return(ATOM); 
                           }
[0-9]+                  {
/*                          int i;
                          sscanf(yytext,"%d",&i);
                          ExprYylval.node = new_node(NUMBER,(node_ptr)i,Nil); 
*/
                          return(NUMBER);
                        }
","                     return(COMMA);
":"                     return(COLON);
";"                     return(SEMI);
.			fprintf(stderr,"illegal character");

%%






