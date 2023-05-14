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
#include <ctype.h>

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
void esperarCuchara(void); // APARTADO 1
void soltarCuchara(void);  // APARTADO 1
void cambiarEstado(estado_filosofo nuevoestado);
char palillosLibres(unsigned char token);
int cucharaLibre(unsigned char token); // APARTADO 1
void alterarToken(unsigned char *tok, estado_filosofo nuevoestado);
void *comunicaciones(void);
void *esperarConexion(void);
void printlog(char *msg);                                         // APARTADO 0.2
void print_token(unsigned char token[2], estado_filosofo estado); // APARTADO 0.2
char *estado2str(int estado);                                     // APARTADO 0.2

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
  if (pthread_join(h1, NULL) != 0)
  {
    sprintf(msg, "Filosofo %d: Falló al sincronizar con el hilo de comunicaciones\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                             // APARTADO 0.2

    exit(16); // APARTADO 0.1
  }

  if (pthread_join(h2, NULL) != 0)
  {
    sprintf(msg, "Filosofo %d: Falló al sincronizar con el hilo filosofo\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                    // APARTADO 0.2

    exit(16); // APARTADO 0.1
  }
  return 0;
}

int esEntero(char *cadena) // APARTADO 0.1
{
  while (*cadena != '\0')
  {
    if (!isdigit((unsigned char)*cadena))
    {
      return 0;
    }
    cadena++;
  }

  return 1;
}

int esIP(const char *ip)
{ // APARTADO 0.1
  // Crea una copia de la cadena para no modificar la original
  char *copia = strdup(ip);

  int segmentos = 0;                   // Contador de segmentos
  char *segmento = strtok(copia, "."); // Divide la cadena en segmentos

  while (segmento)
  {
    segmentos++;

    // Verifica que el segmento es un número
    for (int i = 0; segmento[i] != '\0'; i++)
    {
      if (segmento[i] < '0' || segmento[i] > '9')
      {
        free(copia);
        return 0; // No es un número
      }
    }

    // Convierte el segmento a un número y verifica que esté en el rango correcto
    int num = atoi(segmento);
    if (num < 0 || num > 255)
    {
      free(copia);
      return 0; // No está en el rango correcto
    }

    segmento = strtok(NULL, ".");
  }

  free(copia);
  return segmentos == 4; // Debe haber exactamente 4 segmentos
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

    if (esEntero((lista[1])))
      idfilo = atoi(lista[1]);
    else
    {
      sprintf(msg, "El id del filosofo debe ser un numero entero\n"); // APARTADO 0.2
      printlog(msg);                                                  // APARTADO 0.2

      exit(2);
    }
    if (esEntero((lista[2])))
      numfilo = atoi(lista[2]);
    else
    {
      sprintf(msg, "El numero de filosofos debe ser un numero entero\n"); // APARTADO 0.2
      printlog(msg);                                                      // APARTADO 0.2

      exit(2); // APARTADO 0.1
    }

    if (esIP(lista[3]))
      strcpy(siguiente_chain, lista[3]);
    else
    {
      sprintf(msg, "La ip siguiente debe ser una ip valida\n"); // APARTADO 0.2
      printlog(msg);                                            // APARTADO 0.2

      exit(2); // APARTADO 0.1
    }
    if (esEntero((lista[4])))
      puerto_siguiente_chain = (unsigned short)atoi(lista[4]);
    else
    {
      sprintf(msg, "El puerto siguiente debe ser un numero entero\n"); // APARTADO 0.2
      printlog(msg);                                                   // APARTADO 0.2

      exit(2); // APARTADO 0.1
    }
    if (esEntero((lista[5])))
      puerto_local = (unsigned short)atoi(lista[5]);
    else
    {
      sprintf(msg, "El puerto local debe ser un numero entero\n"); // APARTADO 0.2
      printlog(msg);                                               // APARTADO 0.2

      exit(2); // APARTADO 0.1
    }
    if (esEntero((lista[6])))
      delay = atoi(lista[6]);
    else
    {
      sprintf(msg, "El delay debe ser un numero entero\n"); // APARTADO 0.2
      printlog(msg);                                        // APARTADO 0.2

      exit(2); // APARTADO 0.1
    }
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

  if (pthread_mutex_init(&mestado, NULL) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al inicializar el mutex\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                        // APARTADO 0.2

    exit(13); // APARTADO 0.1
  }

  if (pthread_cond_init(&condestado, NULL) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al inicializar la variable condicional\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                                       // APARTADO 0.2

    exit(14); // APARTADO 0.1
  }

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
  sprintf(msg, "Filosofo %d: Levantandose de la mesa\n", idfilo); // APARTADO 0.2
  printlog(msg);                                                  // APARTADO 0.2
  // levantandose de la mesa

  return NULL;
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
  default:
    return "desconocido";
  }
}

// sincronización con el cambio de estado a "comiendo"
void esperarPalillos(void)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                      // APARTADO 0.2
    exit(11);                                                           // APARTADO 0.1
  }

  while (estado != comiendo)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0) // APARTADO 0.1
    {
      sprintf(msg, "Filosofo %d: Error al esperar a que el estado sea comiendo\n", idfilo); // APARTADO 0.2
      printlog(msg);                                                                        // APARTADO 0.2
      exit(15);                                                                             // APARTADO 0.1
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                         // APARTADO 0.2
    exit(12);                                                              // APARTADO 0.1
  }
}
// sincronización con el cambio de estado a "pensando"
void soltarPalillos(void)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                      // APARTADO 0.2
    exit(11);                                                           // APARTADO 0.1
  }

  while (estado != pensando)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0) // APARTADO 0.1
    {
      sprintf(msg, "Filosofo %d: Error al esperar a que el estado sea pensando\n", idfilo); // APARTADO 0.2
      printlog(msg);                                                                        // APARTADO 0.2
      exit(15);                                                                             // APARTADO 0.1
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                         // APARTADO 0.2
    exit(12);                                                              // APARTADO 0.1
  }
}

// sincronización con el cambio de estado a "condimentando"
void esperarCuchara(void) // APARTADO 1
{
  if (pthread_mutex_lock(&mestado) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo);
    printlog(msg);
    exit(11);
  }

  while (estado != condimentando)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0)
    {
      sprintf(msg, "Filosofo %d: Error al esperar a que el estado sea condimentando\n", idfilo);
      printlog(msg);
      exit(15);
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo);
    printlog(msg);
    exit(12);
  }
}

// sincronización con el cambio de estado a "hablando"
void soltarCuchara(void) // APARTADO 1
{
  if (pthread_mutex_lock(&mestado) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo);
    printlog(msg);
    exit(11);
  }

  while (estado != hablando)
  {
    if (pthread_cond_wait(&condestado, &mestado) != 0)
    {
      sprintf(msg, "Filosofo %d: Error al esperar a que el estado sea hablando\n", idfilo);
      printlog(msg);
      exit(15);
    }
  }

  if (pthread_mutex_unlock(&mestado) != 0)
  {
    sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo);
    printlog(msg);
    exit(12);
  }
}

// modificando el estado del filósofo
void cambiarEstado(estado_filosofo nuevoestado)
{
  if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                      // APARTADO 0.2
    exit(11);                                                           // APARTADO 0.1
  }

  estado = nuevoestado;

  if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
  {
    sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.2
    printlog(msg);                                                         // APARTADO 0.2
    exit(12);                                                              // APARTADO 0.1
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
    break;
  case condimentando: // APARTADO 1
    if (cucharaLibre(*tok) == 1)
      *tok |= 0x80; // 10000000
    else if (cucharaLibre(*tok) == 2)
      *tok |= 0x40; // 01000000
    break;
  case hablando: // APARTADO 1
    if (cucharaLibre(*tok) == 1 || cucharaLibre(*tok) == 0)
      *tok &= 0xBF; // 10111111
    else if (cucharaLibre(*tok) == 2)
      *tok &= 0x7F; // 01111111
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

    if (pthread_mutex_lock(&mestado) != 0)
    {                                                                     // APARTADO 1
      sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 1
      printlog(msg);                                                      // APARTADO 1
      exit(11);                                                           // APARTADO 1
    }

    if (estado == queriendo_condimentar) // APARTADO 1
    {

      if (cucharaLibre(token[0]) != 0) // APARTADO 1
      {
        alterarToken(&token[0], condimentando); // APARTADO 1
        estado = condimentando;                 // APARTADO 1

        if (pthread_cond_signal(&condestado) != 0)                                                                         // APARTADO 1
        {                                                                                                                  // APARTADO 1
          sprintf(msg, "Filosofo %d: Error al señalar la condición en queriendo_condimentar -> condimentando.\n", idfilo); // APARTADO 1
          printlog(msg);                                                                                                   // APARTADO 1
          exit(17);                                                                                                        // APARTADO 1
        }
      }
    }

    else if (estado == dejando_condimentar) // APARTADO 1
    {

      alterarToken(&token[0], hablando); // APARTADO 1
      estado = hablando;                 // APARTADO 1

      if (pthread_cond_signal(&condestado) != 0) // APARTADO 1
      {
        sprintf(msg, "Filosofo %d: Error al señalar la condición en dejando_condimentar -> hablando.\n", idfilo); // APARTADO 1
        printlog(msg);                                                                                            // APARTADO 1
        exit(17);                                                                                                 // APARTADO 1
      }
    }

    if (pthread_mutex_unlock(&mestado) != 0)
    {                                                                        // APARTADO 1
      sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 1
      printlog(msg);                                                         // APARTADO 1
      exit(12);                                                              // APARTADO 1
    }

    if (pthread_mutex_lock(&mestado) != 0) // APARTADO 0.1
    {
      sprintf(msg, "Filosofo %d: Error al bloquear el mutex.\n", idfilo); // APARTADO 0.2
      printlog(msg);                                                      // APARTADO 0.2
      exit(11);                                                           // APARTADO 0.1
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
          sprintf(msg, "Filosofo %d: Error al señalar la condición en queriendo_comer -> comiendo.\n", idfilo); // APARTADO 0.2
          printlog(msg);                                                                                        // APARTADO 0.2
          exit(17);                                                                                             // APARTADO 0.1
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
        sprintf(msg, "Filosofo %d: Error al señalar la condición en dejando_comer -> pensar.\n", idfilo); // APARTADO 0.2
        printlog(msg);                                                                                    // APARTADO 0.2
        exit(17);                                                                                         // APARTADO 0.1
      }
    }

    if (pthread_mutex_unlock(&mestado) != 0) // APARTADO 0.1
    {
      sprintf(msg, "Filosofo %d: Error al desbloquear el mutex.\n", idfilo); // APARTADO 0.2
      printlog(msg);                                                         // APARTADO 0.2
      exit(12);                                                              // APARTADO 0.1
    }

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
        exit(18);                                                                            // APARTADO 0.1
      }
    }

    //  fin mientras
  }
}