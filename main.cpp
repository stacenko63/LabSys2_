#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
using namespace std;

static void signal_handler(int);
void sigaction_handle(int signum, siginfo_t *info, void* ctx);
int pid1, pid2, status, result = 0, count = 1;
bool check = false;


int main( int argc, char *argv[], char *env[] ){

    srand(time(NULL));

    setlocale(LC_ALL, "RUS");



    if(signal(SIGUSR1, signal_handler) == SIG_ERR){
        cout << "Родитель: Ошибка в создании обработчика для сигнала SIGUSR1\n";
        return -1;
    }

    if(signal(SIGUSR2, signal_handler) == SIG_ERR){
        cout << "Родитель: Ошибка в создании обработчика для сигнала SIGUSR2\n";
        return -1;
    }

    pid1 = getpid();



    if( (pid2 = fork()) == 0 )
    {
        pid2 = getpid();
        cout << "Дочерний процесс: создан\n";
        cout <<  "Дочерний процесс: отправляет родительскому процессу сигнал SIGUSR1\n";
        cout << "Дочерний процесс: готов угадывать число\n";
        kill( pid1, SIGUSR1);

        for( ;; );

    }
    else
    {
        struct sigaction act, oldact;
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = sigaction_handle;
        while (true) {
            sleep(1);
            sigaction(SIGUSR1, &act, &oldact);
            if (check) break;
        }
        wait(&status);

        if( WIFEXITED( status ) )
        {
            cout << "Дочерний процесс завершился кодом " << WEXITSTATUS( status );
        }
        else
        {
            if( WIFSIGNALED( status ) )
            {
                cout << "Дочерний процесс умер на сигнале " << WTERMSIG( status );
            }
        }
    }

    return 0;
}

void sigaction_handle(int signum, siginfo_t *info, void* ctx) {


    if (signum == SIGUSR1 && info->si_pid == pid2)
    {
        if (info->si_value.sival_int == -1)
        {
            check = true;
            //kill(pid2, SIGUSR1);
        }
        else
        {
            cout << "Родительский процесс: отправленное число " << info->si_value.sival_int << "\n";
            if (info->si_value.sival_int != result) {
                kill(pid2, SIGUSR2);
            } else {
                kill(pid2, SIGUSR1);
            }
        }
    }

}

static void signal_handler(int signo){

    struct sigaction act, oldact;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = sigaction_handle;


    switch(signo){

        case SIGUSR1:
            cout << "Процесс обработки сигнала SIGUSR1\n";
            if (pid1 == getpid() && result == 0)
            {
                result = rand() % 100 + 1;
                cout << "Родительский процесс: число загадано\nЗагаданное число: " << result << "\n";
                kill(pid2, SIGUSR2);
            }
            else if (pid1 == getpid())
            {
                cout << "Родительский процесс: перехватываю сигнал от дочернего процесса\n";
                sigaction(SIGUSR1, &act, &oldact);
            }
            else if (pid2 == getpid())
            {
                cout << "Дочерний процесс: я отгадал число!\nКоличество попыток: " << count - 1 << "\n";
                sigqueue(pid1, SIGUSR1, sigval {-1});
                exit(0);
            }
            break;


        case SIGUSR2:
            cout << "Процесс обработки сигнала SIGUSR2\n";
            if (pid2 == getpid())
            {
                cout << "Дочерний процесс: пробую угадать число!\n";
                int tmp = rand() % 100 + 1;
                tmp = rand() % 100 + 1;
                cout << "Попытка " << count++ << ": " << tmp << "\n";
                sigqueue(pid1, SIGUSR1, sigval {tmp});
            }
            break;

    }
    return;
}