#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

////////////////////////////////////////////////////////

static struct timespec ts_start, ts_end;
static struct timeval tv_start, tv_end;
static clock_t c_start;

void mystopwatch_start()
{
    gettimeofday(&tv_start, NULL);
    c_start = clock();
}

void mystopwatch_stop()
{
    gettimeofday(&tv_end, NULL);

    double wall = (1000000.0 * (double)(tv_end.tv_sec - tv_start.tv_sec));
    wall += ((double)(tv_end.tv_usec - tv_start.tv_usec));

    double cpu = 1000000 * (double)(clock() - c_start) / CLOCKS_PER_SEC;

    printf("@WALL: %lf\n", wall);
    printf("@CPU: %lf\n", cpu);
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;

    // Storing start time
    clock_t start_time = clock();

    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}

////////////////////////////////////////////////////////

int np;
int part_i;
int num_threads = 2;
int *main_array, *init_array;
pthread_t *do_semigroup_thread, *do_semigroup_final_thread;
int p_index;
int base_array_size = 9;
pthread_mutex_t mutex;
pthread_t *create_array_thread;
int *x, loop;
static const int initial_values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
static const int zero_values[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

////////////////////////////////////////////////////////

void *create_array(void *x_void_ptr)
{
    int loop;
    for (loop = part_i * (num_threads); loop < (part_i * num_threads) + (num_threads); loop++)
        main_array[loop] = init_array[loop];
    return NULL;
}
void *do_semigroup(void *x_void_ptr)
{
    int *x_pt = (int *)x_void_ptr;
    int tmp[9];
    int this_base = (num_threads * part_i) + *x_pt;
    pthread_mutex_lock(&mutex);
    tmp[*x_pt] = main_array[this_base] + main_array[p_index + this_base];
    pthread_mutex_unlock(&mutex);
    delay(100);
    pthread_mutex_lock(&mutex);
    main_array[p_index + this_base] = tmp[*x_pt];
    pthread_mutex_unlock(&mutex);

    return NULL;
}

////////////////////////////////////////////////////////


int main(int argc, char **argv)
{
    if (argc > 1)
    {
        num_threads = atoi(argv[1]);
    }
    mystopwatch_start();
    init_array = (int *)malloc(base_array_size * sizeof(int));
    np = base_array_size / num_threads;

    memcpy(init_array, initial_values, sizeof(initial_values));

    main_array = (int *)malloc(base_array_size * sizeof(int));
    create_array_general(np);
    ///////////////////////////////////////////
    for (int j = 0; j < np; j++)
    {
        part_i = j;
        do_semigroup_general();
        int base = part_i * num_threads;
    }
    memcpy(init_array, zero_values, sizeof(zero_values));
    free(do_semigroup_thread);
    ///////////////////////////////////////////

    while (np > 1)
    {
        copy_main_to_init();
        np = np / num_threads;
        if (np < 1)
        {
            np = 1;
        }
        create_array_general(np);
        part_i = 0;
        do_semigroup_general();
    }
    // printf("this is final1:\n");
    // for (loop = 0; loop < 9; loop++)
    //     printf("%d ", main_array[loop]);
    // printf("\n");
    ////////////////////////////////////////////////////////
    free(main_array);
    mystopwatch_stop();

    return 0;
}

void create_array_general(int np_tmp)
{
    np = np_tmp;

    for (int j = 0; j < np; j++)
    {
        part_i = j;
        create_array_thread = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
        for (int i = 0; i < num_threads; i++)
        {
            if (pthread_create(&create_array_thread[i], NULL, create_array, &x[i]))
            {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }
        }
        for (int i = 0; i < num_threads; i++)
        {
            if (pthread_join(create_array_thread[i], NULL))
            {
                fprintf(stderr, "Error joining thread\n");
                return 2;
            }
        }
    }
    printf("this is init:\n");
    for (loop = 0; loop < 9; loop++)
        printf("%d", main_array[loop]);
    printf("\n");

    free(create_array_thread);
}

void do_semigroup_general()
{

    pthread_t *do_semigroup_thread;
    if (pthread_mutex_init(&mutex, NULL))
    {
        fprintf(stderr, "error initializing mutex");
        return 3;
    }
    do_semigroup_thread = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    p_index = 1;
    while (p_index < num_threads)
    {
        x = (int *)malloc(base_array_size * sizeof(int));
        for (int m = 0; m < num_threads - p_index; m++)
        {
            x[m] = m;
            if (pthread_create(&do_semigroup_thread[m], NULL, do_semigroup, &x[m]))
            {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }
        }

        for (int m = 0; m < num_threads - p_index; m++)
        {
            if (pthread_join(do_semigroup_thread[m], NULL))
            {
                fprintf(stderr, "Error joining thread\n");
                return 2;
            }
        }
        p_index = p_index * 2;
    }
    printf("this is final2:\n");
    for (loop = num_threads * part_i; loop < num_threads * part_i + num_threads; loop++)
        printf("%d ", main_array[loop]);
    printf("\n");
}

void make_array_zero()
{
    if ((sizeof(init_array) / sizeof(init_array[0])) % num_threads != 0)
    {
        int init_tmp = (sizeof(init_array) / sizeof(init_array[0])) % num_threads;
        for (int i = init_tmp; i < 1; i--)
        {
            init_array[i] = 0;
        }
    }
}

void copy_main_to_init()
{
    
    for (int i = 0; i < np; i++)
    {
        init_array[i] = main_array[(i * num_threads) + (num_threads - 1)];
    }
}
