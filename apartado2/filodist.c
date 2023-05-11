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
#include <sys/time.h>

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

char msg[100]; // APARTADO 0.2

/* prototipos funciones*/
void procesaLineaComandos(int numero, char *lista[]);
void inicializaciones(void);
void *filosofo(void);
void esperarPalillos(void);
void soltarPalillos(void);
void esperarCuchara(void);
void soltarCuchara(void);
void cambiarEstado(estado_filosofo nuevoestado);
char palillosLibres(unsigned char token);
int cucharaLibre(unsigned char token);
void alterarToken(unsigned char *tok, estado_filosofo nuevoestado);
void *comunicaciones(void);
void *esperarConexion(void);
void printlog(char *msg);                                         // APARTADO 0.2
void print_token(unsigned char token[2], estado_filosofo estado); // APARTADO 0.2
char *estado2str(int estado);                                     // APARTADO 0.2
void soltarSilla(void);                                           // APARTADO 2

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
    sprintf(msg, "Filosofo %d: Falló al lanzar el hilo de comunicaciones\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                    // APARTADO 0.2

    exit(10);
  }
  // lanzamiento del hilo principal de funcionamiento del filósofo
  ret = pthread_create(&h2, NULL, (void *)filosofo, (void *)NULL);
  if (ret != 0)
  {
    sprintf(msg, "Filosofo %d: Falló al lanzar el hilo filosofo\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                           // APARTADO 0.2

    exit(10);
  }
  // sincronización con la terminación del hilo de comunicaciones y el
  // hilo que ejecuta la función filósofo
  pthread_join(h1, NULL);
  pthread_join(h2, NULL);
  return 0;
}

// procesa la linea de comandos, almacena los valores leidos en variables
// globales e imprime los valores leidos
void procesaLineaComandos(int numero, char *lista[])
{
  if (numero != 7)
  {
    sprintf(msg, "Forma de uso: %s id_filosofo num_filosofos ip_siguiente puerto_siguiente puerto_local delay_conexion\n", lista[0]); // APARTADO 0.2
    printlog(msg);                                                                                                                    // APARTADO 0.2

    sprintf(msg, "Donde id_filosofo es un valor de 0 a n. El iniciador del anillo debe ser el filosofo con id=0\n"); // APARTADO 0.2
    printlog(msg);                                                                                                   // APARTADO 0.2

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
      sprintf(msg, "El numero de filosofos debe ser >=2 y <8\n"); // APARTADO 0.2
      printlog(msg);                                              // APARTADO 0.2

      exit(2);
    }

    sprintf(msg, "Filosofo %d Valores leidos:\n", idfilo); // APARTADO 0.2
    printlog(msg);                                         // APARTADO 0.2

    sprintf(msg, "Filosofo %d   Numero filosofos: %d\n", idfilo, numfilo); // APARTADO 0.2
    printlog(msg);                                                         // APARTADO 0.2

    sprintf(msg, "Filosofo %d   Dir. IP siguiente filosofo: %s\n", idfilo, siguiente_chain); // APARTADO 0.2
    printlog(msg);                                                                           // APARTADO 0.2

    sprintf(msg, "Filosofo %d   Puerto siguiente filosofo: %d\n", idfilo, puerto_siguiente_chain); // APARTADO 0.2
    printlog(msg);                                                                                 // APARTADO 0.2

    sprintf(msg, "Filosofo %d   Puerto local: %d\n", idfilo, puerto_local); // APARTADO 0.2
    printlog(msg);                                                          // APARTADO 0.2

    sprintf(msg, "Filosofo %d   Delay conexion: %d\n", idfilo, delay); // APARTADO 0.2
    printlog(msg);                                                     // APARTADO 0.2
  }
}
// inicializa el mutex, la variable condicional y el estado del filósofo
void inicializaciones(void)
{
  pthread_mutex_init(&mestado, NULL);
  pthread_cond_init(&condestado, NULL);
  estado = no_sentado;
}
// hilo principal del filosofo
void *filosofo(void)
{
  int numbocados = 0;

  while (numbocados < MAX_BOCADOS)
  {

    sprintf(msg, "Filosofo %d: cambiando estado a queriendo condimentar\n", idfilo); // APARTADO 1
    printlog(msg);                                                                   // APARTADO 1
    cambiarEstado(queriendo_condimentar);                                            // APARTADO 1
    esperarCuchara();                                                                // APARTADO 1

    sprintf(msg, "Filosofo %d: Condimentando\n", idfilo); // APARTADO 1
    printlog(msg);                                        // APARTADO 1
    sleep(5);                                             // APARTADO 1
    cambiarEstado(dejando_condimentar);                   // APARTADO 1
    soltarCuchara();

    sprintf(msg, "Filosofo %d: Hablando\n", idfilo); // APARTADO 1
    printlog(msg);                                   // APARTADO 1
    sleep(10);                                       // APARTADO 1

    sprintf(msg, "Filosofo %d: cambiando estado a queriendo comer\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                             // APARTADO 0.2
    cambiarEstado(queriendo_comer);
    esperarPalillos();
    // comiendo
    sprintf(msg, "Filosofo %d: Comiendo\n", idfilo); // APARTADO 0.2
    printlog(msg);                                   // APARTADO 0.2
    sleep(5);
    numbocados++;
    cambiarEstado(dejando_comer);
    soltarPalillos();
    sprintf(msg, "Filosofo %d: Pensando\n", idfilo); // APARTADO 0.2
    printlog(msg);                                   // APARTADO 0.2
    sleep(10);
  }

  cambiarEstado(levantado);                                       // APARTADO 2                                               
  sprintf(msg, "Filosofo %d: Levantandose de la mesa\n", idfilo); // APARTADO 0.2
  printlog(msg);  
  sleep(10);                                                // APARTADO 0.2
                        
  sprintf(msg, "Filosofo %d: Esperando para irse\n", idfilo); // APARTADO 2
  printlog(msg);                                              // APARTADO 2
}

void printlog(char *msg) // APARTADO 0.2
{
  // Imprime en stderr el mensaje recibido, precedido de un timestamp entero con precisión de milisegundos
  struct timeval tv;
  gettimeofday(&tv, NULL);
  fprintf(stderr, "%ld.%06ld %s", tv.tv_sec, tv.tv_usec, msg);
}

void print_token(unsigned char token[2], estado_filosofo estado)
{ // APARTADO 0.2
  // El tipo estado_filosofo es un enum declarado en filodist.h
  char buff[2][9];
  unsigned char tok;
  char msg[100];

  for (int byte = 0; byte < 2; byte++)
  { // Para cada byte del token
    tok = token[byte];
    for (int i = 7; i >= 0; i--)
    { // Sacamos sus bits
      buff[byte][i] = (tok & 1) ? '1' : '0';
      tok = tok >> 1;
    }
    buff[byte][8] = 0; // Añadir terminador de cadena
  }
  sprintf(msg, "Filosofo %d: (estado=%s) transmite token = %s|%s\n", idfilo, estado2str(estado), buff[0], buff[1]);
  printlog(msg);
}

char *estado2str(int estado)
{ // APARTADO 0.2 // APARTADO 1
  switch (estado)
  {
  case 0:
    return "no_sentado";
  case 1:
    return "queriendo_comer";
  case 2:
    return "comiendo";
  case 3:
    return "dejando_comer";
  case 4:
    return "pensando";
  case 5:
    return "queriendo_condimentar";
  case 6:
    return "condimentando";
  case 7:
    return "dejando_condimentar";
  case 8:
    return "hablando";
  case 9:
    return "esperando_irse";
  case 10:
    return "levantado";
  default:
    return "desconocido";
  }
}

// sincronización con el cambio de estado a "comiendo"
void esperarPalillos(void)
{
  pthread_mutex_lock(&mestado);
  while (estado != comiendo)
    pthread_cond_wait(&condestado, &mestado);
  pthread_mutex_unlock(&mestado);
}
// sincronización con el cambio de estado a "pensando"
void soltarPalillos(void)
{
  pthread_mutex_lock(&mestado);
  while (estado != pensando)
    pthread_cond_wait(&condestado, &mestado);
  pthread_mutex_unlock(&mestado);
}

// sincronización con el cambio de estado a "levantado"
void soltarSilla(void)
{
  pthread_mutex_lock(&mestado);
  while (estado != levantado)
    pthread_cond_wait(&condestado, &mestado);
  pthread_mutex_unlock(&mestado);
}

// sincronización con el cambio de estado a "condimentando"
void esperarCuchara(void) // APARTADO 1
{
  pthread_mutex_lock(&mestado);
  while (estado != condimentando)
    pthread_cond_wait(&condestado, &mestado);
  pthread_mutex_unlock(&mestado);
}

// sincronización con el cambio de estado a "hablando"
void soltarCuchara(void) // APARTADO 1
{
  pthread_mutex_lock(&mestado);
  while (estado != hablando)
    pthread_cond_wait(&condestado, &mestado);
  pthread_mutex_unlock(&mestado);
}

// modificando el estado del filósofo
void cambiarEstado(estado_filosofo nuevoestado)
{
  pthread_mutex_lock(&mestado);
  estado = nuevoestado;
  pthread_mutex_unlock(&mestado);
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

int cucharaLibre(unsigned char token) // APARTADO 1
{

  // 0xC0=11000000 en binario y 0x80=10000000 en binario y 0x40=01000000 en binario
  if ((token & 0xC0) == 0x40 || (token & 0xC0) == 0)
    return 1;
  else if ((token & 0xC0) == 0x80)
    return 2;

  return 0;
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
  case condimentando: // APARTADO 1
    if (cucharaLibre(tok[0]) == 1)
      *tok |= 0x80; // 10000000
    else if (cucharaLibre(tok[0]) == 2)
      *tok |= 0x40; // 01000000
    break;
  case hablando: // APARTADO 1
    if (cucharaLibre(tok[0]) == 1 || cucharaLibre(tok[0]) == 0)
      *tok &= 0xBF;
    else if (cucharaLibre(tok[0]) == 2)
      *tok &= 0x7F;
    break;
  case levantado:              // APARTADO 2
    if ((*tok & 0x07) != 0x07) // si no estan todos levantados (111) levanta al siguiente
      (*tok)++;                // incrementa el token en 1 para levantar al siguiente
    break;
  default:;
  case esperando_irse: // APARTADO 2
    if (idfilo == 0 && ((*tok & 0x07) == 0x07))
    {
      *tok |= 0xFF; // 11111111
      exit(0);
    }

    if ((*tok & 0xFF) == 0xFF) // si estan todos levantados (11111111) se van los demás
      exit(0);
    break;
  }
}

// hilo de comunicaciones
void *comunicaciones(void)
{
  int ret;

  unsigned char token[2];  // APARTADO 0.1
  token[0] = token[1] = 0; // APARTADO 0.1

  unsigned char old_token[2];      // APARTADO 0.2
  old_token[0] = old_token[1] = 0; // APARTADO 0.2

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
    sprintf(msg, "Filosofo %d: No se pudo crear el socket de comunicación con el anterior en el anillo.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                                                   // APARTADO 0.2
    exit(3);
  }
  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = htonl(INADDR_ANY);
  servidor.sin_port = htons(puerto_local);

  int reuse = 1;
  if (setsockopt(sockserver, SOL_SOCKET, SO_REUSEADDR,
                 (const char *)&reuse, sizeof(reuse)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

#ifdef SO_REUSEPORT
  if (setsockopt(sockserver, SOL_SOCKET, SO_REUSEPORT,
                 (const char *)&reuse, sizeof(reuse)) < 0)
    perror("setsockopt(SO_REUSEPORT) failed");
#endif

  if (bind(sockserver, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
  {
    sprintf(msg, "Filosofo %d: Error vinculando el socket de comunicación con el anterior en el anillo.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                                                   // APARTADO 0.2

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

    sprintf(msg, "Filosofo %d: Error creando el socket de conexion con el siguiente. \n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                                 // APARTADO 0.2

    exit(5);
  }

  sprintf(msg, "Filosofo %d: Direccion de conexion del siguiente filosofo %s  puerto: %d\n", idfilo, siguiente_chain, puerto_siguiente_chain); // APARTADO 0.2
  printlog(msg);                                                                                                                               // APARTADO 0.2

  host_info = gethostbyname(siguiente_chain);
  if (host_info == NULL)
  {
    sprintf(msg, "Filosofo %d: nombre de host desconocido: %s\n", idfilo, siguiente_chain); // APARTADO 0.2
    printlog(msg);                                                                          // APARTADO 0.2

    exit(3);
  }

  next.sin_family = host_info->h_addrtype;
  memcpy((char *)&next.sin_addr, host_info->h_addr, host_info->h_length);
  next.sin_port = htons(puerto_siguiente_chain);

  if (connect(socknext, (struct sockaddr *)&next, sizeof(next)) < 0)
  {
    sprintf(msg, "Filosofo %d: Error %d conectando con el filosofo siguiente\n", idfilo, errno); // APARTADO 0.2
    printlog(msg);                                                                               // APARTADO 0.2

    perror("Error conectando\n");
    exit(7);
  }
  // 4-esperamos a que se haya aceptado la conexion del anterior
  anterior_len = sizeof(anterior);
  sockant = accept(sockserver, (struct sockaddr *)&anterior,
                   (socklen_t *)&anterior_len);

  sprintf(msg, "Filosofo %d: Llega conexion valor %d\n", idfilo, sockant); // APARTADO 0.2
  printlog(msg);                                                           // APARTADO 0.2
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

    memcpy(old_token, token, sizeof(unsigned char) * 2); // APARTADO 0.2

    if (ret != 2)
    {
      sprintf(msg, "Filosofo %d: Error de lectura en el socket de conexion con el anterior nodo Ret=%d\n", idfilo, ret); // APARTADO 0.2
      printlog(msg);                                                                                                     // APARTADO 0.2
    }

    pthread_mutex_lock(&mestado);        // APARTADO 1
    if (estado == queriendo_condimentar) // APARTADO 1
    {

      if (cucharaLibre(token[0]) != 0)
      {                                         // APARTADO 1
        alterarToken(&token[0], condimentando); // APARTADO 1
        estado = condimentando;                 // APARTADO 1
        pthread_cond_signal(&condestado);       // APARTADO 1
      }
    }

    else if (estado == dejando_condimentar) // APARTADO 1
    {

      alterarToken(&token[0], hablando); // APARTADO 1
      estado = hablando;                 // APARTADO 1
      pthread_cond_signal(&condestado);  // APARTADO 1
    }
    pthread_mutex_unlock(&mestado); // APARTADO 1

    pthread_mutex_lock(&mestado);
    if (estado == queriendo_comer)
    {
      //   si no si estado=queriendo_comer
      //    alterar token cuando esten libres y avanzar
      //    cambiar estado a comiendo y señalar la condición
      if (palillosLibres(token[1]))
      {                                    // APARTADO 0.1
        alterarToken(token + 1, comiendo); // APARTADO 0.1
        estado = comiendo;
        pthread_cond_signal(&condestado);
      }
    }
    //   si no si estado=dejando_comer
    else if (estado == dejando_comer)
    {
      //    alterar token y avanzar
      //    cambiar estado a pensando y señalar la condicion
      alterarToken(token + 1, pensando); // APARTADO 0.1
      estado = pensando;
      pthread_cond_signal(&condestado);
    }
    pthread_mutex_unlock(&mestado);

    pthread_mutex_lock(&mestado); // APARTADO 2
    if (estado == levantado)
    {                                     // APARTADO 2
      alterarToken(&token[0], levantado); // APARTADO 2
      estado = esperando_irse;            // APARTADO 2
    }

    else if (estado == esperando_irse)
    {
      alterarToken(&token[0], esperando_irse); // APARTADO 2
    }

    pthread_mutex_unlock(&mestado);

    if (memcmp(old_token, token, sizeof(unsigned char) * 2) != 0) // APARTADO 0.2
    {
      print_token(token, estado); // APARTADO 0.2
    }
    // print_token(token, estado);
    if (ret == 2) // si se leyó bien
    {

      ret = write(socknext, token, sizeof(char) * 2); // APARTADO 0.1
      usleep(1000);                                   // APARTADO 0.1
      if (ret != 2)
      {
        sprintf(msg, "Error de escritura en el socket de conexion con el siguiente nodo\n"); // APARTADO 0.2
        printlog(msg);                                                                       // APARTADO 0.2
      }
    }

    //  fin mientras
  }
}