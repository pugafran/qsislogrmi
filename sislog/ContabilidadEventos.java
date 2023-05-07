package sislog;

// Clase que mantiene la contabilidad de los eventos recibidos. 
// Ya que algunos de sus métodos pueden ser invocados desde diferentes hilos
// se sincronizan entre sí mediante bloques synchronized (exclusión mutua) 

class ContabilidadEventos {
    private int val[][];           // Matriz de enteros que contabiliza los eventos recibidos
    private int filas,columnas;    // Numero de filas (facilidades) y columnas (niveles) que tiene la matriz

    public ContabilidadEventos(int fils, int cols) {
        // Inicializador
        filas=fils;
        columnas=cols;	   
        val=new int[fils][cols];
        for (int i=0;i<filas;i++)
            for (int j=0;j<columnas;j++)
                val[i][j]=0;
    }

    // Metodo que contabiliza un evento
    void contabilizaEvento(int facilidad, int nivel){
        // Comprobar si facilidad y nivel están dentro de los limites del array
        // Si no están no se hace nada. Si están, se incrementa el contador
        // correspondiente en un bloque synchronized (exclusión mutua)
        
        // A RELLENAR
        |
        |
        |
        |
    }

    // Metodo que devuelve el número de eventos contabilizados para una facilidad y un nivel dados
    int obtenerValorFacilidadNivel(int facilidad, int nivel)
    {
       int ret;
       // La lectura del contador debe hacerse sincronizada por si otro hilo
       // lo está modificando

       // A RELLENAR
       |
       |
       |
       return ret;
    }
    
    int obtenerNumeroFacilidades()
    {
       return filas;
    }
    
    int obtenerNumeroNiveles()
    {
       return columnas;
    }
}
