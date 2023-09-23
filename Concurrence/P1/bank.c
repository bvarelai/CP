// -t 3 -i 2 -a 3
// g branch eje2.

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"

#define MAX_AMOUNT 20

struct account {
    int value;
    pthread_mutex_t *mutex;
};

struct bank {
    int num_accounts;        // number of accounts
    struct account *accounts;           // balance array
};

struct args {
    int          thread_num;  // application defined thread #
    int          delay;       // delay between operations
    int          iterations;  // number of operations
    int          net_total;   // total amount deposited by this thread
    struct bank *bank;        // pointer to the bank (shared with other threads)
};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads);

void *transfer(void *ptr)
{
    struct args *args =  ptr;
    int amount, accountFrom, accountTo, balance;
    pthread_mutex_t *lockTo, *lockFrom;

    while(args->iterations--) {
      
    do {
            accountFrom = rand() % args->bank->num_accounts;
            accountTo   = rand() % args->bank->num_accounts;
        } while( accountFrom == accountTo );
        
        lockFrom    = args->bank->accounts[accountFrom].mutex;
        lockTo      = args->bank->accounts[accountTo].mutex;

        //Lock accounts
        do {
            pthread_mutex_lock(lockTo);
            if(pthread_mutex_trylock(lockFrom) != 0)
                pthread_mutex_unlock(lockTo);
            else
                break;
        } while(1);

        //Get amount to move. Module of 0 is UB (evitar valores negativos en las cuentas)
        //debe estar en la seccion critica
        amount      =
            args->bank->accounts[accountFrom].value > 0 ?
                (rand() % args->bank->accounts[accountFrom].value)
                : 0;

        printf("Thread %d moving %d from account %d to %d\n",
            args->thread_num, amount, accountFrom, accountTo);

        balance = args->bank->accounts[accountTo].value;
        if(args->delay) usleep(args->delay); // Force a context switch

        balance += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[accountTo].value = balance;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[accountFrom].value = args->bank->accounts[accountFrom].value - amount;

        pthread_mutex_unlock(lockFrom);
        pthread_mutex_unlock(lockTo);
        break;    
    }

    if(args->delay) usleep(args->delay);
    
    return NULL;
}

// Threads run on this function
void *deposit(void *ptr)
{
    struct args *args =  ptr;
    int amount, account, balance;
    pthread_mutex_t *lock;

    while(args->iterations--) {
      
        amount  = rand() % MAX_AMOUNT;
        account = rand() % args->bank->num_accounts;
        lock = args->bank->accounts[account].mutex;

        printf("Thread %d depositing %d on account %d\n",
            args->thread_num, amount, account);

        pthread_mutex_lock(lock);
        balance = args->bank->accounts[account].value;
        if(args->delay) usleep(args->delay); // Force a context switch

        balance += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[account].value = balance;
        pthread_mutex_unlock(lock);
        if(args->delay) usleep(args->delay);

        args->net_total += amount;
    }
    return NULL;
}
void *supervisor(void *ptr) {
    int i, lockedAccounts;

    struct thread_info *threads = ptr;
    struct args *args;

    //Search for the thread pointer
    for(i = 0; threads[i].args->thread_num != -1; i++);
    args = threads[i].args;

    while( args->iterations != 0 ) {
        //Lock all accounts
        for(i = 0, lockedAccounts = 0; lockedAccounts != args->bank->num_accounts; i++) {
            if(i == args->bank->num_accounts)
                i = 0;

            if(!pthread_mutex_lock(args->bank->accounts[i].mutex)) {
                lockedAccounts++;
            }
        }
    
        print_balances(args->bank, threads, args->net_total);
        for(i = 0; i < args->bank->num_accounts; i++)
            pthread_mutex_unlock( args->bank->accounts[i].mutex) ;

        if(args->delay) usleep(args->delay);
    }

    return NULL;
}

// start opt.num_threads threads running on deposit.
struct thread_info *start_threads(struct options opt, struct bank *bank)
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * (opt.num_threads + 1) );

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> net_total  = 0;
        threads[i].args -> bank       = bank;
        threads[i].args -> delay      = opt.delay;
        
        if(i==(opt.num_threads-1))
           threads[i].args -> iterations = 0;
        else
           // If quotient is 0. gets 0
           threads[i].args -> iterations = opt.iterations/(opt.num_threads-1);    

        if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }
    
    // Wait for the threads to finish
    for (i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].id, NULL);
        
    //Start supervisor thread
    threads[ opt.num_threads ].args    = malloc(sizeof(struct args));
    *(threads[ opt.num_threads ].args) = (struct args){
        .thread_num = -1,
        .net_total  = opt.num_threads, //This stores the number of threads
        .bank       = bank,
        .delay      = opt.delay,
        .iterations = 1,               //This thread will stop when iterations == 0
    };
    if (0 != pthread_create(&threads[ opt.num_threads ].id, NULL, supervisor, threads)) {
            printf("Could not create supervisor thread");
            exit(1);
    }
    
    for (i = 0; i < opt.num_threads; i++) {
        
        if(i==(opt.num_threads-1))
          threads[i].args -> iterations = 0;
        else
          // If quotient is 0. gets 0
          threads[i].args -> iterations = opt.iterations/(opt.num_threads-1);            
        
        if (0 != pthread_create(&threads[i].id, NULL, transfer, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    return threads;
}

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
    int total_deposits=0, bank_total=0;
    printf("\nNet deposits by thread\n");

    for(int i = 0; i < num_threads; i++) {
        printf("%d: %d\n", i, thrs[i].args->net_total);
        total_deposits += thrs[i].args->net_total;
    }
    printf("Total: %d\n", total_deposits);

    printf("\nAccount balance\n");
    for(int i=0; i < bank->num_accounts; i++) {
        printf("%d: %d\n", i, bank->accounts[i].value);
        bank_total += bank->accounts[i].value;
    }
    printf("Total: %d\n", bank_total);
}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct bank *bank, struct thread_info *threads) {
    //Wait for the supervisor thread
    threads[ opt.num_threads ].args->iterations = 0;
    pthread_join(threads[ opt.num_threads ].id, NULL);
    
    // Wait for the threads to finish
    for (int i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].id, NULL);

    print_balances(bank, threads, opt.num_threads);

    for (int i = 0; i <= opt.num_threads; i++) {
        free(threads[i].args);
    }
    for(int i = 0; i < opt.num_accounts; i++) {
        pthread_mutex_destroy(bank->accounts[i].mutex);
        free(bank->accounts[i].mutex);
    }

    free(threads);
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, int num_accounts) {
    bank->num_accounts = num_accounts;
    bank->accounts     = malloc(bank->num_accounts * sizeof(struct account));

    for(int i=0; i < bank->num_accounts; i++) {
        bank->accounts[i].mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(bank->accounts[i].mutex, NULL);

        bank->accounts[i].value = 0;
    }
}

int main (int argc, char **argv)
{
    struct options      opt;
    struct bank         bank;
    struct thread_info *thrs;

    srand(time(NULL));


    // Default values for the options
    opt.num_threads  = 5;
    opt.num_accounts = 10;
    opt.iterations   = 100;
    opt.delay        = 10;

    read_options(argc, argv, &opt);

    init_accounts(&bank, opt.num_accounts);

    thrs = start_threads(opt, &bank);

    wait(opt, &bank, thrs);

    return 0;
}