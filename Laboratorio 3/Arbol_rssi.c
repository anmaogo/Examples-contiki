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



/*---------------------------------------------------------------------------*/
/*
ESPACIO PARA LIBRERIAS PROPIAS
*/

#include "tree_n_ary.h"
#include "Arbol_lib.h"

/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
//VARIABLE GLOBALES
static struct beacon parent_data;
static tree_n_ary *nodo_ary;

/*---------------------------------------------------------------------------*/
PROCESS(send_beacon, "Broadcast example");
PROCESS(unicast_msg, "envia unicast al padre" );
PROCESS(select_parent, "selecciona un padre basandose en mejor RSSI" );
PROCESS(unicast_rtx, "proceso para rtx unicast" );

// Nuevos procesos 

PROCESS(send_routing_table,"Envia tabla de enrutamiento actual al padre");
PROCESS(update_routing_table,"Actualiza tabla de enrutamiento");
PROCESS(generate_one_one_msg,"Envia mensaje 1 a 1");
PROCESS(rtx_one_one,"Reenvia mensaje 1-1");




AUTOSTART_PROCESSES(&send_beacon, &unicast_msg, &select_parent,&unicast_rtx, &send_routing_table, &update_routing_table, &generate_one_one_msg, &rtx_one_one);
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

static void recv_uc(struct unicast_conn *c, const linkaddr_t *from){

  packetbuf_attr_t msg_type = packetbuf_attr(PACKETBUF_ATTR_UNICAST_TYPE);
  if(msg_type == U_DATA){

    struct unicast unicast_rcv = *(struct unicast   *)packetbuf_dataptr();
    if(linkaddr_node_addr.u8[0]!= 1){
        add_rtx(&unicast_rcv);
        process_post(&unicast_rtx, PROCESS_EVENT_CONTINUE,NULL);
    }
    printf("Unicast: Mensaje recibido de %d.%d. Mensaje: trasmisor=%d.%d , rssi_a= %d\n",from->u8[0], from->u8[1],unicast_rcv.id.u8[0],unicast_rcv.id.u8[1] ,unicast_rcv.data.rssi_a );
  
  }else if(msg_type == U_CONTROL){

    struct routing_table_msg serie_rcv = *(struct routing_table_msg   *)packetbuf_dataptr();
    printf("Unicast: Serie recibida de %d.%d:  ",from->u8[0], from->u8[1]);
    imprimir_serie(&serie_rcv);
    printf("\n");

    serie_list_add(&serie_rcv);
    process_post(&update_routing_table, PROCESS_EVENT_CONTINUE,NULL);
  }else if(msg_type == U_ONE_ONE_DATA){

    struct one_to_one_msg one_to_one_rcv = *(struct one_to_one_msg   *)packetbuf_dataptr();
      if(one_to_one_rcv.id_dst.u8[0]==linkaddr_node_addr.u8[0]){
        printf("one_one_msg: Mensaje llego con exito al nodo %d, del nodo %d\n",one_to_one_rcv.id_dst.u8[0],one_to_one_rcv.id_src.u8[0]);
        printf("#A color=orange\n"); 

      }else{

        add_one_to_one_msg_table(&one_to_one_rcv);
        printf("one_one_msg: Se recivio mensaje_ src:%d , dst:%d\n",one_to_one_rcv.id_src.u8[0],one_to_one_rcv.id_dst.u8[0]);
        process_post(&rtx_one_one, PROCESS_EVENT_CONTINUE,NULL);
        printf("#A color=orange\n"); 
        
      }
  }
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
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(unicast_rtx,ev,data)
{

  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  struct retx_list *retx_data;
  struct unicast send_uni;
  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
      PROCESS_WAIT_EVENT();
      retx_data = pop_rtx();
      if(retx_data != NULL){
        printf("Unicast: comienzo a reenviar\n");
        set_unicast( &send_uni ,&(retx_data->id) ,  &(retx_data->data));

        packetbuf_copyfrom(&send_uni, sizeof(struct unicast));

        if(!linkaddr_cmp(&(parent_data.id), &linkaddr_node_addr)) {
          packetbuf_set_attr(PACKETBUF_ATTR_UNICAST_TYPE, U_DATA);
          unicast_send(&uc, &(parent_data.id));
          printf("Unicast: Se reenvio dato RSSI_a= %d al nodo %d.%d,",retx_data->data.rssi_a,parent_data.id.u8[0],parent_data.id.u8[1]);
          printf(" del nodo %d.%d, estando en el nodo %d.%d\n",retx_data->id.u8[0],retx_data->id.u8[1],linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1]);
        }
        

        // Borrar posicion de memoria
        clear_rtx(retx_data);
      }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/

PROCESS_THREAD(unicast_msg, ev, data)
{
  // Cerrar la trasmision cuando se sale del proceso
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  // Abrir canal para trasmicion unicast
  unicast_open(&uc, 146, &unicast_callbacks);
  static struct etimer et;
  struct unicast send_uni;
  struct  send_message message;

  etimer_set(&et,  CLOCK_SECOND * 70);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  printf("Unicast: Unicast inicializado\n");

  while(1) {

    //etimer_set(&et, CLOCK_SECOND+CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    //etimer_set(&et, CLOCK_SECOND);
    //etimer_set(&et, CLOCK_SECOND * 8 - CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    etimer_set(&et, CLOCK_SECOND * 8+ (random_rand() % (CLOCK_SECOND))/2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if(linkaddr_node_addr.u8[0]!= 1 && parent_data.id.u8[0]!= 0){
      printf("Unicast: Comienzo a enviar\n");
      set_message(&message,&linkaddr_node_addr,parent_data.rssi_c);
      set_unicast( &send_uni ,&linkaddr_node_addr ,  &message);

      packetbuf_copyfrom(&(send_uni), sizeof(struct unicast));

      if(!linkaddr_cmp(&(parent_data.id), &linkaddr_node_addr)) {
        packetbuf_set_attr(PACKETBUF_ATTR_UNICAST_TYPE, U_DATA);
        unicast_send(&uc, &(parent_data.id));
        printf("Unicast:Se envio al nodo %d.%d unicast, nodo: %d.%d, rssi_a: %d \n",parent_data.id.u8[0],parent_data.id.u8[1],send_uni.id.u8[0],send_uni.id.u8[1],send_uni.data.rssi_a);
      }
      
   }
  }

  PROCESS_END();
}




/*Proceso send_routing_table*/

PROCESS_THREAD(send_routing_table, ev, data){

  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  static struct routing_table_msg send_table;

  unicast_open(&uc, 146, &unicast_callbacks);
  static struct etimer et;
  etimer_set(&et,  CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  printf("Routing_table: send_routing_table inicializado\n");

  while(1) {
    etimer_set(&et,  CLOCK_SECOND * 2 + CLOCK_SECOND/(4*linkaddr_node_addr.u8[0]));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if(linkaddr_node_addr.u8[0]!= 1 && parent_data.id.u8[0]!= 0){
      printf("Routing_table: control\n");
      init_serie(&send_table);
      serializar(nodo_ary ,send_table.serie);

      packetbuf_copyfrom(&send_table, sizeof(struct routing_table_msg));

        if(!linkaddr_cmp(&(parent_data.id), &linkaddr_node_addr)) {
          packetbuf_set_attr(PACKETBUF_ATTR_UNICAST_TYPE, U_CONTROL);
          unicast_send(&uc, &(parent_data.id));
          printf("Routing_table: Se envia routing table\n");
          print_node_decendents(nodo_ary,0);
          printf("\n");
        }

    }


  }

  PROCESS_END();
}




/*Proceso Update_routing_table*/

PROCESS_THREAD(update_routing_table, ev, data){

  PROCESS_BEGIN();

  while(1) {
      PROCESS_WAIT_EVENT();
      struct routing_table_list *serie_data = pop_serie();
      if(serie_data != NULL){
        print_all_list();
        tree_n_ary *nodo_entrante = deserializacion((serie_data->serie.serie));

        add_child(nodo_ary,nodo_entrante);
        // Borrar posicion de memoria
        clear_serie(serie_data);
        //Llama nuevo dato
        printf("Routing_table: Tree_n_ary actualizado\n");
        print_node_decendents(nodo_ary,0);
        printf("\n");

      }
  }

  PROCESS_END();
}

/*generate_one_one_msg*/

PROCESS_THREAD(generate_one_one_msg, ev, data){

  PROCESS_BEGIN();

  struct one_to_one_msg msg_generate;
  linkaddr_t dst_random;
  static struct etimer et;

  etimer_set(&et,  CLOCK_SECOND * 25);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  printf("one_one_msg: one_one_msg inicializado\n");
  while(1) {
    if( parent_data.id.u8[0]!= 0 && linkaddr_node_addr.u8[0]==NODO_FUENTE){
      printf("#A color=orange\n"); //Habilitar View
      linkaddr_copy(&(msg_generate.id_src), &linkaddr_node_addr);
      generate_dst_random(&dst_random);
      linkaddr_copy(&(msg_generate.id_dst), &dst_random);

      add_one_to_one_msg_table(&msg_generate);
      printf("one_one_msg: Se genero mensaje de nodo %d al nodo %d\n",msg_generate.id_src.u8[0],msg_generate.id_dst.u8[0]);
      process_post(&rtx_one_one, PROCESS_EVENT_CONTINUE,NULL);
    }
    etimer_set(&et,  CLOCK_SECOND * 4);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  }

  PROCESS_END();
}

/*rtx_one_one*/

PROCESS_THREAD(rtx_one_one, ev, data){

  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();
  struct one_to_one_msg_table *retx_data;
  linkaddr_t node_dst;
  int number_node_dst;
  unicast_open(&uc, 146, &unicast_callbacks);


  while(1) {
      PROCESS_WAIT_EVENT();
      retx_data = pop_one_to_one_msg_table();
      if(retx_data != NULL){
        printf("one_one_msg: comienzo a reenviar\n");

        packetbuf_copyfrom(&(retx_data->msg), sizeof(struct one_to_one_msg));

        node_dst.u8[1]=0;
        number_node_dst = search_forwarder(nodo_ary, retx_data->msg.id_dst.u8[0] ,0);

        if(number_node_dst==0){
          node_dst.u8[0]=parent_data.id.u8[0];
        }else{
           node_dst.u8[0]=number_node_dst;
        }

        if(!linkaddr_cmp(&(node_dst), &linkaddr_node_addr)) {
          packetbuf_set_attr(PACKETBUF_ATTR_UNICAST_TYPE, U_ONE_ONE_DATA);
          unicast_send(&uc, &(node_dst));
          printf("one_one_msg: Se envia mensaje del nodo %d al nodo %d, dst:%d,src:%d\n",linkaddr_node_addr.u8[0],node_dst.u8[0],retx_data->msg.id_dst.u8[0],retx_data->msg.id_src.u8[0]);
        }

        // Borrar posicion de memoria
        clear_one_to_one_msg_table(retx_data);
      }
  }

  PROCESS_END();
}




/*---------------------------------------------------------------------------*/
/*FIN DE CODIGO*/
