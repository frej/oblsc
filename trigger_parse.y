/*
 * Grammar for trigger definitions.
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
 %locations
%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct state *state}
%parse-param {struct trigger_state *trigger_state}
%error-verbose

%{
#include <glib.h>
#include <stdio.h>
#include "state.h"
#include "trigger_type.h"

typedef void* yyscan_t;

%}

%union {
  gdouble time;
  gint samples;
  guint32 value;
  struct trigger_timed_value *timed_value;
  struct trigger_pattern pattern;
  struct trigger *trigger;
  struct signal_def *signal;
  GList *list;
}

/* Bison declarations.  */
%token <value> VALUE
%token <signal> SIGNAL
%token COMMA
%token LPARA
%token RPARA
%token LBRACKET
%token RBRACKET
%token EQUALS
%token LBRACE
%token RBRACE
%token <time> TIME
%token <samples> SAMPLE_CNT
%type <time> time_delay_spec
%type <samples> sample_delay_spec
%type <list> sequential_trigger_list
%type <list> parallel_trigger_list
%type <signal> signal
%type <pattern> pattern_trigger pattern_trigger_list
%type <trigger> pattern_trigger_spec timed_trigger_spec trigger delayed_trigger
%type <timed_value> timed_value timed_values_list timed_values

%{
  int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner);
  void yyerror(YYLTYPE *locp,
	       yyscan_t scanner, struct state *state,
	       struct trigger_state *trigger_state,
	       char const *msg);
%}

%% /* The grammar follows.  */

triggers: sequential_triggers
| parallel_triggers
| delayed_trigger { trigger_activate($1, 0, TRUE); }
        ;

sequential_triggers: LPARA sequential_trigger_list RPARA {
  trigger_activate_sequential_list($2);
}
;

parallel_triggers: LBRACE parallel_trigger_list RBRACE {
  trigger_activate_parallel_list($2);
}
;

sequential_trigger_list: sequential_trigger_list COMMA trigger {
  $$ = g_list_append($1, $3);
}
| delayed_trigger {
  $$ = g_list_append(NULL, $1);
}
;

parallel_trigger_list: parallel_trigger_list COMMA trigger {
  $$ = g_list_append($1, $3);
}
| delayed_trigger {
  $$ = g_list_append(NULL, $1);
}
;

delayed_trigger: trigger time_delay_spec {
  $$ = trigger_add_time_delay(state, trigger_state, $1, $2);
}
| trigger sample_delay_spec {
  $$ = trigger_add_sample_delay(trigger_state, $1, $2);
}
| trigger {
  $$ = trigger_add_sample_delay(trigger_state, $1, 0);
}
;

trigger: timed_trigger_spec    
       | pattern_trigger_spec
       ;

time_delay_spec: TIME {
  $$ = $1;
}
;

sample_delay_spec: SAMPLE_CNT {
  $$ = $1;
}
;

timed_trigger_spec: SIGNAL EQUALS timed_values {
  $$ = trigger_make_timed_trigger(trigger_state, $1, $3);
}
;

timed_values: LBRACKET timed_values_list RBRACKET {
  $$ = $2;
}
;

timed_values_list: timed_values_list COMMA timed_value {
  $3->next = $1; $$ = $3;
}
| timed_value {
  $1->next = NULL; $$ = $1;
}
;

timed_value: VALUE TIME {
  $$ = trigger_make_timed_value(state, $1, $2);
 }
| VALUE SAMPLE_CNT {
  $$ = trigger_make_delayed_value($1, $2);
}
;

pattern_trigger_spec: LBRACKET pattern_trigger_list RBRACKET {
  $$ = trigger_make_pattern_trigger(trigger_state, $2);
};

pattern_trigger_list: pattern_trigger_list COMMA pattern_trigger {
  $$ = trigger_pattern_merge($1, $3);
}
| pattern_trigger
;

pattern_trigger: signal EQUALS VALUE {
  $$ = trigger_pattern_make($1, $3);
};

signal: SIGNAL {
  if ($1 == NULL)
    YYABORT;
  $$ = $1;
 };

%%
void yyerror(YYLTYPE *locp,
	     yyscan_t scanner, struct state *state,
	     struct trigger_state *trigger_state,
	     char const *msg)
{
  fprintf(stderr, "Failed to parse trigger: %s\n", msg);
}
