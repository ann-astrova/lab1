#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

long thread_count = 10;
long part = 1000;
double step;
double g;
long n;
double *m;
double **X;
double **Y;
double **U_X;
double **U_Y;
double ** F_X_local;
double ** F_Y_local;
double *F_X;
double *F_Y;
long arr = 0;
long iter = 0;
pthread_cond_t cond;
pthread_mutex_t mutex;


void barier(){
    pthread_mutex_lock(&mutex);
    arr++;
    long thread_iter = iter;
    if (arr == thread_count){
        arr=0;
        iter++;
        pthread_cond_broadcast(&cond);
    }
    else{
        while (thread_iter == iter){
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
}


void f(int t_p, long start, long count, long my_rank){
    double fijx;
    double fijy;
    for (long i = 0; i < n; i++){
        F_X_local[my_rank][i] = 0;
        F_Y_local[my_rank][i] = 0;
    }
    
    for (long i = start; i < start + count; i++){
        for (long j= i + 1; j<n; j++){
            double r = sqrt(pow(X[t_p][j] - X[t_p][i], 2) + pow(Y[t_p][j] - Y[t_p][i], 2));
            if (r != 0){
                fijx = (g * m[i] * m[j] * (X[t_p][j] - X[t_p][i])) / pow(r, 3);
                fijy = (g * m[i] * m[j] * (Y[t_p][j] - Y[t_p][i])) / pow(r, 3);
                F_X_local[my_rank][i] += fijx;
                F_X_local[my_rank][j] -= fijx;
                F_Y_local[my_rank][i] += fijy;
                F_Y_local[my_rank][j] -= fijy;

            }
            
        }
    }
}


void* routine(void* rank){
    long my_rank = (long)rank;
    long point = n/thread_count;
    long ost = n - thread_count * point;
    long start = my_rank * point + (my_rank < ost ? my_rank : ost);
    long count = point + (my_rank < ost ? 1 : 0);

    for (int t=1; t < part + 1; t++){
        f(t - 1, start, count, my_rank);
        barier();
        if (my_rank == 0){
            for (long p=0; p<thread_count; p++){
                for (long i=0; i<n; i++){
                    F_X[i] += F_X_local[p][i];
                    F_Y[i] += F_Y_local[p][i];
                }
            }
        }
        barier();

        for (long i=start; i < start + count; i++){
            double f_x_j = F_X[i];
            double f_y_j = F_Y[i];
            X[t][i] = X[t-1][i] + U_X[t-1][i] * step;
            Y[t][i] = Y[t-1][i] + U_Y[t-1][i] * step;
            U_X[t][i] = U_X[t-1][i] +  step  * f_x_j / m[i];
            U_Y[t][i] = U_Y[t-1][i] +  step  * f_y_j / m[i];
            F_X[i] = 0;
            F_Y[i] = 0;
        }
        barier();
    }
    return NULL;
}


int main(int argc , char** argv){
    g = 6.67430 * pow(10.0, -11.0);
    long t_end = strtol(argv[1] , NULL, 10);
    step = (double)t_end/part;
    char* file_name = argv[2];

    FILE* file = fopen(file_name, "r");
    if (file == NULL){
        perror("Error with file");
        return 1;
    }
    fscanf(file, "%ld", &n);
    m = malloc(n * sizeof(double));
    F_X = calloc(n, sizeof(double));
    F_Y = calloc(n, sizeof(double));
    X = calloc(part + 1, sizeof(double*));
    Y = calloc(part + 1, sizeof(double*));
    U_X = calloc(part + 1, sizeof(double*));
    U_Y = calloc(part + 1, sizeof(double*));
    F_Y_local = calloc(thread_count, sizeof(double*));
    F_X_local = calloc(thread_count, sizeof(double*));
    for (long t = 0; t < part + 1; t++) {
        X[t]   = calloc(n, sizeof(double));
        Y[t]   = calloc(n, sizeof(double));
        U_X[t] = calloc(n, sizeof(double));
        U_Y[t] = calloc(n, sizeof(double));
    }
    for (long t = 0; t < thread_count; t++) {
        F_X_local[t] = calloc(n, sizeof(double));
        F_Y_local[t] = calloc(n, sizeof(double));
    }
    
    for (long i=0; i<n; i++){
        fscanf(file, "%lf %lf %lf %lf %lf", &m[i], &X[0][i], &Y[0][i], &U_X[0][i], &U_Y[0][i]);
    }
    fclose(file);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&mutex, &attr);
    pthread_cond_init(&cond, NULL);
    pthread_t * threads = malloc(thread_count * sizeof(pthread_t));


    for (long i=0; i < thread_count; i++){
        int error = pthread_create(&threads[i], NULL, routine, (void*) i);
        if (error != 0)
            perror("Error");
    }
    
    for (long i=0; i < thread_count; i++){
        pthread_join(threads[i], NULL);
    }

    FILE *ans;
    ans = fopen("answer_task2.csv", "w");
    if (ans == NULL) {
        perror("Неудалось создать файл");
        return 1;
    }
    fprintf(ans, "t");
    for (long i=0; i<n; i++){
        fprintf(ans, ",x%ld,y%ld", i+1, i+1);
    }
    for (int t=0; t < part + 1; t++){
        fprintf(ans, "\n%lf,", t * step);
        for (long i=0; i < n; i++){
            if (i != n-1)
                fprintf(ans, "%lf,%lf,", X[t][i], Y[t][i]);
            else
                fprintf(ans, "%lf,%lf", X[t][i], Y[t][i]);
        }
    }

    fclose(ans);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    pthread_mutexattr_destroy(&attr);
    free(threads);
    free(m);
    for (long t = 0; t < part + 1; t++) {
        free(X[t]);
        free(Y[t]);
        free(U_X[t]);
        free(U_Y[t]);
    }
    for (long p = 0; p < thread_count; p++) {
        free(F_X_local[p]);
        free(F_Y_local[p]);
    }
    free(F_X_local);
    free(F_Y_local);
    free(X);
    free(Y);
    free(U_X);
    free(U_Y);
    free(F_X);
    free(F_Y);
    return 0;
}