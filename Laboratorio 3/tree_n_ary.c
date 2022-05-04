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



 #include "tree_n_ary.h"


MEMB(tree_n_ary_mem, tree_n_ary , 50);
LIST(tree_n_ary_list);


tree_n_ary * add_node(int nodo){
    tree_n_ary *in_l;
    in_l = memb_alloc(&tree_n_ary_mem);
    if(in_l == NULL) {            // If we could not allocate a new entry, we give up.
      printf("ERROR: we could not allocate a new entry for <<tree_mem>> in tree_rssi\n");
    }else{
        list_push(tree_n_ary_list,in_l); // Add an item to the start of the list.
        in_l->nodo = nodo;
        in_l->child = NULL;
        in_l->sibling = NULL;
    }
    return in_l;
}

tree_n_ary * add_sibling(tree_n_ary *nodo,  tree_n_ary *n_added){

    tree_n_ary *p = nodo;
    tree_n_ary * nodo_anterior = NULL;

    while( p->sibling != NULL){
        if(p->nodo == n_added->nodo){
            nodo_anterior->sibling =  n_added;
            n_added->sibling = p->sibling;
            // borrar p
            eliminate_branch(p,0);
            return n_added;
        }
        nodo_anterior = p;
        p = p->sibling;
    } 
    if(p->nodo == n_added->nodo){
            nodo_anterior->sibling =  n_added;
            n_added->sibling = p->sibling;
            // borrar p
            eliminate_branch(p,0);
            return n_added;
    }
    p->sibling = n_added;
    return n_added;
}

tree_n_ary * add_child(tree_n_ary *nodo, tree_n_ary *n_added){
    if(nodo->child != NULL){
        if(nodo->child->nodo == n_added->nodo){

            n_added->sibling = nodo->child->sibling;
            // Borrar direcion nodo->child
            eliminate_branch(nodo->child,0);
            nodo->child = n_added;
        }else{
            add_sibling( nodo->child, n_added);
        }
    }else{
        nodo->child = n_added;
    }
    return n_added;
}


void print_node_decendents(tree_n_ary * nodo, int ini){
    printf("%d ",nodo->nodo);
    if(nodo->sibling != NULL && ini != 0){
        printf("-->");
        print_node_decendents(nodo->sibling,1);
    }
    if(nodo->child != NULL){
        printf("\nRouting_table: %d:",nodo->nodo);
        print_node_decendents(nodo->child,1);
    }

}

int search_forwarder(tree_n_ary *nodo, int id,int ini){
    int ans_hermanos;
    int ans_hijos;
    if(nodo->nodo != id){
        if(nodo->sibling != NULL && ini!= 0){
            ans_hermanos = search_forwarder(nodo->sibling,id,1);
            if(ans_hermanos >0){
                return ans_hermanos;
            }
        }
        if(nodo->child != NULL){
            ans_hijos = search_forwarder(nodo->child,id,1);
            if(ans_hijos >0){
                if(ini != 0){
                    return nodo->nodo;
                }else{
                    return ans_hijos;
                }
            }
        }else{
            return 0;
        }
    }else{
        return nodo->nodo;
    }
    return 0;

}

void print_all_list(){
    printf("\nLista completa:\n");
    tree_n_ary *p;
    for(p = list_head(tree_n_ary_list); p != NULL; p = list_item_next(p)){
            printf("Nodo:%d \n",p->nodo);
    }
}

void eliminate_branch(tree_n_ary  *nodo, int ini){

    if(nodo->sibling != NULL && ini != 0){
        eliminate_branch(nodo->sibling,1);
    }
    if(nodo->child != NULL){
        eliminate_branch(nodo->child,1);
    }
    list_remove(tree_n_ary_list, nodo);
    memb_free(&tree_n_ary_mem , nodo);
}


void serializar(tree_n_ary * nodo, char * string){
    char *var= string;
    char **pos = &var;
    serializar_V(nodo ,pos,0);
}

void serializar_V(tree_n_ary * nodo,char **pos, int ini){
    guardar_nodo_str(nodo->nodo,pos);

    if(nodo->child != NULL){
        serializar_V(nodo->child, pos, 1);
    }
    *(*pos) = 41;
    *pos+=1;
    if(nodo->sibling != NULL && ini != 0){
        serializar_V(nodo->sibling, pos, 1);
    }
}

void guardar_nodo_str(int nodo,char ** string){
    if(nodo>=10){
        **string= 48+(nodo/10);
        *(*(string)+1)= 48+(nodo%10);
    }else{
        **string= 48;
        *(*(string)+1)= 48+(nodo);
    }
    *string = *(string)+2;
}

tree_n_ary * deserializacion(char * string){
    tree_n_ary * nodo_parent = add_node((*string-48)*10+(*(string+1)-48));

    char *var= string+2;
    char **pos = &var;
    deserializacion_V(pos,nodo_parent);
    return nodo_parent;
}

void deserializacion_V(char ** pos,tree_n_ary *nodo_parent){
    while(**pos!=41 ){
        tree_n_ary * nodo_child = add_node(((*(*pos))-48)*10+(*((*pos)+1)-48));
        add_child(nodo_parent,nodo_child);
        (*pos)=(*pos)+2;
        deserializacion_V(pos,nodo_child);
    }
    (*pos)++;
}