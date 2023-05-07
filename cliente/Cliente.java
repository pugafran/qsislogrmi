package cliente;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import javax.sound.sampled.Line;

// Imports necesarios para RabbitMQ
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;



public class Cliente {
    // A RELLENAR el nombre de la cola
    private final static String NOMBRE_COLA = "FranciscoGabrielPL"; // A RELLENAR

    // Función main por la que arranca el cliente
    public static void main(String[] argv) throws Exception {
        // =================================================
        // Obtener argumentos de línea de comandos
        if (argv.length < 1) {
            System.out.println("Uso: cliente fichero_eventos");
            System.exit(1);
        }

        // =================================================
        // Parte principal, toda dentro de un try para capturar cualquier excepción
        try {
            // Conectar con Rabbit para poder enviar peticiones a la cola
            // A RELLENAR
            
            
            
            ConnectionFactory factory = new ConnectionFactory();
            Connection connection = factory.newConnection();
            
            factory.setHost("localhost");
            
            Channel channel = connection.createChannel();
            
            channel.queueDeclare(NOMBRE_COLA, true, false, false, null);

            // Arrancar la función que lee eventos del fichero y los envía por rabbit
            enviar_eventos(channel,argv[0]);

            // Terminar
            System.out.println("Cliente finalizado");
            // A RELLENAR cerrar comunicación con RabbitMQ
            channel.close();
            connection.close();

            System.exit(0);
        } 
        catch (Exception e) {
            // Cualquier excepción simplemente se imprime
            System.out.println("Error en Cliente" + e.getMessage());
            e.printStackTrace();
        }
    }

    // =========================================================================
    // La función lee del fichero de entrada y envia las líneas leídas
    // como mensajes a través de la cola de mensajes
    // Requiere como parámetros:
    //
    //  - El canal de comunicación con RabbitMQ para enviar los mensajes
    //  - El nombre del fichero con los eventos a enviar
    // 
    // Una vez finaliza de leer todos los mensajes y enviarlos a la cola, termina
    static void enviar_eventos(Channel channel, String fich_evt) throws IOException, InterruptedException 
    {
        BufferedReader br = new BufferedReader(new FileReader(fich_evt));
        try 
        {
            // Leer todas las líneas del fichero y enviarlas como mensajes
            // A RELLENAR
            for (String linea = br.readLine(); linea != null; linea = br.readLine()) {
                channel.basicPublish("", NOMBRE_COLA, null, linea.getBytes());
                System.out.println(" [x] Sent '" + linea + "'");
            }
            

        }
        catch (Exception e) {
            // Cualquier excepción simplemente se imprime
            System.out.println("Error en Cliente" + e.getMessage());
            e.printStackTrace();
        }	
        finally {
            br.close();
        }
    }
}
