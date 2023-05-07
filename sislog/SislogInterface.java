package sislog;
import java.rmi.Remote;
import java.rmi.RemoteException;

/*
  Interfaz remoto del Sislog que ha de hacer público los metodos que pueden ser
  invocados desde un cliente RMI como Estadis.java
*/
public interface SislogInterface extends Remote {
    // A RELLENAR
    int obtenerValorFacilidadNivel(int facilidad,int nivel) throws RemoteException;
    int obtenerNumeroFacilidades() throws RemoteException;
    int obtenerNumeroNiveles() throws RemoteException;
    String obtenerNombreFacilidad(int facilidad) throws RemoteException;
    String obtenerNombreNivel(int nivel) throws RemoteException;
      
}
