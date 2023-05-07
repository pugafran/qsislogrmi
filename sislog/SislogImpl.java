package sislog;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.concurrent.ArrayBlockingQueue;

/*
La clase SislogImpl implementa el interfaz SislogInterface:

obtenerValorFacilidadNivel(): que será invocado desde un cliente RMI 
    para obtener el número de eventos de un determinado nivel correspondientes 
    a una determinada facilidad

obtenerNumeroFacilidades(): devuelve el numero de facilidades
    con los cuales puede trabajar el sislog.

obtenerNumeroNiveles(): devuelve el numero de niveles de severidad 
    con los cuales puede trabajar el sislog.

obtenerNombreFacilidad(): obtiene el nombre de la facilidad
    cuyo id le pasamos como argumento.

obtenerNombreNivel(): obtiene el nobre del nivel (severidad) 
    cuyo id le pasamos como argumento.

*/

public class SislogImpl extends UnicastRemoteObject implements SislogInterface {
    private ContabilidadEventos accountev;  // Objeto que registra la contabilidad de los eventos recibidos
    private String[] fac_names;             // Nombres de facilidades
    private String[] level_names;           // Nombres de niveles

    public SislogImpl(ContabilidadEventos accountev, String [] fac_names, String[] level_names) throws RemoteException {
        super();
        this.accountev = accountev;
        this.fac_names = fac_names;
        this.level_names = level_names;
    }

    @Override
    public int obtenerValorFacilidadNivel(int facilidad,int nivel) throws RemoteException {
        // A RELLENAR
        |
        |
    }

    @Override
    public int obtenerNumeroFacilidades() throws RemoteException {
        // A RELLENAR
        |
        |
    }

    @Override
    public int obtenerNumeroNiveles() throws RemoteException {
        // A RELLENAR
        |
        |
    }

    @Override
    public String obtenerNombreFacilidad(int facilidad) throws RemoteException {
        // A RELLENAR
        |
        |
    }

    @Override
    public String obtenerNombreNivel(int nivel) throws RemoteException {
        // A RELLENAR
        |
        |
    }
}
