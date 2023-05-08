package cliente;

// Imports necesarios para RabbitMQ
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;

// Imports necesarios para RMI
import java.io.IOException;
import java.rmi.Naming;
import java.rmi.RemoteException;

// Imports necesarios para invocar via RMI métodos del sislog
import sislog.SislogInterface;

public class Estadis {
    // Funcion main por la que arranca
    public static void main(String[] argv) throws Exception {
        int nfils, ncols, suma, n, total;   // Variables para las estadísticas
        SislogInterface evtserv;            // Objeto remoto para invocar métodos RMI
    
        // =================================================
        // Instanciar SecurityManager necesario para RMI
        if (System.getSecurityManager() == null) {
            System.setSecurityManager(new SecurityManager());
        }

        // =================================================
        // Parte principal, toda dentro de un try para capturar cualquier excepción
        try {
            // A RELLENAR. Obtener por RMI el número de niveles (ncols) y facilidades (nfils)
            evtserv = (SislogInterface) Naming.lookup("//localhost/Sislog");
            ncols = evtserv.obtenerNumeroNiveles();
            nfils = evtserv.obtenerNumeroFacilidades();

            System.out.println("**************************  RECUENTO EVENTOS  ********************************");
            System.out.print("\t");

            // Imprimir, separados por \t, los nombres de las columnas (nombres de niveles
            // que se obtienen por RMI)
            // A RELLENAR
            for (int i=0;i<ncols;i++)
                System.out.print(evtserv.obtenerNombreNivel(i) + "\t");
            


            System.out.println("TOTAL");
            suma=0;
            for (int i=0;i<nfils;i++)
            {
                // Para cada fila se imprime primero el nombre de la facilidad (obtenido por RMI)
                // con un \t al final
                // A RELLENAR
                System.out.print(evtserv.obtenerNombreFacilidad(i) + "\t");


                suma=0;

                // Seguidamente se itera por cada columna (nivel) y se obtiene por RMI el valor del
                // contador correspodiente a la facilidad y nivel actual (fila y columna)
                for (int j=0;j<ncols;j++)
                {
                    // A RELLENAR
                    n = evtserv.obtenerValorFacilidadNivel(i, j);

                    System.out.print(n+"\t");
                    suma += n;
                }
                System.out.println(suma);
            }
            // Una vez impresas todas las filas, se imprime una fila final con los totales
            // Estos hay que computarlos por columnas, para lo que de nuevo se
            // harán llamadas RMI para obtener los valores de cada contador
            System.out.print("TOTALES\t");
            total=0;
            for (int j=0;j<ncols;j++)
            {
                suma=0;  // Para computar el total por columnas
                for (int i=0;i<nfils;i++)
                {
                    // A RELLENAR (RMI y actualización de suma)
                    n = evtserv.obtenerValorFacilidadNivel(i, j);
                    suma += n;

                }
                System.out.print(suma+"\t");
                total+=suma;
            }
            System.out.println(total);
        } 
        catch (Exception e) {
            // Cualquier excepción simplemente se imprime
            System.out.println("Error en Estadis" + e.getMessage());
            e.printStackTrace();
        }
    }
}
