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


 #ifndef TREE_LIB_H_
 #define TREE_LIB_H_

/////////////////////
//////librerias///////
////////////////

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>


/////////////////////
//////struct///////
////////////////

struct send_message{
  linkaddr_t id; // Id nodo que lo envia
  int16_t  rssi_a;
};

// Variable para enviar beacons
struct beacon{
    linkaddr_t id;
    int16_t    rssi_a;
};

// Variable para enviar unicast
struct unicast{
    linkaddr_t id;     // Id nodo 
    struct send_message data;
};

// Variable para construir tabla de select_parent
struct preferred_parent
{
  struct preferred_parent *next;
  linkaddr_t id;    //Node id
  int16_t rssi_a;  // rssi acumulado

};


struct retx_list{
  struct retx_list *next;
  linkaddr_t id;    //Node id origen?
  struct send_message data; // data
};



/////////////////////
//////function///////
////////////////
void cond_1_neighbor_discovery(linkaddr_t *id_node,struct beacon *beacon_var);
void set_beacon(struct beacon *beacon_var, linkaddr_t *id, int16_t rssi_a);
void add_parent(struct beacon *beacon_var);
void add_rtx(struct unicast * unicast_var);
void update_parent(struct beacon *beacon_parent);

void print_select_table_parent();
void set_unicast(struct unicast *unicast_var, linkaddr_t *id,struct send_message *mensaje);

void set_message(struct  send_message *message,linkaddr_t  *id, int16_t rssi_a);

struct retx_list *pop_rtx();
void  clear_rtx( struct retx_list *ptr);

 #endif //TRee lib