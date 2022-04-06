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

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */


#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>


#define INF_NEG -999
// El maximo en la tabla de padres candidatos podemos considerar un remove o un update
#define MAX_PARENTS 30
// El maximo de Unicast messages para envío
#define MAX_UNICAST_MSGS 30

//
/* This is the structure of unicast ping messages. */
struct unicast_message {
  char* msg;
  linkaddr_t id;
};
////////////////////////////////////
// Estructura del mensaje de beacon qu voy a enviar
struct beacon{
  linkaddr_t id;
  signed int rssi_c;
};
// Estructura nodo que se va a divulgar
struct node{
  linkaddr_t preferred_parent;// El padre del nodo
  signed int rssi_c;
};
// Estructura posibles padres para la lista
/* This structure holds information about neighbors. */
struct possible_parent {

  struct possible_parent *next;
  linkaddr_t id;
  signed int rssi_c;

};

// Estructura de mensajes para retransmision */
struct u_retransmit_msg {

  struct u_retransmit_msg *next;
  char* msg;
  linkaddr_t id;

};
// Asignacion de estructura
struct beacon b;
struct node n;
struct possible_parent *selected_parent;

struct unicast_message u_msg;
//static struct unicast_conn msg_unicast;
// llenar el beacon con la informacion
void fill_beacon_msg (struct beacon *b,linkaddr_t id,signed int rssi_c);
void fill_unicast_msg(struct unicast_message *unicast_msg,linkaddr_t id);
