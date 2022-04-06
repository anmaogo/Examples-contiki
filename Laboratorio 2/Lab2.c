#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Estructura del nodo, se tiene un id, apuntadores al sibling y se child
struct node {
    int id;
    struct node *sibling;
    struct node *child;
};

// Esta corresponde la esructura de la lista, tiene las mismas caracteristicas
// de los nodos pero tiene un apuntador al siguiente elemento de la lista
struct LinkedList{
    int id;
    struct node *sibling;
    struct node *child;
    struct LinkedList *next;
};

//Se define el item de la linked list
typedef struct LinkedList *item; //Define node as pointer of id type struct LinkedList
//Se define el elemento nodo
typedef struct node node;



//Definicion de las funciones que se encuentran despues del main
node * new_node(int);
node * add_sibling(node *, int);
node * add_child(node *, int);
void print_childs(node * n, item list_backtrace, item list_visited);
item createitem();
void print_list(item head);
item tail(item head);
void remove_last_item(item head);
bool is_node_in_list(item head,node * n);
item add_node_list(item list_pointer, node *n);
node * from_item_to_node(item list_node);
int search_forwarder(node * n, item list_backtrace,item list_visited, int id_node);
void serialize(node * n, item list_backtrace,item list_visited,char cadena_serializada[]);
void deserialize(node *n,char cadena_serializada[],item list_backtrace);

int main(int argc, char *argv[])
{


  node *root=new_node(1);

  node *dos=add_child(root,2);
  node *tres=add_child(root,3);
  node *cuatro=add_child(root,4);

  node *cinco=add_child(dos,5);
  node *seis=add_child(dos,6);
  node *siete=add_child(dos,7);
  node *ocho=add_child(dos,8);

  node *catorce=add_child(siete,14);
  node *quince=add_child(siete,15);
  node *dieciseis=add_child(siete,16);


  node *ventiuno=add_child(dieciseis,21);
  node *ventidos=add_child(dieciseis,22);

  node *nueve=add_child(tres,9);
  node *diez=add_child(tres,10);
  node *once=add_child(tres,11);

  node *diecisiete=add_child(once,17);
  node *dieciocho=add_child(once,18);

  node *ventitres=add_child(diecisiete,23);

  node *doce=add_child(cuatro,12);
  node *trece=add_child(cuatro,13);

  node *diecinueve = add_child(doce,19);
  node *veinte = add_child(doce,20);

  node *venticuatro = add_child(veinte,24);

  node *venticinco = add_child(venticuatro,25);

  // Se define el primer elemento de cada una de las listas
  item list_backtrace=NULL;
  item list_visited=NULL;
  //Función para imprimir la tabla de enrutamiento del nodo en cuestion
  //print_childs(root,list_backtrace,list_visited);
  // Funcion que retorna a que nodo se debe enviar el msg de unicast
  char cadena_example[100];
  serialize(root,list_backtrace,list_visited,cadena_example);
  printf("El arbol es: %s\n", cadena_example);


  node *papa = new_node(0);
  deserialize(root,cadena_example,list_backtrace);

  int caso1 = search_forwarder(root,list_backtrace,list_visited,22);
  int caso2 = search_forwarder(root,list_backtrace,list_visited,23);
  int caso3 = search_forwarder(root,list_backtrace,list_visited,24);
  int caso4 = search_forwarder(root,list_backtrace,list_visited,25);
  int caso5 = search_forwarder(root,list_backtrace,list_visited,18);
  int caso6 = search_forwarder(root,list_backtrace,list_visited,13);
  int caso7 = search_forwarder(root,list_backtrace,list_visited,30);

  printf("El que debe reenviar en caso 1 es:  %i\n",caso1);
  printf("El que debe reenviar en caso 2 es:  %i\n",caso2);
  printf("El que debe reenviar en caso 3 es:  %i\n",caso3);
  printf("El que debe reenviar en caso 4 es:  %i\n",caso4);
  printf("El que debe reenviar en caso 5 es:  %i\n",caso5);
  printf("El que debe reenviar en caso 6 es:  %i\n",caso6);
  printf("El que debe reenviar en caso 7 es:  %i\n",caso7);

}



node * new_node(int id)
{
    node *new_node = malloc(sizeof(node));
    if ( new_node ) {
        new_node->sibling = NULL;
        new_node->child = NULL;
        new_node->id = id;
    }
    return new_node;
}

node * add_sibling(node * n, int id)
{
    if ( n == NULL )
        return NULL;

    while (n->sibling)
        n = n->sibling;


    return (n->sibling = new_node(id));
}

node * add_child(node * n, int data)
{
    if ( n == NULL )
        return NULL;

    if ( n->child )
        return add_sibling(n->child, data);
    else
        return (n->child = new_node(data));
}

int search_forwarder(node * n, item list_backtrace,item list_visited, int id_node){
  node *current_node=n;
  node *next_node;

  if(current_node->id == id_node){

    if(list_backtrace==NULL){
      printf("Yo soy el nodo de destino\n");
      return 0;
    }

    else{

      if(list_backtrace->next == NULL){
        //printf("Lo debe enviar el %i \n", list_backtrace->id);
        return list_backtrace->id;
      }
      else{
        //printf("lo debe enviar %i \n", list_backtrace->next->id);
        return list_backtrace->next->id;
      }

    }

  }


  if(list_visited!=NULL){
    if(list_visited->id==current_node->id){
      //printf("estamos en el final\n");
      return 0;
    }
  }

  // Verificar si el nodo tiene hijo ¿Tiene hijo?
  if(current_node -> child !=NULL){
    // Agregar el current node a la lista
    //printf("el nodo %i si tiene hijo\n",current_node -> id);
    //Verificar si el hijo ya aparece en la lista de visitados
    bool found_in_visited= is_node_in_list(list_visited,current_node->child);

    if(!found_in_visited){
      //printf("El nodo %i no ha sido visitado\n",current_node->child->id );
      //Guardar el current en backtrace por que acá tendremos que volver
      list_backtrace=add_node_list(list_backtrace, current_node);
      //Definir que el siguiente nodo al que iremos será al hijo
      next_node=current_node->child;
    }
    else{
      //printf("Este nodo ya fue visitado evaluar los hermanos \n" );
      if(current_node -> sibling !=NULL)
        next_node=current_node->sibling;
      else{
        //printf("Devolvernos al nodo %i\n",tail(list_backtrace)->id );
        next_node=from_item_to_node(tail(list_backtrace));
        remove_last_item(list_backtrace);
      }

    }

  }
  else if (current_node -> sibling !=NULL){
    //printf("el nodo %i si tiene hermanos\n",current_node->id);
    bool found_in_visited= is_node_in_list(list_visited,current_node->sibling);
    // Si no encuentra el nodo hermano
    if(!found_in_visited)
        next_node=current_node->sibling;
  }
  else{
    //printf("el nodo %i no tiene ni hijos ni hermanos\n",current_node->id);
    //Aca debemos devolvernos en el backtrace
    // entonces le miramos la cola al backtrace
    //printf("Devolvernos al nodo %i\n",tail(list_backtrace)->id );
    next_node=from_item_to_node(tail(list_backtrace));
    //borramos la cola de la lista
    remove_last_item(list_backtrace);

  }
  // Este nodo ya fue guardado como visitado?

  bool guardado =is_node_in_list(list_visited,current_node);
  // si no ha sido guardado pues guardarlo
  if (!guardado)
    list_visited=add_node_list(list_visited, current_node);

  //printf("El backtrace es: \n" );
  //print_list(list_backtrace);

  //printf("Los visitados son : \n" );
  //print_list(list_visited);

  search_forwarder(next_node,list_backtrace,list_visited,id_node);

}

// Esta funcion imprime los hijos de un nodo en especifico
void print_childs(node * n, item list_backtrace,item list_visited){

  node *current_node=n;
  node *next_node;
  //printf("Estoy en el nodo %d\n",current_node->id );


  if(list_visited!=NULL){
    if(list_visited->id==current_node->id){
      printf("La tabla de enrutamiento del nodo %i es : \n", current_node->id);
      print_list(list_visited);
      // Descomentar esto si el profesor pide que muestre hermanos del nodo a imprimir

      // while(current_node->sibling !=NULL){
      //   printf("los datos son %i \n",current_node->sibling->id);
      //   current_node =current_node->sibling;
      // }


      return;
    }
  }

  // Verificar si el nodo tiene hijo
  if(current_node -> child !=NULL){
    //printf("entra en child!DE NULL\n");
    // Agregar el current node a la lista
    //printf("el nodo %i si tiene hijo\n",current_node -> id);
    //Verificar si el hijo ya aparece en la lista de visitados
    bool found_in_visited= is_node_in_list(list_visited,current_node->child);

    if(!found_in_visited){
      //printf("El nodo %i no ha sido visitado\n",current_node->child->id );
      //Guardar el current en backtrace por que acá tendremos que volver

      list_backtrace=add_node_list(list_backtrace, current_node);
      //Definir que el siguiente nodo al que iremos será al hijo
      next_node=current_node->child;
    }
    else{
      //printf("Este nodo ya fue visitado evaluar los hermanos \n" );
      if(current_node -> sibling !=NULL)
        next_node=current_node->sibling;
      else{
        //printf("Devolvernos al nodo %i\n",tail(list_backtrace)->id );
        next_node=from_item_to_node(tail(list_backtrace));
        remove_last_item(list_backtrace);
      }

    }

  }
  else if (current_node -> sibling !=NULL){
    //printf("entra en SIBLING !DE NULL\n");
    //printf("el nodo %i si tiene hermanos\n",current_node->id);
    bool found_in_visited= is_node_in_list(list_visited,current_node->sibling);
    // Si no encuentra el nodo hermano
    if(!found_in_visited)
        next_node=current_node->sibling;
  }
  else{
    //printf("el nodo %i no tiene ni hijos ni hermanos\n",current_node->id);
    //Aca debemos devolvernos en el backtrace
    // entonces le miramos la cola al backtrace
    //printf("Devolvernos al nodo %i\n",tail(list_backtrace)->id );
    //printf("entra en NINGUNO DE LOS 2\n");
    next_node=from_item_to_node(tail(list_backtrace));
    //printf("ahora ire al nodo acaa %d\n",next_node->id );
    //borramos la cola de la lista
    remove_last_item(list_backtrace);

  }
  // Este nodo ya fue guardado como visitado?

  bool guardado =is_node_in_list(list_visited,current_node);
  // si no ha sido guardado pues guardarlo
  if (!guardado)
    list_visited=add_node_list(list_visited, current_node);

  //printf("El backtrace es: \n" );
  //print_list(list_backtrace);

  //printf("Los visitados son : \n" );
  //print_list(list_visited);
  //printf("ahora ire al nodo %d\n",next_node->id );
  print_childs(next_node,list_backtrace,list_visited);

}

//Crear un item
item createitem(){
     item temp; // declare a item
     temp = (item)malloc(sizeof(struct LinkedList)); // allocate memory using malloc()
     temp->next = NULL;// make next point to NULL
     return temp;//return the new item
 }

item add_node_list(item list_pointer, node *n){

  item temp,p;// declare two items temp and p
  temp = createitem();//createitem will return a new item with id = value and next pointing to NULL.
  temp->id = n->id; // add element's value to id part of item
  temp->sibling = n->sibling; // add element's value to id part of item
  temp->child = n->child; // add element's value to id part of item
  if(list_pointer == NULL){

      list_pointer = temp;     //when linked list is empty
  }
  else{

      p  = list_pointer;//assign head to p
      while(p->next != NULL){
          p = p->next;//traverse the list until p is the last item.The last item always points to NULL.
      }
      p->next = temp;//Point the previous last item to the new item created.
  }
  return list_pointer;
}
//Imprimir la lista
void print_list(item head) {
    item current = head;
    while (current != NULL) {
        printf("los datos son %d\n", current->id);
        current = current->next;
    }
}

// Funcion que retorna la cola de la lista
item tail(item head){
 // si existe algo en la lista recorrala, si no pues no
 if(head==NULL)
  return NULL;
  // como la lista no esta vacia si ejecuta aca entonces recorrerla hasta que sea null
  item l;
  for(l = head; l->next != NULL; l = l->next);
  return l;
}

//Funcion que reueve el ultimo elemento de la lista
void remove_last_item(item head){
    item current;
    current = head;
    if(current==NULL)
      return;

    if (current->next == NULL)
      return;

   while (current->next->next != NULL)
    current = current->next;

  // aqui queda el current que será la cola
  current->next =NULL;
  free(current->next);

}

bool is_node_in_list(item head,node * n){
  item current = head;
  bool found=false;

  while (current != NULL) {
      if(current->id == n->id){
        found=true;
      }
      current = current->next;
  }
  return found;
}

// pasar de un item a un node
//Aqui hay un problema
node * from_item_to_node(item list_node){

  node *n = malloc(sizeof(node));
  n->id = list_node->id; // add element's value to id part of item
  n->sibling = list_node->sibling; // add element's value to id part of item
  n->child = list_node->child; // add element's value to id part of item
  return n;
}

// Esta funcion debe devolver una cadena de caracteres con el arbol serializado
void serialize(node * n, item list_backtrace,item list_visited,char cadena_serializada[]){

  node *current_node=n;
  node *next_node;
  bool n_backs=false;

  //printf("Estoy en el nodo %d\n",current_node->id );


  if(list_visited!=NULL){
    if(list_visited->id==current_node->id){
      //printf("La tabla de enrutamiento del nodo %i es : \n", current_node->id);
      //print_list(list_visited);
      //printf("%s\n", cadena_serializada);
      cadena_serializada[strlen(cadena_serializada)-1] = '\0';
      return;
    }
  }

  // Verificar si el nodo tiene hijo
  if(current_node -> child !=NULL){
    //printf("entra en child!DE NULL\n");
    // Agregar el current node a la lista
    //printf("el nodo %i si tiene hijo\n",current_node -> id);
    //Verificar si el hijo ya aparece en la lista de visitados
    bool found_in_visited= is_node_in_list(list_visited,current_node->child);

    if(!found_in_visited){
      //printf("El nodo %i no ha sido visitado\n",current_node->child->id );
      //Guardar el current en backtrace por que acá tendremos que volver

      list_backtrace=add_node_list(list_backtrace, current_node);
      //Definir que el siguiente nodo al que iremos será al hijo
      next_node=current_node->child;
    }
    else{
      //printf("Este nodo ya fue visitado evaluar los hermanos \n" );
      if(current_node -> sibling !=NULL){
        next_node=current_node->sibling;
        n_backs=true;
        //strcat(cadena_serializada, ")");
      }

      else{
        //printf("Devolvernos al nodo %i\n",tail(list_backtrace)->id );
        next_node=from_item_to_node(tail(list_backtrace));
        n_backs=true;
        //strcat(cadena_serializada, ")");
        remove_last_item(list_backtrace);
      }

    }

  }
  else if (current_node -> sibling !=NULL){
    //printf("entra en SIBLING !DE NULL\n");
    //printf("el nodo %i si tiene hermanos\n",current_node->id);
    bool found_in_visited= is_node_in_list(list_visited,current_node->sibling);
    // Si no encuentra el nodo hermano
    n_backs=true;
    //strcat(cadena_serializada, "s)");
    if(!found_in_visited)
      next_node=current_node->sibling;



  }
  else{

    next_node=from_item_to_node(tail(list_backtrace));
    n_backs=true;
    //strcat(cadena_serializada, ")");
    remove_last_item(list_backtrace);

  }
  // Este nodo ya fue guardado como visitado?

  bool guardado =is_node_in_list(list_visited,current_node);
  // si no ha sido guardado pues guardarlo
  if (!guardado){
    list_visited=add_node_list(list_visited, current_node);
    char sid[5];
    sprintf(sid, "%d", current_node->id);
    strcat(cadena_serializada, sid);
    strcat(cadena_serializada, ",");
  }
  if(n_backs)
  {
    strcat(cadena_serializada, ")");
    strcat(cadena_serializada, ",");
  }


  serialize(next_node,list_backtrace,list_visited,cadena_serializada);
}

//Entra una cadena y construye el arbol
void deserialize(node * n,char cadena_serializada[],item list_backtrace){
  // Extract the first token
  node *current_node=n;

  char * token = strtok(cadena_serializada, ",");

  while( token != NULL ) {
    if (strcmp(token,")") == 0){
      current_node=from_item_to_node(tail(list_backtrace));
      //printf("Volviendo a %d\n",current_node->id );
      remove_last_item(list_backtrace);
    }
    else{

      // Agregamos un nuevo hijo
      int token_int;
      sscanf(token, "%d",&token_int);
      printf("Agregando como hijo de %d a %d\n",current_node->id,token_int );
      add_child(current_node,token_int);
      list_backtrace=add_node_list(list_backtrace, current_node);
      //Irnos al hijo en cuestion
      if(current_node->child->id==token_int){
          //si a donde ahora tendre que guardar es el primer hijo perfecto
          current_node=current_node->child;
      }
      else{
        //Si donde ahora tengo que guardar no es el primer hijo, debo navegar entre los hijos
        //hasta el hijo donde debo guardar
        current_node=current_node->child;

        while(current_node->id !=token_int){
          current_node= current_node->sibling;

        }

      }

    }

    token = strtok(NULL, ",");

   }


}
