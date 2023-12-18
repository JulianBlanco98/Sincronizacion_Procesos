#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/shm.h>

// Declarar las variables globales
static int filas = 15;       // filas de la matriz
static int columnas = 10000; // columnas de la matriz
int tuberia[2];              // tubería padre - hijo

// Estructura para almacenar el PID y el total de números primos de cada hijo de NV 2
struct infoHijoNivel2
{
    int pid;
    int totalPrimos;
};

// Declarar las funciones a realizar
void inicializarMatriz(int matriz[filas][columnas]);
void mostrarMatriz(int matriz[filas][columnas]);
int calcularNumerosPrimos(int fila, int matriz[filas][columnas]);
void escribirPrimoEnArchivo(int nivel, int id_proceso, int num_primo);

// Funciones de las señales
void manejador_senial_SIGUSR1(int);
void manejador_senial_SIGINT(int);

int main(int argc) //, char *argv[] le he quitado esta línea para que no me salga
{
    if (argc > 1)
    {
        printf("\nNúmero de parámetros incorrecto. No se admite ninguno \n");
        exit(0);
    }
    // Borrar los archivos de la anterior ejecución
    system("rm -f N1_*.primos");
    system("rm -f N2_*.primos");
    system("rm -f N3_*.primos");

    // Crear el conjunto de datos
    int hijo[3];                 // 3 procesos hijos: nivel 2
    int matriz[filas][columnas]; // matriz de trabajo
    int estado;                  // variable para usar en wait
    int numerosPrimosMatriz = 0; // total de números primos
    char nombreArchivo_Padre[20];
    sprintf(nombreArchivo_Padre, "N1_%d.primos", getpid());

    // Memoria compartida
    key_t llave;                            // Clave para obtener con ftok
    long int shmid;                         // ID de memoria a obtener con shmget
    struct infoHijoNivel2 *infoHijolNivel2; // Puntero de memoria compartida

    // señales
    struct sigaction a;

    // armado de señal SIGURS1 del hijo2 (emisor) a padre (receptor)
    a.sa_handler = manejador_senial_SIGUSR1;
    sigemptyset(&a.sa_mask);
    a.sa_flags = 0;
    sigaction(SIGUSR1, &a, NULL);

    // Crear la memoria compartida
    llave = ftok("/bin/cat", 121);
    shmid = shmget(llave, 3 * sizeof(struct infoHijoNivel2), 0777 | IPC_CREAT); // Reservar espacio para 3 estructuras
    if (shmid == -1)
    {
        printf("Error en la creacion de la memoria compartida.\n");
        exit(0);
    }
    infoHijolNivel2 = (struct infoHijoNivel2 *)shmat(shmid, NULL, 0);
    // Inicializar la memoria compartida
    for (int i = 0; i < 3; i++)
    {
        infoHijolNivel2[i].pid = 0;
        infoHijolNivel2[i].totalPrimos = 0;
    }

    // Creación de la tubería
    if (pipe(tuberia) < 0)
    {
        printf("\nError en la creación de la tubería \n");
        exit(0);
    }

    // Inicializar la matriz con los aleatorios: 0 a 30000
    inicializarMatriz(matriz);
    // mostrarMatriz(matriz);

    FILE *archivoPadre = fopen(nombreArchivo_Padre, "a");
    fprintf(archivoPadre, "COMIENZA EL PROGRAMA EL PADRE (PID: %d)...\n", getpid());
    fclose(archivoPadre);
    printf("\nProceso padre (%d) emepezando el programa...\n", getpid());

    // Asignar a los procesos hijos de nivel 2, las filas a utilizar
    int inicio_hijo = 0;
    // Crear los 3 procesos hijos de nivel 2
    for (int l = 0; l < 3; l++)
    {
        int totalPrimosAux = 0; // Contador primos de cada hijo de nivel 2

        hijo[l] = fork();
        switch (hijo[l])
        {
        case -1: // Error
            printf("Error al crear el proceso de hijo 1. \n");
            break;
        case 0: // Código de los hijos de nivel 2

            printf("\n\tProceso HIJO %d (PID: %d) | Proceso padre (PID: %d) \n", l + 1, getpid(), getppid());
            // armado de señal SIGINT del padre (emisor) a hijo (receptor)
            a.sa_handler = manejador_senial_SIGINT;
            sigemptyset(&a.sa_mask);
            a.sa_flags = 0;
            sigaction(SIGINT, &a, NULL);

            fopen(nombreArchivo_Padre, "a");
            fprintf(archivoPadre, "Padre (PID: %d) crea el hijo de Nivel 2: (PID: %d)\n", getppid(), getpid());
            fclose(archivoPadre);

            // Crear los ficheros para los hijos de Nivel 2
            char nombreArchivo_hijo2[20];
            sprintf(nombreArchivo_hijo2, "N2_%d.primos", getpid());
            FILE *archivo_hijo2 = fopen(nombreArchivo_hijo2, "a");
            fprintf(archivo_hijo2, "Inicio de ejecución del hijo %d de Nivel 2 (PID: %d)\n", l + 1, getpid());
            fclose(archivo_hijo2);
            infoHijolNivel2[l].pid = getpid();

            // Calcular el inicio y el final de cada proceso hijo de nivel 3
            int inicio_fila = inicio_hijo;  // 0, 5, 10
            int fin_fila = inicio_hijo + 4; // 4, 9 ,14

            printf("\tProceso HIJO %d (PID: %d) va a tratar las filas: %d - %d\n", l + 1, getpid(), inicio_fila, fin_fila);

            // Crear los 5 procesos hijos de nivel 3 del hijo 1.
            for (int k = inicio_fila; k <= fin_fila; k++)
            {
                int hijo_nivel3 = fork();

                switch (hijo_nivel3)
                {
                case -1: // Error
                    printf("Error al crear el proceso hijo de nivel 3(%d). \n", k);
                    break; // Código de los hijos de nivel 3
                case 0:
                    printf("\n\t\tProceso Nieto %d (PID: %d) | Proceso Padre (PID: %d) \n", k, getpid(), getppid());
                    // filaMatriz(k, matriz);
                    fopen(nombreArchivo_hijo2, "a");
                    fprintf(archivo_hijo2, "Crea el proceso de Nivel 3: %d\n", getpid());
                    fclose(archivo_hijo2);
                    int primosEnFila = calcularNumerosPrimos(k, matriz);
                    printf("\t\tNúmero total de primos en la fila %d: %d\n", k, primosEnFila);

                    exit(primosEnFila);
                    break;
                default:

                    break;
                }
            }
            // esperar a que terminen los 5 hijos de nivel 3
            for (int i = 0; i < 5; i++)
            {
                wait(&estado);
                if (WIFEXITED(estado) >= 0)
                {
                    printf("\tNúmeros primos encontrados: %d \n", WEXITSTATUS(estado));
                    totalPrimosAux = totalPrimosAux + WEXITSTATUS(estado);
                }
            }
            fopen(nombreArchivo_hijo2, "a");
            fprintf(archivo_hijo2, "Total de números primos encontrados: %d", totalPrimosAux);
            fclose(archivo_hijo2);

            infoHijolNivel2[l].totalPrimos = totalPrimosAux; // esctibo en memoria compartida el total de números primos
            // escribir en la tubería la estructura para recuperar
            write(tuberia[1], &infoHijolNivel2[l], sizeof(infoHijolNivel2[l]));
            usleep(500);
            kill(getppid(), SIGUSR1); // Llamada a SIGURS1 para comunicarse con el padre (receptor)
            fopen(nombreArchivo_Padre, "a");
            fprintf(archivoPadre, "Señal SIGUSR1 del hijo (PID: %d) al Padre (PID: %d) para comunicar que puede leer de la tubería\n", getpid(), getppid());
            fclose(archivoPadre);
            pause();

        default:

            break;
        }
        inicio_hijo = inicio_hijo + 5; // actualizar el contador una vez que se traten las 5 filas correspondientes al hijo (nivel2)
    }
    // esperar a que los 3 hijos de nivel 2 acaben
    for (int i = 0; i < 3; i++)
    {
        wait(&estado);
    }
    printf("El proceso padre ha creado 3 procesos hijos con los siguientes PID: %d, %d, %d.\n", hijo[0], hijo[1], hijo[2]);

    for (int i = 0; i < 3; i++)
    {
        printf("Total de primos para el Hijo %d: %d\n", infoHijolNivel2[i].pid, infoHijolNivel2[i].totalPrimos);
    }
    for (int i = 0; i < 3; i++)
    {
        numerosPrimosMatriz = numerosPrimosMatriz + infoHijolNivel2[i].totalPrimos;
    }
    printf("\nEl número total de primos de la matriz es: %d\n", numerosPrimosMatriz);
    // Liberar memomria compartida
    shmdt(infoHijolNivel2);
    shmctl(shmid, IPC_RMID, 0);

    fopen(nombreArchivo_Padre, "a");
    fprintf(archivoPadre, "TOTAL DE PRIMOS ENCONTRADOS EN LA MATRIZ: %d\n", numerosPrimosMatriz);
    fclose(archivoPadre);

    exit(0);
}

/**
 * Función para generar los valores aleatorios de la matriz (0 a 30000)
 * @param matriz: matriz de enteros de la práctica
 */
void inicializarMatriz(int matriz[filas][columnas])
{
    srand(time(NULL)); // función para los aleatorios
    for (int i = 0; i < filas; i++)
    {
        for (int j = 0; j < columnas; j++)
        {
            matriz[i][j] = rand() % 30000; // Números aleatorios entre 0 y 30000
        }
    }
}
/**
 * Función para mostrar la matriz por pantalla.
 * @param matriz: matriz de enteros de la práctica
 */
void mostrarMatriz(int matriz[filas][columnas])
{
    printf("\nMatriz------------------------------------------------\n");
    for (int i = 0; i < filas; i++)
    {
        printf("Fila %2d: ", i);
        for (int j = 0; j < columnas; j++)
        {
            printf("| %2d ", matriz[i][j]);
        }
        printf("|\n");
    }
}
/**
 * Función para indicar el total de primos en una fila de la matriz.
 * @param matriz: matriz de enteros de la práctica.
 * @param fila: fila de la matriz para buscar el total de primos.
 */
int calcularNumerosPrimos(int fila, int matriz[filas][columnas])
{
    int contadorPrimos = 0;
    for (int j = 0; j < columnas; j++)
    {
        int num = matriz[fila][j];
        bool esPrimo = true;
        if (num <= 1)
        {
            esPrimo = false;
        }
        else
        {
            for (int i = 2; i * i <= num; i++)
            {
                if (num % i == 0)
                {
                    esPrimo = false;
                    break;
                }
            }
        }
        if (esPrimo)
        {
            contadorPrimos++;
            escribirPrimoEnArchivo(3, getpid(), num);
        }
    }
    return contadorPrimos;
}
/**
 * Función para generar los ficheros asociados a los hijos de nivel 3.
 * @param nivel: entero para el nivel de jerarquía del proceso (nivel 3).
 * @param id_proceso: entero del pid del proceso hijo de nivel 3.
 * @param num_primo: entero del número primo encontrado.
 */
void escribirPrimoEnArchivo(int nivel, int id_proceso, int num_primo)
{
    // Creación del fichero: N3:pid.primos
    char nombreArchivo[20];
    sprintf(nombreArchivo, "N%d_%d.primos", nivel, id_proceso);
    FILE *archivo = fopen(nombreArchivo, "a");
    // Escribir el número primo en el archivo
    fprintf(archivo, "%d:%d:%d\n", nivel, id_proceso, num_primo);
    fclose(archivo);
}
/**
 * Función para manejar cuando se reciba la señal SIGUSR1.
 */
void manejador_senial_SIGUSR1(int senial)
{
    struct infoHijoNivel2 info; // puntero para obtener la estructura
    pid_t pid = getpid();

    // Leer la estructura desde la tubería
    read(tuberia[0], &info, sizeof(info));
    printf("------> Proceso PADRE (PID: %d) recibió %d primos del Hijo %d\n", pid, info.totalPrimos, info.pid);
    // Envía la señal SIGINT al hijo2 (PID) para que termine
    kill(info.pid, SIGINT);
    usleep(500);
}
/**
 * Función para manejar cuando se reciba la señal SIGINT.
 */
void manejador_senial_SIGINT(int senial)
{
    pid_t pid = getpid();
    printf("\n\t\t\t!!!!!!!!!!!!!!!!!!!!!!!!HIJO %d finalizando.!!!!!!!!!!!!!!!!!!!!!!!!\n", pid);
    exit(0);
}
