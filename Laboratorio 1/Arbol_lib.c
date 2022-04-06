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

 ////////////////////////////
////////  FUNCION //////////
////////////////////////////


void llenar_beacon(struct beacon *b, linkaddr_t id, uint16_t rssi_c)
{
  b->rssi_c = rssi_c;
  linkaddr_copy( &b->id , &id );
}


// llenar el mensaje de unicast con la informacion
void llenar_unicast(struct unicast_message *unicast_msg,linkaddr_t id){
  char str_aux[20];
  strcpy(str_aux,"Yo soy el nodo: ");

  unicast_msg->msg="Yo soy el nodo: ";
  //Link Address copy copia del segundo argumento en el primer once
  linkaddr_copy(&unicast_msg->id,&id);

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
