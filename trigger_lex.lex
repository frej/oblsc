/*
 * Lexer for trigger definitions.
 *
 * This file is part of oblsc.
 *
 * Copyright (C) 2010-2011 Frej Drejhammar <frej.drejhammar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
%option 8bit bison-bridge bison-locations reentrant
%option warn nodefault
%option outfile="trigger_lex.c" header-file="trigger_lex.h"
%option nounput noyywrap noinput
%option extra-type="struct state *"
%{
#include <glib.h>
#include "state.h"
#include "trigger_type.h"
#include "trigger_parse.h"
%}

DIGIT        [0-9]
HEX_DIGIT    [0-9a-fA-F]
OCTAL_DIGIT  [0-7]
NAME         [a-z][a-z0-9]*
WHITESPACE   [ \t\n]

%%

"," { return COMMA; }
"[" { return LBRACKET; }
"]" { return RBRACKET; }
"(" { return LPARA; }
")" { return RPARA; }
"{" { return LBRACE; }
"}" { return RBRACE; }
"=" { return EQUALS; }

"/"{DIGIT}+("."{DIGIT}*)?(""|"s"|"ms"|"us"|"ns"|"ps") {
  gchar *tail;
  
  yylval->time = strtod(yytext+1, &tail);

  if (tail[0] == 's')
    ; /* The value does need to be scaled */
  else if (tail[0] == 'm')
    yylval->time *= 1e-3;
  else if (tail[0] == 'u')
    yylval->time *= 1e-6;
  else if (tail[0] == 'n')
    yylval->time *= 1e-9;
  else if (tail[0] == 'p')
    yylval->time *= 1e-12;
  else { /* This is raw samples, convert to time */
    sscanf(yytext + 1, "%d", &yylval->samples);
    return SAMPLE_CNT;
  }

  return TIME;
}

"0x"{HEX_DIGIT}+ {
  sscanf(yytext, "%x", &yylval->value);
  return VALUE;
}

"0"{OCTAL_DIGIT}+ {
  sscanf(yytext, "%o", &yylval->value);
  return VALUE;
}

{DIGIT}+ {
  sscanf(yytext, "%u", &yylval->value);
  return VALUE;
}

{NAME}  {
  yylval->signal = state_lookup_signal(yyextra, yytext);

  if (yylval->signal == NULL)
    fprintf(stderr, "Error: Signal %s is not defined\n", yytext);
  return SIGNAL;
}

{WHITESPACE}+ /* Ignore */
. /* Ignore */

%%
