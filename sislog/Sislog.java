package sislog;

// Imports necesarios para usar RabbitMQ
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Consumer;
import com.rabbitmq.client.DefaultConsumer;
import com.rabbitmq.client.Envelope;
import com.rabbitmq.client.AMQP;

// Imports necesarios para usar RMI
import java.io.IOException;
import java.rmi.Naming;
import java.rmi.RemoteException;

// Import necesario para escribir en fichero
import java.io.FileWriter;

// Import necesarios para obtener la fecha y hora actuales
import java.util.Date;

// Cola bloqueante para comunicar el hilo ReceptorEventos y los hilos Clasificador
import java.util.concurrent.ArrayBlockingQueue;

// ===================================================================
// Las dos clases siguientes son hilos que se ejecutarán de forma concurrente
//
// - ReceptorEventos es el hilo que espera mensajes de RabbitMQ e introduce
//   los mensajes recibidos en la cola interna (cola bloqueante)
// - Clasificador son los hilos que leen de la cola interna, contabilizan los mensajes
//   recibidos y en función de la facilidad los registran en uno u otro fichero
// ===================================================================


// Clase ReceptorEventos. Recibe mensajes por RabbitMQ, y los mete en la cola bloqueante 
// de eventos para que sean analizados y archivados por los hilos Clasificador
class ReceptorEventos extends Thread {
    // A RELLENAR el nombre de la cola de RabbitMQ
    private final static String NOMBRE_COLA_RABBIT; | // A RELLENAR
    private ArrayBlockingQueue<String> qevent;

    // El constructor recibe una referencia a las colas bloqueantes
    // que le permiten comunicarse con los hilos Clasificador
    public ReceptorEventos(ArrayBlockingQueue<String> qevent){
        this.qevent = qevent;
    }

    // La función run es la que se ejecuta al poner en marcha el hilo
    public void run() {
        // Conectar con rabbitMQ
        // A RELLENAR
        |
        |
        try {
            Connection connection = factory.newConnection();
            Channel channel = connection.createChannel();
            channel.queueDeclare(NOMBRE_COLA_RABBIT, false, false, false, null);

            // Espera por peticiones en la cola rabbitMQ
            Consumer consumer = new DefaultConsumer(channel) {
                @Override
                public void handleDelivery(String consumerTag, Envelope envelope, AMQP.BasicProperties properties, byte[] body) throws IOException {
                    // ************************************************************
                    // Recepción y manejo del mensaje que llega por RabbitMQ
                    // ************************************************************

                    // Convertir en cadena el mensaje recibido
                    String evtmsg = new String(body, "UTF-8");
                    System.out.println("ReceptorEventos: Recibido mensaje = " + evtmsg);
                    // A RELLENAR (enviar mensaje a cola interna bloqueante)
                    |
                    |
                    |
                    |
                }
            };
            System.out.println("ReceptorEventos. Esperando llegada de eventos");
            channel.basicConsume(NOMBRE_COLA_RABBIT, true, consumer);
        } catch (Exception e) {  // No manejamos excepciones, simplemente abortamos
            e.printStackTrace();
            System.exit(7);
        }
    }
}

// Clase Clasificador espera en una cola bloqueante a que el hilo ReceptorEventos le envíe
// un evento para contabilizar y archivar.
class Clasificador extends Thread {
    private ContabilidadEventos actev; //Objeto que permite registrar el total de eventos por facilidad y nivel
    private ArrayBlockingQueue<String> cola;  // Cola bloqueante en que los eventos a clasificar y contabilizar
    private String[] fac_names;    // Nombres de las facilidades u origenes de eventos
    private String[] level_names;  // Nombres de los niveles de severidad de los eventos
    private String[] fac_file_names; // Nombre de los ficheros de registro de los eventos

    // El constructor recibe la cola y los arrays de cadenas con nombres de facilidades, niveles y ficheros
    public Clasificador(ContabilidadEventos actev,
                        ArrayBlockingQueue<String> cola,
                        String[] fac_names,
                        String[] level_names,
                        String[] fac_file_names) {
        this.actev = actev;
        this.cola = cola;
        this.fac_names = fac_names;
        this.level_names = level_names;
        this.fac_file_names = fac_file_names;
    }

    // El método run es el que se ejecuta al arrancar el hilo
    public void run() {
        try {
            while (true) {  // Bucle infinito
                // A RELLENAR (esperar evento en la cola bloqueante, extraer mensaje, procesarlo y contabilizarlo)
                // Si se produce un error al tokenizar el mensaje, se emite un mensaje de error y se ignora
                // Si se puede tokenizar correctamente se escribe el mensaje de log en el fichero 
                // correspondiente y en el formato especificado en el enunciado, y se contabiliza el evento
                // a través de actev.
                |
                |
                |
                |
                |
                |
                |
                |
                |
            }
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(8);
        }
    }
}


// Clase principal que instancia el hilo de recepción y los hilos de clasificacion y los arranca
// Clase principal que instancia el hilo de recepción y los hilos de clasificacion y los arranca
public class Sislog {
    // Función main por la que arranca el programa
    public static void main(String[] argv) throws Exception {
        // Arrays con nombres de facilidades, niveles y ficheros de registro
        String[] facilities_names={
            "kern",
            "user",
            "mail",
            "daemon",
            "auth",
            "syslog",
            "lpr",
            "news",
            "uucp",
            "cron"
        };
        String[] level_names={
            "emerg",
            "alert",
            "crit",
            "err",
            "warning",
            "notice",
            "info",
            "debug"
        };
        String[] facilities_file_names={
            "fac00.dat",
            "fac01.dat",
            "fac02.dat",
            "fac03.dat",
            "fac04.dat",
            "fac05.dat",
            "fac06.dat",
            "fac07.dat",
            "fac08.dat",
            "fac09.dat"
        };
        // Los valores de estas variables se leen de línea de comandos
        int max_facilidades=0;      // Maximo de origenes de alertas
        int max_niveles=0;          // Maximo de niveles de severidad
        int tam_cola=0;             // Tamaño de la array blocking queue
        int num_workers=0;          // Numero de hilos trabajadores

        // Cola interna de sincronización entre el hilo ReceptorPeticiones y los Worker
        ArrayBlockingQueue<String> cola_interna;
        // Variable que representa el hilo receptor de eventos
        ReceptorEventos receptor_eventos;
        // Variable que representa a los hilos trabajadores
        Clasificador[] clasificadores;

        // Lectura de la línea de comandos
        if (argv.length < 4) {
            System.out.println("Uso: Syslog max_facilidades max_niveles tam_cola num_workers");
            System.exit(1);
        }
        try {
            max_facilidades = Integer.parseInt(argv[0]);
            max_niveles = Integer.parseInt(argv[1]);
            tam_cola = Integer.parseInt(argv[2]);
            num_workers = Integer.parseInt(argv[3]);
        } 
        catch (NumberFormatException e) {
            System.out.println("max_facilidades, max_niveles, tam_cola y numworkers deben ser enteros");
            System.exit(2);
        }

        if ((max_facilidades<1) || (max_facilidades>facilities_names.length))
        {
            System.out.println("max_facilidades debe ser un valor >=1 y <="+facilities_names.length);
            System.exit(3);
        }
        if ((max_niveles<1) || (max_niveles>level_names.length))
        {
            System.out.println("max_niveles debe ser un valor >=1 y <="+level_names.length);
            System.exit(4);
        }
        if (tam_cola<1)
        {
            System.out.println("tam_cola debe ser un valor >=1");
            System.exit(5);
        }
        if (num_workers<1){
            System.out.println("num_workers debe ser un valor >=1");
            System.exit(6);
        }

        // Creamos el objeto que nos permite llevar la contabilidad de los eventos
        // A RELLENAR
        |
        |
        
        // Creamos la cola interna de sincronización entre hilos
        // A RELLENAR
        |
        |

        
        // Manager de seguridad para RMI
        if (System.getSecurityManager() == null) {
            System.setSecurityManager(new SecurityManager());
        }
        
        try {
            // Arrancar el servidor RMI y registrar el objeto remoto que implementa la interfaz
            // A RELLENAR
            |
            |
            |
            |
            System.out.println("Sislog registrado para RMI");
        } 
        catch (Exception e) {
            // Cualquier excepción simplemente se imprime y se ignora
            System.out.println("Error al registrar el objeto RMI interfaz con el Syslog: " + e.getMessage());
            e.printStackTrace();
        }
        // Creamos los hilos clasificadores, guardamos sus referencias en un array y los arrancamos
        clasificadores = new Clasificador[num_workers];
        // A RELLENAR
        |
        |
        |

        // Creamos el hilo receptor de eventos, almacenamos su referencia y lo arrancamos
        // A RELLENAR
        |
        |
        
        // Esperamos a que finalice el hilo receptor de eventos (nunca finalizará, hay que parar con Ctrl+C)
        receptor_eventos.join();
    }
}