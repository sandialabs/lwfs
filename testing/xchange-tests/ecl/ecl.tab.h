/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ARROW = 258,
     LPAREN = 259,
     RPAREN = 260,
     LCURLY = 261,
     RCURLY = 262,
     LBRACKET = 263,
     RBRACKET = 264,
     DOT = 265,
     STAR = 266,
     AT = 267,
     SLASH = 268,
     MODULUS = 269,
     PLUS = 270,
     MINUS = 271,
     LEQ = 272,
     LT = 273,
     GEQ = 274,
     GT = 275,
     EQ = 276,
     NEQ = 277,
     LEFT_SHIFT = 278,
     RIGHT_SHIFT = 279,
     ASSIGN = 280,
     LOG_OR = 281,
     LOG_AND = 282,
     ARITH_OR = 283,
     ARITH_AND = 284,
     ARITH_XOR = 285,
     INC_OP = 286,
     DEC_OP = 287,
     BANG = 288,
     SEMI = 289,
     IF = 290,
     ELSE = 291,
     FOR = 292,
     WHILE = 293,
     CHAR = 294,
     SHORT = 295,
     INT = 296,
     LONG = 297,
     UNSIGNED = 298,
     SIGNED = 299,
     FLOAT = 300,
     DOUBLE = 301,
     VOID = 302,
     STRING = 303,
     STATIC = 304,
     STRUCT = 305,
     UNION = 306,
     CONST = 307,
     SIZEOF = 308,
     TYPEDEF = 309,
     RETURN_TOKEN = 310,
     PRINT = 311,
     COMMA = 312,
     DOTDOTDOT = 313,
     integer_constant = 314,
     string_constant = 315,
     floating_constant = 316,
     identifier_ref = 317,
     type_id = 318
   };
#endif
/* Tokens.  */
#define ARROW 258
#define LPAREN 259
#define RPAREN 260
#define LCURLY 261
#define RCURLY 262
#define LBRACKET 263
#define RBRACKET 264
#define DOT 265
#define STAR 266
#define AT 267
#define SLASH 268
#define MODULUS 269
#define PLUS 270
#define MINUS 271
#define LEQ 272
#define LT 273
#define GEQ 274
#define GT 275
#define EQ 276
#define NEQ 277
#define LEFT_SHIFT 278
#define RIGHT_SHIFT 279
#define ASSIGN 280
#define LOG_OR 281
#define LOG_AND 282
#define ARITH_OR 283
#define ARITH_AND 284
#define ARITH_XOR 285
#define INC_OP 286
#define DEC_OP 287
#define BANG 288
#define SEMI 289
#define IF 290
#define ELSE 291
#define FOR 292
#define WHILE 293
#define CHAR 294
#define SHORT 295
#define INT 296
#define LONG 297
#define UNSIGNED 298
#define SIGNED 299
#define FLOAT 300
#define DOUBLE 301
#define VOID 302
#define STRING 303
#define STATIC 304
#define STRUCT 305
#define UNION 306
#define CONST 307
#define SIZEOF 308
#define TYPEDEF 309
#define RETURN_TOKEN 310
#define PRINT 311
#define COMMA 312
#define DOTDOTDOT 313
#define integer_constant 314
#define string_constant 315
#define floating_constant 316
#define identifier_ref 317
#define type_id 318




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 155 "ecl.y"
typedef union YYSTYPE {
    lx_info info;
    sm_ref reference;
    sm_list list;
    char *string;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 171 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



