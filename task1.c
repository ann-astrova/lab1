#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

long thread_count;
long point_count;
long find = 0;
double (*ans)[2];
pthread_mutex_t mutex;


bool mandel(double c, double ci){
    double z = 0.0;
    double zi = 0.0;
    for (int i = 0; i < 100000; i++){
        double zn = z * z - zi * zi + c;
        double zni = 2 * zi * z + ci;
        if (zn * zn +  zni * zni > 4.0){
            return false;
        }
        else{
            z = zn;
            zi = zni;
        }
    }
    return true;

}


void* routine(void* rank){
    long my_rank = (long)rank;
    double boarder = 2.0 / thread_count;

    for(double c=-2.0; c < 1.0; c+=0.001){
        for (double ci=-1 + boarder * my_rank; ci < -1 + boarder * (my_rank + 1); ci+= boarder/1000){
            if (mandel(c, ci) == true){
                pthread_mutex_lock(&mutex);
                if (find < point_count){
                    ans[find][0] = c;
                    ans[find][1] = ci;
                    find += 1;
                }
                bool cond = (find >= point_count);
                pthread_mutex_unlock(&mutex);

                if (cond == true)
                    return NULL;
            }
        }
    }
    return NULL;
}


int main(int argc , char** argv){
    thread_count = strtol(argv[1] , NULL, 10);
    point_count = strtol(argv[2] , NULL, 10);
    ans = calloc(point_count, sizeof(*ans));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&mutex, &attr);

    pthread_t * threads = malloc(thread_count * sizeof(pthread_t));
    
    for (long i=0; i < thread_count; i++){
        int error = pthread_create(&threads[i], NULL, routine, (void*)(long) i);
        if (error != 0){
            perror("Error");
            return 1;
        }
    }
    
    for (long i=0; i < thread_count; i++){
        pthread_join(threads[i], NULL);
    }

    FILE *answer;
    answer = fopen("answer_task1.csv", "w");
    if (answer == NULL) {
        perror("Не удалось создать файл");
        return 1;
    }
    fprintf(answer, "r,i");

    for (long i=0; i < find; i++){
        fprintf(answer, "\n%lf,%lf", ans[i][0],  ans[i][1]);

    }
    fclose(answer);
    free(threads);
    free(ans);
    pthread_mutexattr_destroy(&attr);
    pthread_mutex_destroy(&mutex);
    return 0;
}
