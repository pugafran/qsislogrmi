#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "filodist.h"

/* variables globales */
// identificador del filósofo
int idfilo;
// número de filósofos en la simulación
int numfilo;
// dir IP o nombre FQDN del siguiente filósofo
// en el anillo lógico
char siguiente_chain[45];
// puerto donde enviar el testigo al siguiente filosofo
unsigned short int puerto_siguiente_chain;
// puerto local en donde deberemos recibir el testigo
unsigned short int puerto_local;
// delay incial antes de conectar con el siguiente
// filosofo en el anillo lógico. Este delay permite
// que el siguiente filósofo haya creado, vinculado(bind)
// y hecho el listen en su socket servidor
int delay;
// estado del filosofo
estado_filosofo estado;
// mutex que protege las modificaciones al valor
// del estado del filosofo
pthread_mutex_t mestado;
// variable condicional que permite suspender al filosofo
// hasta que se produce el cambio de estado efectivo
pthread_cond_t condestado;

/*
  Códigos de error en el exit:
  1: error en la linea de comandos
  2: error en el numero de filosofos
  3: error en la creacion del socket de comunicación con el anterior en el anillo
  4: error en el bind
  7: error en la conexión con el filosofo siguiente
  10: error en el pthread_create
  11: error en el pthread_mutex_lock
  12: error en el pthread_mutex_unlock
  13: error en el pthread_mutex_init
  14: error en el pthread_cond_init
  15: error en el pthread_cond_wait
  16: error en el pthread_join
  17: error en el pthread_cond_signal

*/

/* prototipos funciones*/
void procesaLineaComandos(int numero, char *lista[]);
void inicializaciones(void);
void *filosofo(void);
void esperarPalillos(void);
void soltarPalillos(void);
void cambiarEstado(estado_filosofo nuevoestado);
char palillosLibres(unsigned char token);
void alterarToken(unsigned char *tok, estado_filosofo nuevoestado);
void *comunicaciones(void);

int main(int argc, char *argv[])
{
  int ret;
  // objetos de datos de hilo
  pthread_t h1, h2;

  procesaLineaComandos(argc, argv);
  inicializaciones();
  // lanzamiento del hilo de comunicaciones del filósofo
  ret = pthread_create(&h1, NULL, (void *)comunicaciones, (void *)NULL);
  if (ret != 0)
  {
    fprintf(stderr, "Filosofo %d: Falló al lanzar "
                    "el hilo de comunicaciones\n",
            idfilo);
    exit(10);
  }
  // lanzamiento del hilo principal de funcionamiento del filósofo
  ret = pthread_create(&h2, NULL, (void *)filosofo, (void *)NULL);
  if (ret != 0)
  {
    fprintf(stderr, "Filosofo %d: Falló al lanzar el hilo filosofo\n",
            idfilo);
    exit(10);
  }
  // sincronización con la terminación del hilo de comunicaciones y el
  // hilo que ejecuta la función filósofo
  if (pthread_join(h1, NULL) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Falló al sincronizar con el hilo de comunicaciones\n", idfilo); // APARTADO 0.1
    exit(16); // APARTADO 0.1
  }

  if (pthread_join(h2, NULL) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Falló al sincronizar con el hilo filosofo\n", idfilo); // APARTADO 0.1
    exit(16); // APARTADO 0.1
  }
  return 0;
}

// procesa la linea de comandos, almacena los valores leidos en variables
// globales e imprime los valores leidos
void procesaLineaComandos(int numero, char *lista[])
{
  if (numero != 7)
  {
    fprintf(stderr, "Forma de uso: %s id_filosofo num_filosofos "
                    "ip_siguiente puerto_siguiente "
                    "puerto_local delay_conexion\n",
            lista[0]);
    fprintf(stderr, "Donde id_filosofo es un valor de 0 a n. "
                    "El iniciador del anillo debe ser "
                    "el filosofo con id=0\n");
    exit(1);
  }
  else
  {
    idfilo = atoi(lista[1]);
    numfilo = atoi(lista[2]);
    strcpy(siguiente_chain, lista[3]);
    puerto_siguiente_chain = (unsigned short)atoi(lista[4]);
    puerto_local = (unsigned short)atoi(lista[5]);
    delay = atoi(lista[6]);
    if ((numfilo < 2) || (numfilo > 8))
    {
      fprintf(stderr, "El numero de filosofos debe ser >=2 y <8\n");
      exit(2);
    }
    printf("Filosofo %d Valores leidos:\n", idfilo);
    printf("Filosofo %d   Numero filosofos: %d\n", idfilo, numfilo);
    printf("Filosofo %d   Dir. IP siguiente filosofo: %s\n",
           idfilo, siguiente_chain);
    printf("Filosofo %d   Puerto siguiente filosofo: %d\n",
           idfilo, puerto_siguiente_chain);
    printf("Filosofo %d   Puerto local: %d\n", idfilo, puerto_local);
    printf("Filosofo %d   Delay conexion: %d\n", idfilo, delay);
  }
}
// inicializa el mutex, la variable condicional y el estado del filósofo
void inicializaciones(void)
{

  if (pthread_mutex_init(&mestado, NULL) != 0)
  {
    fprintf(stderr, "Filosofo %d: Error al inicializar el mutex\n", idfilo); // APARTADO 0.1
    exit(13);
  }

  if (pthread_cond_init(&condestado, NULL) != 0)
  {
    fprintf(stderr, "Filosofo %d: Error al inicializar la variable condicional\n", idfilo); // APARTADO 0.1
    exit(14);
  }

  estado = no_sentado;
}
// hilo principal del filosofo
void *filosofo(void)
{
  int numbocados = 0;

  while (numbocados < MAX_BOCADOS)
  {
    fprintf(stderr, "Filosofo %d: cambiando estado a "
                    "queriendo comer\n",
            idfilo);
    cambiarEstado(queriendo_comer);
    esperarPalillos();
    // comiendo
    fprintf(stderr, "Filosofo %d: Comiendo\n", idfilo);
    sleep(5);
    numbocados++;
    cambiarEstado(dejando_comer);
    soltarPalillos();
    fprintf(stderr, "Filosofo %d: Pensando\n", idfilo);
    sleep(10);
  }
  fprintf(stderr, "Filosofo %d: Levantandose de la mesa\n", idfilo);
  // levantandose de la mesa

  return NULL; // APARTADO 0.1
}

// sincronización con el cambio de estado a "comiendo"
void esperarPalillos(void)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(11);                                                              // APARTADO 0.1
  }

  while (estado != comiendo)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0) // APARTADO 0.1
    {
      fprintf(stderr, "Filosofo %d: Error al esperar la variable condicional.\n", idfilo); // APARTADO 0.1
      exit(15);                                                                            // APARTADO 0.1
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(12);                                                                 // APARTADO 0.1
  }
}
// sincronización con el cambio de estado a "pensando"
void soltarPalillos(void)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(11);                                                              // APARTADO 0.1
  }

  while (estado != pensando)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0) // APARTADO 0.1
    {
      fprintf(stderr, "Filosofo %d: Error al esperar la variable condicional.\n", idfilo); // APARTADO 0.1
      exit(15);                                                                            // APARTADO 0.1
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(12);                                                                 // APARTADO 0.1
  }
}
// modificando el estado del filósofo
void cambiarEstado(estado_filosofo nuevoestado)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(11);                                                              // APARTADO 0.1
  }

  estado = nuevoestado;

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    fprintf(stderr, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.1
    exit(12);                                                                 // APARTADO 0.1
  }
}
// comprueba el estado de los palillos necesarios
// para que el filósofo pueda comer
char palillosLibres(unsigned char token)
{
  int pos;
  unsigned char ocupado = 1;
  unsigned char tokenorg = token;

  pos = idfilo;
  // desplazamiento a la derecha
  // se rellena con ceros por la
  // izquierda
  token = token >> pos;
  ocupado &= token;
  if (!ocupado)
  {
    ocupado = 1;
    if (idfilo > 0)
      pos = idfilo - 1;
    else
      pos = numfilo - 1;
    token = tokenorg >> pos;
    ocupado &= token;
  }
  return (!ocupado);
}
// cambia el token reservando o liberando los recursos que el filósofo
// utiliza en función del nuevo estado al que pasa
void alterarToken(unsigned char *tok, estado_filosofo nuevoestado)
{
  int pos;
  unsigned char bit;
  unsigned char tokenaux;

  switch (nuevoestado)
  {
  case comiendo:
    pos = idfilo;
    bit = 1;
    bit = bit << pos;
    *tok |= bit;
    if (idfilo > 0)
      pos = idfilo - 1;
    else
      pos = numfilo - 1;
    bit = 1;
    bit = bit << pos;
    *tok |= bit;
    break;
  case pensando:
    tokenaux = ~*tok;
    pos = idfilo;
    bit = 1;
    bit = bit << pos;
    tokenaux |= bit;
    if (idfilo > 0)
      pos = idfilo - 1;
    else
      pos = numfilo - 1;
    bit = 1;
    bit = bit << pos;
    tokenaux |= bit;
    *tok = ~tokenaux;
    break;
  default:;
  }
}

// hilo de comunicaciones
void *comunicaciones(void)
{
  int ret;

  unsigned char token[2];  // APARTADO 0.1
  token[0] = token[1] = 0; // APARTADO 0.1

  struct sockaddr_in next;
  struct hostent *host_info;
  int sockserver, sockant;
  int socknext;
  struct sockaddr_in servidor, anterior;
  int anterior_len;

  // 1-crear_socket_comunicacion_con_anterior y listen
  sockserver = socket(AF_INET, SOCK_STREAM, 0);
  if (sockserver < 0)
  {
    fprintf(stderr, "Filosofo %d: No se pudo crear "
                    "el socket de comunicación con el anterior "
                    "en el anillo.\n",
            idfilo);
    exit(3);
  }
  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = htonl(INADDR_ANY);
  servidor.sin_port = htons(puerto_local);

  int reuse = 1; // APARTADO 0.1
  if (setsockopt(sockserver, SOL_SOCKET, SO_REUSEADDR,
                 (const char *)&reuse, sizeof(reuse)) < 0) // APARTADO 0.1
    perror("setsockopt(SO_REUSEADDR) failed");             // APARTADO 0.1

#ifdef SO_REUSEPORT // APARTADO 0.1
  if (setsockopt(sockserver, SOL_SOCKET, SO_REUSEPORT,
                 (const char *)&reuse, sizeof(reuse)) < 0) // APARTADO 0.1
    perror("setsockopt(SO_REUSEPORT) failed");             // APARTADO 0.1
#endif                                                     // APARTADO 0.1

  if (bind(sockserver, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  {
    fprintf(stderr, "Filosofo %d: Error vinculando el "
                    "socket de comunicación con el anterior en el anillo.\n",
            idfilo);
    exit(4);
  }
  listen(sockserver, SOMAXCONN);

  // 2-esperar-delay para permitir que el resto de procesos
  // se lancen y lleguen a crear su socket servidor
  sleep(delay);
  // 3-conectar_con_siguiente
  socknext = socket(AF_INET, SOCK_STREAM, 0);
  if (socknext < 0)
  {
    fprintf(stderr, "Filosofo %d: Error creando el"
                    "socket de conexion con el siguiente. \n",
            idfilo);
    exit(5);
  }

  fprintf(stderr, "Filosofo %d: Direccion de conexion "
                  "del siguiente filosofo %s  puerto: %d\n",
          idfilo, siguiente_chain, puerto_siguiente_chain);

  host_info = gethostbyname(siguiente_chain);
  if (host_info == NULL)
  {
    fprintf(stderr, "Filosofo %d: nombre de host desconocido: %s\n",
            idfilo, siguiente_chain);
    exit(3);
  }
  next.sin_family = host_info->h_addrtype;
  memcpy((char *)&next.sin_addr, host_info->h_addr, host_info->h_length);
  next.sin_port = htons(puerto_siguiente_chain);

  if (connect(socknext, (struct sockaddr *)&next, sizeof(next)) < 0)
  {
    fprintf(stderr, "Filosofo %d: Error %d conectando "
                    "con el filosofo siguiente\n",
            idfilo, errno);
    perror("Error conectando\n");
    exit(7);
  }
  // 4-esperamos a que se haya aceptado la conexion del anterior
  anterior_len = sizeof(anterior);
  sockant = accept(sockserver, (struct sockaddr *)&anterior,
                   (socklen_t *)&anterior_len);
  fprintf(stderr, "Filosofo %d: Llega conexion valor %d\n", idfilo, sockant);

  // si llegamos a este punto el ciclo está completo
  // 5-si idfilosofo=0 inyectar token
  if (idfilo == 0)
  {
    write(socknext, token, (size_t)sizeof(unsigned char) * 2);
  }
  //  mientras no fin
  while (1)
  {
    // 6- esperar token
    ret = read(sockant, token, sizeof(unsigned char) * 2);
    if (ret != 2)
    {
      fprintf(stderr, "Filosofo %d: Error de lectura "
                      "en el socket de conexion con el anterior nodo Ret=%d\n",
              idfilo, ret);
    }

    if (pthread_mutex_lock(&mestado) != 0)
    {
      fprintf(stderr, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.1
      exit(11);                                                              // APARTADO 0.1
    }

    if (estado == queriendo_comer)
    {
      //   si no si estado=queriendo_comer
      //    alterar token cuando esten libres y avanzar
      //    cambiar estado a comiendo y señalar la condición
      if (palillosLibres(token[1]))
      {                                    // APARTADO 0.1
        alterarToken(token + 1, comiendo); // APARTADO 0.1
        estado = comiendo;
        if (pthread_cond_signal(&condestado) != 0) // APARTADO 0.1
        {
          fprintf(stderr, "Filosofo %d: Error al señalar la condición en queriendo_comer -> comiendo.\n", idfilo); // APARTADO 0.1
          exit(17);                                                                                                // APARTADO 0.1
        }
      }
    }
    //   si no si estado=dejando_comer
    else if (estado == dejando_comer)
    {
      //    alterar token y avanzar
      //    cambiar estado a pensando y señalar la condicion
      alterarToken(token + 1, pensando); // APARTADO 0.1
      estado = pensando;
      if (pthread_cond_signal(&condestado) != 0) // APARTADO 0.1
      {
        fprintf(stderr, "Filosofo %d: Error al señalar la condición en dejando_comer -> pensando.\n", idfilo); // APARTADO 0.1
        exit(17);                                                                                              // APARTADO 0.1
      }
    }
    if (pthread_mutex_unlock(&mestado) != 0)
    {
      fprintf(stderr, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.1
      exit(12);                                                                 // APARTADO 0.1
    }
    if (ret == 2) // si se leyó bien
    {
      ret = write(socknext, token, sizeof(char) * 2); // APARTADO 0.1
      usleep(1000);                                   // APARTADO 0.1
      if (ret != 2)
      {
        fprintf(stderr, "Error de escritura "
                        "en el socket de conexion con el siguiente nodo\n");
        exit(18); // APARTADO 0.1
      }
    }

    //  fin mientras
  }
}