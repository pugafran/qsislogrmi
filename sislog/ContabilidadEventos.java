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
        if ((facilidad < filas && nivel < columnas) && (facilidad >= 0 && nivel >= 0)) {
            synchronized (this) {
                val[facilidad][nivel]++;
            }
        }

        else
            System.err.println("Error en contabilidad: facilidad o nivel fuera de rango");


    }

    // Metodo que devuelve el número de eventos contabilizados para una facilidad y un nivel dados
    int obtenerValorFacilidadNivel(int facilidad, int nivel)
    {
       
        if (facilidad < 0 || facilidad >= filas || nivel < 0 || nivel >= columnas) {
            System.err.println("Error en contabilidad: índices fuera de rango.");
            return -1;  
        }
       
        int ret;
       // La lectura del contador debe hacerse sincronizada por si otro hilo
       // lo está modificando

       // A RELLENAR
         synchronized (this) {
              ret = val[facilidad][nivel];
         }
         
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
