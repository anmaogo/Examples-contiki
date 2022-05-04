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

 ////////////////////////////
 ////////  LIBRARIES  ///////
 ////////////////////////////

 #include "Arbol_lib.h"

 ////////////////////////////
////////  MEMORIA //////////
////////////////////////////

 MEMB(preferred_parent_mem, struct possible_parent, MAX_PARENTS);
 //MEMB(seleccionar_root_mem, struct seleccionar_root, 10);
 LIST(preferred_parent_list);

 MEMB(retx_list_mem, struct retx_list, 12);
LIST(retx_list_list);

MEMB(routing_table_list_mem, struct routing_table_list, 30);
LIST(routing_table_list_list);

MEMB(one_to_one_msg_table_mem, struct one_to_one_msg_table, 12);
LIST(one_to_one_msg_table_list);

 ////////////////////////////
////////  FUNCION //////////
////////////////////////////


void llenar_beacon(struct beacon *b, linkaddr_t id, uint16_t rssi_c)
{
  b->rssi_c = rssi_c;
  linkaddr_copy( &b->id , &id );
}


void cond_1_neighbor_discovery(linkaddr_t *id_node,struct beacon *beacon_var){
    beacon_var->id.u16 = 0;
    if(id_node->u8[0]==1){
        beacon_var->rssi_c = 0;
    }else{
        beacon_var->rssi_c = 1;
    }
}

//add_parent
void add_parent(struct beacon *beacon_var){
    //Recorre lista en un for

    struct preferred_parent *p;
    struct preferred_parent *in_l;

    //Recorrer toda la lista
    for(p = list_head(preferred_parent_list); p != NULL; p = list_item_next(p)){
        // p coge el valor de cada uno de los y los compara
        if(linkaddr_cmp(&(p->id), &(beacon_var->id))) {
            // Actualizar valor rssi
            p->rssi_c = beacon_var->rssi_c;
            printf("Se actualizo valor rssi a %d del nodo %d\n", beacon_var->rssi_c,p->id.u8[0]);
            break;
        }
    }
  //Si no conocia este posible padre
  if(p == NULL){
    //ADD to the list
    in_l = memb_alloc(&preferred_parent_mem);
    if(in_l == NULL) {            // If we could not allocate a new entry, we give up.
      printf("ERROR: we could not allocate a new entry for <<preferred_parent_list>> in tree_rssi\n");
    }else{
        list_push(preferred_parent_list,in_l); // Add an item to the start of the list.
        //Guardo los campos del mensaje
        linkaddr_copy(&(in_l->id), &(beacon_var->id));  // Guardo el id del nodo
        //rssi_cc es el rssi del padre + el rssi del enlace al padre
        in_l->rssi_c  = beacon_var->rssi_c;
        printf("beacon added to list: id = %d rssi_c = %d\n", in_l->id.u8[0], in_l->rssi_c);
    }
  }
  print_select_table_parent();

 }

 void print_select_table_parent(){
   struct preferred_parent *p;
   int16_t cont =0;
   //Recorrer toda la lista
   for(p = list_head(preferred_parent_list); p != NULL; p = list_item_next(p)){
           cont++;
           printf("En la posicion: %d, esta el nodo %d.%d, con RSII_a de: %d \n",cont,p->id.u8[0],p->id.u8[1],p->rssi_c);

   }
}

//UPDATE parent
void update_parent(struct beacon *beacon_parent){
    struct preferred_parent *p;
    struct beacon new_parent;
    new_parent.id.u16 = 0;
    new_parent.rssi_c = 1;
    for(p = list_head(preferred_parent_list); p != NULL; p = list_item_next(p)){
        if(p->rssi_c != 1){
            if(new_parent.rssi_c == 1 || ((new_parent.rssi_c) < p->rssi_c)){
                new_parent.rssi_c = p->rssi_c;
                linkaddr_copy(&(new_parent.id), &(p->id));
            }
        }
    }
    if(beacon_parent->id.u8[0] != 0){
        printf("#L %d 0\n", beacon_parent->id.u8[0]); // 0: old parent
    }
    linkaddr_copy(&(beacon_parent->id), &(new_parent.id));
    beacon_parent->rssi_c = new_parent.rssi_c;

    printf("#L %d 1\n", beacon_parent->id.u8[0]); // 1: new parent

 }


void set_unicast(struct unicast *unicast_var, linkaddr_t *id,struct send_message *mensaje){
    (unicast_var->data)  = *mensaje;
    linkaddr_copy(&(unicast_var->id), id);
}

void set_message(struct  send_message *message,linkaddr_t  *id, int16_t rssi_a){
    linkaddr_copy(&(message->id), id);
    message->rssi_a = rssi_a;
}

struct routing_table_list *pop_serie(){
        struct routing_table_list *p;
        p =   list_pop(routing_table_list_list);
        return p;
}

struct one_to_one_msg_table *pop_one_to_one_msg_table(){
        struct one_to_one_msg_table *p;
        p =   list_pop(one_to_one_msg_table_list);
        return p;
}

 struct retx_list *pop_rtx(){
     struct retx_list *p;
     p =   list_pop(retx_list_list);
     return p;
 }


void add_one_to_one_msg_table(struct one_to_one_msg *msg){
    struct one_to_one_msg_table  *in_l;
    in_l = memb_alloc(&one_to_one_msg_table_mem);
    if(in_l == NULL) {            // If we could not allocate a new entry, we give up.
      printf("ERROR: we could not allocate a new entry for <<one_to_one_msg_table>> \n");
    }else{
        list_push(one_to_one_msg_table_list,in_l); // Add an item to the start of the list.
        //Guardo los campos del mensaje
        in_l->msg.id_src= msg->id_src;
        in_l->msg.id_dst= msg->id_dst;
    }
}


void  clear_rtx( struct retx_list *ptr){
     memb_free(&retx_list_mem , ptr);
 }

 void clear_serie(struct routing_table_list *ptr){
     memb_free(&routing_table_list_mem , ptr);
 }

 void clear_one_to_one_msg_table(struct one_to_one_msg_table *ptr){
    memb_free(&one_to_one_msg_table_mem , ptr);
}


  void add_rtx(struct unicast * unicast_var){
    //Recorre lista en un for
    
    struct retx_list *p;
    struct retx_list *in_l;

    //Recorrer toda la lista
    for(p = list_head(retx_list_list); p != NULL; p = list_item_next(p)){
        // p coge el valor de cada uno de los y los compara
        if(linkaddr_cmp(&(p->id), &(unicast_var->id))) {
            // Actualizar valor rssi
            p->data.rssi_a = unicast_var->data.rssi_a;
            printf("Se actualizo valor  de rtx a %d dB del nodo %d\n", unicast_var->data.rssi_a,p->id.u8[0]);
            break;
        }
    }
  //Si no conocia este posible padre
  if(p == NULL){
    //ADD to the list
    in_l = memb_alloc(&retx_list_mem);
    if(in_l == NULL) {            // If we could not allocate a new entry, we give up.
      printf("ERROR: we could not allocate a new entry for <<rtx_list_list>> in tree_rssi\n");
    }else{
        list_push(retx_list_list,in_l); // Add an item to the start of the list.
        //Guardo los campos del mensaje
        linkaddr_copy(&(in_l->id), &(unicast_var->id));  // Guardo el id del nodo
        //rssi_ac es el rssi del padre + el rssi del enlace al padre
        in_l->data.rssi_a  = unicast_var->data.rssi_a;
        printf("beacon added to list: id = %d rssi_a = %d\n", in_l->id.u8[0], in_l->data.rssi_a);
    }
  }
  print_rtx_table();

 }

  void init_serie(struct routing_table_msg *serie){
     int i;
     char *dir = serie->serie;
     for(i=0;i<MAX_ROUTING_TABLE;i++){
         *dir = 0;
         dir++;
     }
 }

void imprimir_serie(struct routing_table_msg *serie){
    int i;
     char *dir = serie->serie;
     for(i=0;i<MAX_ROUTING_TABLE;i++){
         printf("%c",*dir);
         dir++;
     }
}

void equal_serie(struct routing_table_msg *src,struct routing_table_msg *dst){
    int i;
    char *src_vec= src->serie;
    char *dst_vec = dst->serie;
    for(i=0;i<MAX_ROUTING_TABLE;i++){
        *dst_vec = *src_vec;
        src_vec++;
        dst_vec++;
    }
}



void serie_list_add(struct routing_table_msg *serie){
    
    struct routing_table_list *in_l;
    in_l = memb_alloc(&routing_table_list_mem);
    if(in_l == NULL) {            // If we could not allocate a new entry, we give up.
      printf("ERROR: we could not allocate a new entry for <<routing_table_list>> \n");
    }else{
        list_push(routing_table_list_list,in_l); // Add an item to the start of the list.
        equal_serie( serie ,&(in_l->serie));
        //Guardo los campos del mensaje
    }
}

 void print_rtx_table(){
    struct retx_list *p;
    int16_t cont =0;
    //Recorrer toda la lista
    for(p = list_head(retx_list_list); p != NULL; p = list_item_next(p)){
            cont++;
            printf("RTX en la posicion: %d, esta el nodo %d.%d, con RSII_a de: %d \n",cont,p->id.u8[0],p->id.u8[1],p->data.rssi_a);

    }
 }

 void generate_dst_random(linkaddr_t  *id_random){
    if(MODE_RANDOM==1){
        id_random->u8[0] = random_rand()%NODOS_TOTALES+1;
        if(id_random->u8[0]== linkaddr_node_addr.u8[0]){
            id_random->u8[0]-=1;
        }
    }else{
        id_random->u8[0] = NODO_DESTINO;
    }
    id_random->u8[1] = 0;
}