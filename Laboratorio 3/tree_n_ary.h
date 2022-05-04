/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */


 #ifndef TREE_N_ARY_H_
 #define TREE_N_ARY_H_

 /////////////////////
//////librerias///////
////////////////



#include <stdio.h>
#include "list.h"
#include <stdlib.h>

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"


/////////////////////
//////struct///////
////////////////

typedef struct tree_n_ary tree_n_ary;

struct tree_n_ary{
  struct tree_n_ary *next;
  int nodo;                //Numero nodo id
  struct tree_n_ary *child;     // Direccion hijo
  struct tree_n_ary *sibling;   // Direccion hermano
};


/////////////////////
//////function///////
////////////////

tree_n_ary * add_node(int nodo);
tree_n_ary * add_sibling( tree_n_ary *nodo, tree_n_ary *n_added);
tree_n_ary * add_child(tree_n_ary  *nodo, tree_n_ary *n_added);
void eliminate_branch(tree_n_ary  *nodo, int ini);
void print_node_decendents( tree_n_ary * nodo, int ini);
int search_forwarder(tree_n_ary *n, int id,int ini);
void print_all_list();

void serializar(tree_n_ary * nodo, char * string);
void serializar_V(tree_n_ary * nodo, char **pos, int ini);
void guardar_nodo_str(int nodo,char ** string);

tree_n_ary * deserializacion(char * string);
void deserializacion_V(char ** pos,tree_n_ary *nodo_padre);

 #endif //TRee lib


