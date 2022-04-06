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

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>

#include "Arbol_lib.h"
/*---------------------------------------------------------------------------*/
/*
ESPACIO PARA MEMORIAS
*/


//TABLA PARA RETRANSMITIR MENSAJES
/*---------------------------------------------------------------------------*/
/* En MEMB definimos un espacio de memoria en el cual se almacenará la lista  */
MEMB(u_retransmit_memb, struct u_retransmit_msg, MAX_UNICAST_MSGS);
// Se define de tipo lista
LIST(u_retransmit_msg_list);
/*---------------------------------------------------------------------------*/
//VARIABLE GLOBALES
static struct beacon parent_data;

/*---------------------------------------------------------------------------*/
PROCESS(send_beacon, "Broadcast example");
PROCESS(unicast_msg, "envia unicast al padre" );
PROCESS(select_parent, "selecciona un padre basandose en mejor RSSI" );
PROCESS(unicast_rtx, "proceso para rtx unicast" );
AUTOSTART_PROCESSES(&send_beacon, &unicast_msg, &select_parent,&unicast_rtx);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

  if(linkaddr_node_addr.u8[0]!= 1){
        // leer beacon recibido
        struct beacon beacon_rcv = *(struct beacon   *)packetbuf_dataptr();
        // Evaluar si es diferente de uno, lo que indica que ya tiene ruta hacia AC
        printf("Rssi Nodo anterior: %d. ",beacon_rcv.rssi_c);
        if(beacon_rcv.rssi_c != 1){
          //sumar el RSSI del enlace
          beacon_rcv.rssi_c += packetbuf_attr(PACKETBUF_ATTR_RSSI);
        }

        printf("Mensaje Beacon recibido del nodo %d.%d: ID = %d, RSSI_ACUMULADO = %d.\n",
                from->u8[0], from->u8[1], beacon_rcv.id.u8[0], beacon_rcv.rssi_c);


        add_parent(&beacon_rcv);

        process_post(&select_parent, PROCESS_EVENT_CONTINUE,NULL);
    }


}


static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_beacon, ev, data)
{
  // Las variables van antes del process PROCESS_BEGIN
  // estas instrucciones corren una vez cuando el proceso inicia
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)


  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  static int16_t flag = 0;
  if(flag==0){
    flag=1;
    // Inicializar padre como 0 e inicializar valor de RSSI_a
    cond_1_neighbor_discovery(&linkaddr_node_addr,&parent_data);

  }


  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    llenar_beacon(&b,linkaddr_node_addr,parent_data.rssi_c);

    packetbuf_copyfrom(&b, sizeof(struct beacon));
    broadcast_send(&broadcast);
    printf("broadcast message sent with %d\n",parent_data.rssi_c);
  }

  PROCESS_END();

}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(select_parent, ev, data){

  PROCESS_BEGIN();

  while(1) {
      PROCESS_WAIT_EVENT();
      if(linkaddr_node_addr.u8[0]!= 1){
        update_parent(&parent_data);
        printf("El parent es el nodo: %d.%d con un RSSI de:%d dB\n",parent_data.id.u8[0],parent_data.id.u8[1],parent_data.rssi_c);
      }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/


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
       process_post(&unicast_rtx,PROCESS_EVENT_CONTINUE,&(u_retransmit_msg_list));

   }
  }
}
/*---------------------------------------------------------------------------*/
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

PROCESS_THREAD(unicast_rtx,ev,data)
{

  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
    // este evento corre cuando le llega un evento a este proceso
    PROCESS_YIELD();// cede el procesador hasta que llegue un evento
      if(ev== PROCESS_EVENT_CONTINUE){



        if(linkaddr_node_addr.u8[0]!=1){
          printf("Aqui inicio el proceso de RTX\n");
          struct unicast_message aux_msg;
          struct u_retransmit_msg *msg_retransmit;
          for(msg_retransmit = list_head(u_retransmit_msg_list); msg_retransmit != NULL; msg_retransmit = list_item_next(msg_retransmit)) {


            printf("La lista tiene una longitud de : %d \n",list_length(u_retransmit_msg_list));
            aux_msg.id=msg_retransmit->id;
            aux_msg.msg=msg_retransmit->msg;
            printf("Se retrasmite ID: %d \n", msg_retransmit->id.u8[0]);
            llenar_unicast(&aux_msg,aux_msg.id);
            packetbuf_copyfrom(&aux_msg, sizeof(struct unicast_message));
            unicast_send(&uc, &parent_data.id);


            // Remover de la lista cada que voy enviando
            list_remove(u_retransmit_msg_list,msg_retransmit);
            //Liberar memoria
            memb_free(&u_retransmit_memb,&msg_retransmit);
          }


        }



    }//Ciere de IF EV

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


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


    if(linkaddr_node_addr.u8[0]!=1) {
      llenar_unicast(&u_msg,linkaddr_node_addr);
      packetbuf_copyfrom(&u_msg, sizeof(struct unicast_message));
      unicast_send(&uc, &parent_data.id);
    }

  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/*FIN DE CODIGO*/
