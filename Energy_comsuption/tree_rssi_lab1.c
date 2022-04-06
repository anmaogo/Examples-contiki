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
#include "lib/list.h"
#include "lib/memb.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "powertrace.h"

#include <stdio.h>
// Librerias propias
#include <tree_rssi_lab1_lib.h>



//LIberia del radio
//#include cc2420.h
/*---------------------------------------------------------------------------*/
// Crear el proceso de seleccion de padre
PROCESS(select_parent, "seleccionar padre");
// Crear proceso enviar un beacon
PROCESS(send_beacon, "Enviar beacon");
// Crear Procesos de Unicast
PROCESS(unicast_msg, "Mensaje de unicast");
AUTOSTART_PROCESSES(&send_beacon,&select_parent,&unicast_msg);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* En MEMB definimos un espacio de memoria en el cual se almacenará la lista  */
MEMB(possible_parent_memb, struct possible_parent, MAX_PARENTS);
// Se define de tipo lista
LIST(possible_parents_list);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* En MEMB definimos un espacio de memoria en el cual se almacenará la lista  */
MEMB(u_retransmit_memb, struct u_retransmit_msg, MAX_UNICAST_MSGS);
// Se define de tipo lista
LIST(u_retransmit_msg_list);
/*---------------------------------------------------------------------------*/


static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

  struct beacon b_recv;
  void *msg= packetbuf_dataptr();

  b_recv=*((struct beacon*)msg);
  signed int b_rec_rssi_c =b_recv.rssi_c;
  linkaddr_t b_rec_id = b_recv.id;

  // atributo para extraer el RSSI packetbuf_attr
  uint16_t rssi_link=packetbuf_attr(PACKETBUF_ATTR_RSSI);
  signed int rssi_total=b_rec_rssi_c+rssi_link;
  // Imprimo lo que recibo y luego lo meto en una lista
  printf("broadcast message received from %d with rssi_c = %d\n",b_rec_id.u8[0],b_rec_rssi_c);

  // Si soy el nodo root no interesa hacer nada porque yo soy la raiz
  if (linkaddr_node_addr.u8[0]!=1) {

  struct possible_parent *p;

  // Guardar los datos en una lista
  /* Recorrer la lista y ver si ya conocemos al padre */

  for(p = list_head(possible_parents_list); p != NULL; p = list_item_next(p)) {

    /* Si se encuentra un padre, se compara el RSSI y se actualiza en caso de que sea diferente */

    if(linkaddr_cmp(&p->id, &b_rec_id )) {
      printf("Encontré al nodo en la lista, tenía rssi %d\n",p->rssi_c);
      // update del RSSI
      p->rssi_c=rssi_total;
      printf("Nuevo rssi %d\n",p->rssi_c);
      break;
    }
  }
  // Si N es NULL quiere decir que Recorrio toda la lista y no encontro a nadie con el mismo ID
  /* If n is NULL, this neighbor was not found in our list, and we
     allocate a new struct neighbor from the neighbors_memb memory
     pool. */
  if(p == NULL) {
    p = memb_alloc(&possible_parent_memb);
    /* If we could not allocate a new neighbor entry, we give up. We
       could have reused an old neighbor entry, but we do not do this
       for now. */
    if(p == NULL) {
      return;
    }

    linkaddr_copy(&p->id, &b_rec_id);// Guardar en p el nuevo id para la entrada
    p->rssi_c =b_rec_rssi_c;
    list_add(possible_parents_list, p);
  }


    printf("Se inicia proceso selección de padre\n");
    process_post(&select_parent,PROCESS_EVENT_CONTINUE,&(possible_parents_list));

  }

}

//Para boradcast
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{

  struct unicast_message msg_recv;
  // Imprimir el Beacon Recibido apunta a donde esta guardado el packet
  void *msg= packetbuf_dataptr();
  // ya tengo el mensaje que debo retransmitir
  msg_recv=*((struct unicast_message*)msg);//

  printf("unicast received payload %s %d.%d\n",
	 msg_recv.msg,msg_recv.id.u8[0], msg_recv.id.u8[1]);

   // Defino el mensaje que se meterá en la lista
   struct u_retransmit_msg *msg_retransmit;
   // si no soy el nodo 1 debo retransmitir
   if(linkaddr_node_addr.u8[0]!=1){
     printf("Rtx\n");
     //Defino que la variable p será un padre posible
     for(msg_retransmit = list_head(u_retransmit_msg_list); msg_retransmit != NULL; msg_retransmit = list_item_next(msg_retransmit)) {
       if(linkaddr_cmp(&msg_retransmit->id, &msg_recv.id )) {
         // Si encontramos mensaje de ese mismo nodo no lo guardamos, quiere decir que
         // aun no lo ha retransmitido
         printf("Este nodo ya envio pero no hemos retransmitido \n");
         break;
       }
     }
     if(msg_retransmit == NULL) {
       msg_retransmit = memb_alloc(&u_retransmit_memb);
       /* If we could not allocate a new neighbor entry, we give up. We
          could have reused an old neighbor entry, but we do not do this
          for now. */
       if(msg_retransmit == NULL) {
         return;
       }

       linkaddr_copy(&msg_retransmit->id, &msg_recv.id);// Guardar en msg_retransmit el nuevo id para la entrada
       msg_retransmit->msg =msg_recv.msg;
       list_add(u_retransmit_msg_list, msg_retransmit);
       printf("Mensaje Agregado a la lista \n");

   }
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: \n",
    dest->u8[0], dest->u8[1]);
}


//Para unicast
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
//Proceso send beacon
PROCESS_THREAD(send_beacon, ev, data)
{
  // Las variables van antes del process PROCESS_BEGIN
  // estas instrucciones corren una vez cuando el proceso inicia
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)


  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  if (linkaddr_node_addr.u8[0]==1) {
    n.rssi_c=0;
  }
  else{
    n.rssi_c = INF_NEG ;
  }

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    fill_beacon_msg(&b,linkaddr_node_addr,n.rssi_c);

    packetbuf_copyfrom(&b, sizeof(struct beacon));
    broadcast_send(&broadcast);
    printf("broadcast message sent with %d\n",n.rssi_c);
  }

  PROCESS_END();

}


/*---------------------------------------------------------------------------*/
//Codigo del proceso select_parent
PROCESS_THREAD(select_parent,ev,data)
{

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  powertrace_start(CLOCK_SECOND * 10); //Para ver el consumo de energia cada 10 segundos

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {
    // este evento corre cuando le llega un evento a este proceso
    PROCESS_YIELD();// cede el procesador hasta que llegue un evento
      if(ev== PROCESS_EVENT_CONTINUE){
      //Recorrer la tabla de rssi totales y escoger el menor

      // Recorrer la lista y ver los valores de RSSI
      struct possible_parent *p;
      struct possible_parent *p_header;
      // Asigno el rssi seleccionado a la cabeza de la lista

      signed int rssi_selected;
      p_header=list_head(possible_parents_list);
      rssi_selected=p_header->rssi_c;


      for(p = list_head(possible_parents_list); p != NULL; p = list_item_next(p)) {

        /* Recorro la lista competa y voy imprimiendo los valores de rssi */
        if(p->rssi_c >= rssi_selected ){
          selected_parent=p;
          rssi_selected=p->rssi_c;
        }

      }

      printf("Padre seleccionado %d\n", selected_parent->id.u8[0] );
      // Actualizar el rssi que voy a empezar a divulgar
      n.rssi_c=selected_parent->rssi_c;
      printf("#L %d 0\n", n.preferred_parent.u8[0]); //Borrar flechita
      linkaddr_copy(&n.preferred_parent,&selected_parent->id);
      printf("#L %d 1\n", n.preferred_parent.u8[0]); //Dibujar flechita


    }

  }

  PROCESS_END();
}

//Proceso para enviar unicast

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_msg, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
    static struct etimer et;


    etimer_set(&et, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 8));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    fill_unicast_msg(&u_msg,linkaddr_node_addr);

    packetbuf_copyfrom(&u_msg, sizeof(struct unicast_message));

    if(linkaddr_node_addr.u8[0]!=1) {
      unicast_send(&uc, &n.preferred_parent);
      // Aqui tambien recorro la lista de mensajes de unicast y conforme envìo elimino
      // Defino el mensaje que se meterá en la lista
      struct unicast_message aux_msg;
      struct u_retransmit_msg *msg_retransmit;


//se recorre la lista de los mensajes de RTX para luego ser enviados y elimados (recordar que hay que liberar memoria)

      for(msg_retransmit = list_head(u_retransmit_msg_list); msg_retransmit != NULL; msg_retransmit = list_item_next(msg_retransmit)) {


        printf("La lista tiene una longitud de : %d \n",list_length(u_retransmit_msg_list));
        aux_msg.id=msg_retransmit->id;
        aux_msg.msg=msg_retransmit->msg;
        printf("Se retrasmite ID: %d \n", msg_retransmit->id.u8[0]);
        fill_unicast_msg(&aux_msg,aux_msg.id);
        packetbuf_copyfrom(&aux_msg, sizeof(struct unicast_message));
        unicast_send(&uc, &n.preferred_parent);


        // Remover de la lista cada que voy enviando
        list_remove(u_retransmit_msg_list,msg_retransmit);
        //Liberar memoria
        memb_free(&u_retransmit_memb,&msg_retransmit);
      }
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
