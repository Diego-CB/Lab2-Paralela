#include <unistd.h>     //std lib
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <fstream>      //fstream, ofstream, ifstream
#include <string>       //string
#include <omp.h>        //paralelizar es una fiesta!
#include <ctime>

#define INFILE "numeros.csv"
#define LOWFILE "low.csv"
#define MIDFILE "mid.csv"
#define HIGFILE "high.csv"

using namespace std;

//Funciton prototypes
bool isPrime(int number);
int compare (const int * a, const int * b); //what is it returning?
void par_qsort(int *data, int lo, int hi); //}, int (*compare)(const int *, const int*))
//void outputLine(int primeNumber);

int main(int argc, char * argv[]) {

int exponente = 2;

    int primeNumber;
    char expChar;

    #pragma omp master
    {
        //UX input rango de números: exponente
        cout<<"Ingrese un exponente base 10 para el rango de números: ";
        cin>>expChar;
    }
    #pragma omp barrier
    

    //Offset para procesar ASCII - chars inician en val=48
    exponente =int(expChar) - 48;
    long limit = pow(10,exponente);

    cout<<"Buscaremos los primos entre 0 y "<<limit<<endl;

    /*----------
     *Constructor del manejador de archivos
     *
     *  in/out - fstream <identificador>("<archivo>",modo)
     *  out only - ofstream <identificador>("<archivo>",modo)
     *  in only - ifstream <identificador>("<archivo>")
     *
     *  modos:
     *      ios::app - append to file
     *      ios::out - new/overwrite file
     *      ios::binary - nontext input & output
     *      ios::in - read from file
     */
    ofstream escribirNumeros(INFILE,ios::out);
    if( escribirNumeros.bad() ) {
        cerr<<"Falló la creación del archivo "<<INFILE<<endl;
        exit(EXIT_FAILURE);
    }

    //Llenamos el archivo INFILE con números aleatorios
    int posibles_elementos = limit / 2;
    if (posibles_elementos < 1){
        posibles_elementos = 1;
    }

    unsigned int semilla = static_cast<unsigned int>(std::time(nullptr) + omp_get_thread_num());

    #pragma omp parallel for
    for (int i = 0; i < limit; i++) {
        int num = rand_r(&semilla) % (posibles_elementos) + 1;

        #pragma omp critical
        {
            if (i > 0) {
                escribirNumeros << ",";  // Agregar coma solo si no es el primer número
            }
            escribirNumeros << num;
        }
    }

    // Finalmente, escribir el último número fuera de la región crítica
    escribirNumeros << "," << limit << std::endl;
    escribirNumeros.close();
    
    //Constructores para lectura de archivo y escritura de resultados
    //Esta es otra forma de realizar el manejo de arhivos
    ifstream leerNumeros;
    leerNumeros.open(INFILE);
    if( leerNumeros.bad() ) {
        cerr<<"No se pudo leer el archivo "<<INFILE<<endl;
        exit(EXIT_FAILURE);
    }

    // ----- Carga de datos a memoria principal

    cout << "ante arrays" << endl;

    // int * Array = new int[limit];
    int * Array = (int*) malloc(limit * sizeof(int));

    // Se leen los datos del infile y se gurdan en un array
    string ch;
    int index = 0;
    while(getline(leerNumeros, ch,',')) {

        int numero = stoi(ch);  //Debemos convertir el string a int
        Array[index] = numero;
        index++;
    }

    // ----- Clasificacions de los numeros
    //Cambio no.2
    #pragma omp parallel
    {
        #pragma omp single
        par_qsort(Array, 0, limit-1);
    }

    // ----- Escritura de clasificaciones

    // Abrir los archivos de escritura
    ofstream low_write;
    low_write.open(LOWFILE);
    if( low_write.bad() ) {
        cerr<<"No se pudo crear el archivo "<<LOWFILE<<endl;
        exit(EXIT_FAILURE);
    }

    ofstream mid_write;
    mid_write.open(MIDFILE);
    if( mid_write.bad() ) {
        cerr<<"No se pudo crear el archivo "<<MIDFILE<<endl;
        exit(EXIT_FAILURE);
    }

    ofstream hig_write;
    hig_write.open(HIGFILE);
    if( low_write.bad() ) {
        cerr<<"No se pudo crear el archivo "<<HIGFILE<<endl;
        exit(EXIT_FAILURE);
    }

    cout << "ante limites" << endl;

    int low_limit =  (int) limit / 3;
    int mid_limit = low_limit * 2;

    cout << "limites " << low_limit << " " << mid_limit << " " << limit << endl;


    //Cambio no. 3 FINAL
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Escribir números bajos
            #pragma omp parallel for
            for (int i = 0; i < low_limit; i++) {
                low_write << Array[i];

                if (i < low_limit - 1) {
                    low_write << ",";
                }
            }
        }

        #pragma omp section
        {
            // Escribir números medios
            #pragma omp parallel for
            for (int i = low_limit; i < mid_limit; i++) {
                mid_write << Array[i];

                if (i < mid_limit - 1) {
                    mid_write << ",";
                }
            }
        }

        #pragma omp section
        {
            // Escribir números altos
            #pragma omp parallel for
            for (int i = mid_limit; i < limit; i++) {
                hig_write << Array[i];

                if (i < limit - 1) {
                    hig_write << ",";
                }
            }
        }
    }

    // escribir clasificaciones
    printf("Primeros Elementos: %d, %d, %d\n", Array[0],       Array[1],          Array[2]);
    low_write << Array[0] << ",";
    low_write << Array[1] << ",";
    low_write << Array[2];
    low_write.close();

    printf("Medios Elementos: %d, %d, %d\n",   Array[limit/4], Array[limit/4 +1], Array[limit/4 +2]);
    mid_write << Array[limit/4] << ",";
    mid_write << Array[limit/4 + 1] << ",";
    mid_write << Array[limit/4 + 2];
    mid_write.close();

    printf("Ultimos Elementos: %d, %d, %d\n",  Array[limit-3], Array[limit-2],    Array[limit-1]);
    hig_write << Array[limit-3] << ",";
    hig_write << Array[limit-2] << ",";
    hig_write << Array[limit-1];
    hig_write.close();

    cout << endl;
    delete Array;
    return 0;
}

/*-----------
 * isPrime - test for a prime number
 */
bool isPrime(int number){
    if (number == 0 || number == 1)
	{
		return false;
	}
	int divisor;
	for (divisor = number/2; number%divisor != 0; --divisor)
	{
		;
	}
	if (divisor != 1)
	{
		return false;
	}
	else
	{
		return true;
	}
} /*isPrime*/

/*-----------
 * outputLine - format text in line form and output to ifstream
 */
// void outputLine(int primeNumber){
//     long primeLong = (long)primeNumber;
// 	double primeRoot = (double)primeLong;

// 	cout<<primeNumber<<" "<<endl;
// }

//comparar
int compare (const int * a, const int * b) //what is it returning?
{
   return ( *(int*)a - *(int*)b ); //What is a and b?
}

//Cambio no.2
void par_qsort(int *data, int lo, int hi) {
  if (lo >= hi) return;
  
  int l = lo;
  int h = hi;
  int p = data[(hi + lo) / 2];

  while (l <= h) {
    while ((data[l] - p) < 0) l++;
    while ((data[h] - p) > 0) h--;
    if (l <= h) {
      int tmp = data[l];
      data[l] = data[h];
      data[h] = tmp;
      l++;
      h--;
    }
  }

  #pragma omp task
  par_qsort(data, lo, h);
  
  #pragma omp task
  par_qsort(data, l, hi);

  #pragma omp taskwait
}
