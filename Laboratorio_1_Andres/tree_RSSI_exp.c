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

// contiki necesita makefile

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
#include "tree_lib.h"

#include "net/netstack.h"
//#define REMOTE 1


/*------------------------Definicion de procesos-----------------------------*/
PROCESS(neighbor_discovery, "Descubrir vecinos");
PROCESS(parent_select, "Selecconar padre");
PROCESS(send_unicast, "Enviar el  unicast a padre");
PROCESS(retx_unicast, "Retrasmitir informacion de nodo hijo a nodo padre");
PROCESS(ini_unicast,"En este proceso se iniciara el unicast");

//AUTOSTART_PROCESSES(&neighbor_discovery,&parent_select);
AUTOSTART_PROCESSES(&neighbor_discovery,&parent_select,&ini_unicast);
/*---------------------------variables globales------------------------------*/
// Variable el cual va a contener el ID del padre, y el RSSI_a hacia el
static struct beacon parent_data;

/*-------------------------Definiciones broadcast ----------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    if(linkaddr_node_addr.u8[0]!= 1){
        // leer beacon recibido
        struct beacon beacon_rcv = *(struct beacon   *)packetbuf_dataptr();
        // Evaluar si es diferente de uno, lo que indica que ya tiene ruta hacia AC
        printf("Rssi Nodo anterior: %d. ",beacon_rcv.rssi_a);
        if(beacon_rcv.rssi_a != 1){
          //sumar el RSSI del enlace
          beacon_rcv.rssi_a += packetbuf_attr(PACKETBUF_ATTR_RSSI);
        }

        printf("Mensaje Beacon recibido del nodo %d.%d: ID = %d, RSSI_ACUMULADO = %d.\n",
                from->u8[0], from->u8[1], beacon_rcv.id.u8[0], beacon_rcv.rssi_a);


        add_parent(&beacon_rcv);

        process_post(&parent_select, PROCESS_EVENT_CONTINUE,NULL);
    }

}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*-------------------------Definiciones unicast ----------------------------*/

static void recv_uc(struct unicast_conn *c, const linkaddr_t *from){
  struct unicast unicast_rcv = *(struct unicast   *)packetbuf_dataptr();

    if(linkaddr_node_addr.u8[0]!= 1){
        add_rtx(&unicast_rcv);
        process_post(&retx_unicast, PROCESS_EVENT_CONTINUE,NULL);
    }
    printf("Unicast: Mensaje recibido de %d.%d. Mensaje: trasmisor=%d.%d , rssi_a= %d\n",from->u8[0], from->u8[1],unicast_rcv.id.u8[0],unicast_rcv.id.u8[1] ,unicast_rcv.data.rssi_a );
}


static void sent_uc(struct unicast_conn *c, int status, int num_tx) {
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("Unicast:  mensaje  enviado to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}


static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;

/*--------------------Descubrir vecinos------------------------------------
    Funciones:
            - Enviar beacon: Enviar broadcast con RSSI y Nodo cada 2 segundos
            - Nodo 1: Nodo 1 tiene RSSI de 0, y si envia broadcast

*/
PROCESS_THREAD(neighbor_discovery, ev, data){



  static int16_t flag = 0;
  if(flag ==0){
    flag = 1;
    // Inicializar padre como 0 e inicializar valor de RSSI_a
    cond_1_neighbor_discovery(&linkaddr_node_addr,&parent_data);
  }

  /*

  #if REMOTE
    static radio_value_t val;

    if(NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, MY_TX_POWER_DBM ) == RADIO_RESULT_OK)
    {
        NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &val);
        printf("Transmission Power Set : %d dBm\n", val);
    }
    else if(NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, MY_TX_POWER_DBM) == RADIO_RESULT_INVALID_VALUE)
    {
    printf("ERROR: RADIO_RESULT_INVALID_VALUE\n");
    }else{
    printf("ERROR: The TX power could not be set\n");
    }
    #endif
*/
  static struct etimer et; // Definicion timer
  PROCESS_EXITHANDLER(broadcast_close(&broadcast););

  PROCESS_BEGIN();

  struct beacon send_beacon;
  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {
    /* Delay 2 seconds */
    etimer_set(&et, CLOCK_SECOND * 2 - CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    //etimer_set(&et, CLOCK_SECOND * 2+ (random_rand() % (CLOCK_SECOND))/2);
    //etimer_set(&et, CLOCK_SECOND * 2);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    // Cargar datos a enviar en variable beaon
    printf("broadcast RSSI_a es: %d\n",parent_data.rssi_a);
    set_beacon( &send_beacon ,&linkaddr_node_addr ,  parent_data.rssi_a);
    // Cargar dato a enviar
    packetbuf_copyfrom(&send_beacon, sizeof(struct beacon));
    // enviar broadcast
    broadcast_send(&broadcast);
    printf("Beacon enviado, RSSI acumulado de: %d \n", parent_data.rssi_a);

  }

  PROCESS_END();
}
/*--------------------Selecconar padre------------------------------------
    Funciones: - Se activa cada vez que llega un mensaje de broadcast
               - Se revisa la tabla de padres candidatos, y se cambia
               la variable
               - El nodo 1 no ejecuta esta funcion


*/

PROCESS_THREAD(parent_select, ev, data){

  PROCESS_BEGIN();

  while(1) {
      PROCESS_WAIT_EVENT();
      if(linkaddr_node_addr.u8[0]!= 1){
        update_parent(&parent_data);
        printf("El parent es el nodo: %d.%d con un RSSI de:%d dB\n",parent_data.id.u8[0],parent_data.id.u8[1],parent_data.rssi_a);
      }
  }

  PROCESS_END();
}
/*------------------Enviar el  unicast a padre----------------------------
    Funciones: - Envia unicast al padre
               - Si es el nodo 1 no lo hace
               - Cada x segundos
               - Solo va a enviar una vez que se haya conectado a un padre
*/

PROCESS_THREAD(send_unicast, ev, data){
  // Cerrar la trasmision cuando se sale del proceso
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  // Abrir canal para trasmicion unicast
  unicast_open(&uc, 146, &unicast_callbacks);
  static struct etimer et;
  struct unicast send_uni;
  struct  send_message message;
  while(1) {

    //etimer_set(&et, CLOCK_SECOND+CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    //etimer_set(&et, CLOCK_SECOND);
    //etimer_set(&et, CLOCK_SECOND * 8 - CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    etimer_set(&et, CLOCK_SECOND * 8+ (random_rand() % (CLOCK_SECOND))/2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(linkaddr_node_addr.u8[0]!= 1 && parent_data.id.u8[0]!= 0){
      printf("Unicast: Comienzo a enviar\n");
      set_message(&message,&linkaddr_node_addr,parent_data.rssi_a);
      set_unicast( &send_uni ,&linkaddr_node_addr ,  &message);

      packetbuf_copyfrom(&(send_uni), sizeof(struct unicast));

      if(!linkaddr_cmp(&(parent_data.id), &linkaddr_node_addr)) {
        unicast_send(&uc, &(parent_data.id));
        printf("Unicast:Se envio al nodo %d.%d unicast, nodo: %d.%d, rssi_a: %d \n",parent_data.id.u8[0],parent_data.id.u8[1],send_uni.id.u8[0],send_uni.id.u8[1],send_uni.data.rssi_a);
      }

   }
  }

  PROCESS_END();
}
/*---------Retrasmitir informacion de nodo hijo a nodo padre---------------
    Funciones: - Se llama con el unicast_recv
               - No funciona con 1
               - Usa unicast_retx_list
               -
*/

PROCESS_THREAD(retx_unicast, ev, data){

  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  struct retx_list *retx_data;
  struct unicast send_uni;
  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
      PROCESS_WAIT_EVENT();
      retx_data = pop_rtx();
      while(retx_data != NULL){
        printf("Unicast: comienzo a reenviar\n");
        set_unicast( &send_uni ,&(retx_data->id) ,  &(retx_data->data));

        packetbuf_copyfrom(&send_uni, sizeof(struct unicast));

        if(!linkaddr_cmp(&(parent_data.id), &linkaddr_node_addr)) {
          unicast_send(&uc, &(parent_data.id));
          printf("Unicast: Se reenvio dato RSSI_a= %d al nodo %d.%d,",retx_data->data.rssi_a,parent_data.id.u8[0],parent_data.id.u8[1]);
          printf(" del nodo %d.%d, estando en el nodo %d.%d\n",retx_data->id.u8[0],retx_data->id.u8[1],linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1]);
        }


        // Borrar posicion de memoria
        clear_rtx(retx_data);
        //Llama nuevo dato
        retx_data = pop_rtx();

      }
  }

  PROCESS_END();
}
/*--------------------Inicializar unicast ------------------------------------
    Funciones: - inicializar unicast 1 segundo despues de inicio del procesador
*/
PROCESS_THREAD(ini_unicast, ev, data){
  static struct etimer et;
  PROCESS_BEGIN();
  etimer_set(&et, (2*linkaddr_node_addr.u8[0]+1)*CLOCK_SECOND);
  //etimer_set(&et, CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  if(linkaddr_node_addr.u8[0]!=1){
    printf("Unicast: ini\n");
    process_start(&retx_unicast,NULL);
  }
  process_start(&send_unicast,NULL);
  PROCESS_END();
}
