#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
using namespace std;

static void signal_handler(int);
void sigaction_handle(int signum, siginfo_t *info, void* ctx);
int pid1, pid2, status, result = 0, count = 1, module = 10;
bool check = false, isParentWish = true;
clock_t t;


int main(int argc, char *argv[], char *env[]){

    srand(11 + time(NULL));

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

    pid2 = fork();

    while (true) {


        if (isParentWish) {

            cout << "Начало игры! Загадывает родительский процесс, угадывает дочерний процесс\n\n";

            if (pid2 == 0) {
                srand(time(NULL));
                pid2 = getpid();
                t = clock();
                cout << "Дочерний процесс: создан\n";
                cout << "Дочерний процесс: отправляет родительскому процессу сигнал SIGUSR1\n";
                cout << "Дочерний процесс: готов угадывать число\n";
                kill(pid1, SIGUSR1);

                for (;;);

            } else {
                struct sigaction act, oldact;
                act.sa_flags = SA_SIGINFO;
                act.sa_sigaction = sigaction_handle;
                while (true) {
                    sleep(1);
                    sigaction(SIGUSR1, &act, &oldact);
                    if (check) break;
                }
                //cout << "Начало игры! Загадывает дочерний процесс, угадывает родительский процесс\n\n";
            }
        }
        else {
            cout << "Начало игры! Загадывает дочерний процесс, угадывает родительский процесс\n\n";

            t = clock();

            if (pid2 == 0) {

                struct sigaction act, oldact;
                act.sa_flags = SA_SIGINFO;
                act.sa_sigaction = sigaction_handle;

                srand(time(NULL));
                pid2 = getpid();
                cout << "Дочерний процесс: создан\n";
                cout << "Дочерний процесс: загадывает число\n";
                result = rand() % 10 + 1;
                cout << "Дочерний процесс: загаданное число: " << result << "\n";
                sleep(1);
                kill(pid1, SIGUSR2);


                while (true) {
                    sigaction(SIGUSR1, &act, &oldact);
                    if (check) break;
                }

            }
            else {
                wait(&status);

            }
        }


        result = 0;
        count = 1;
        t = 0;
        isParentWish = isParentWish ? false : true;
        check = false;
        break;
    }
}

void sigaction_handle(int signum, siginfo_t *info, void* ctx) {

    if (signum == SIGUSR1 && info->si_pid == pid2)
    {
        if (info->si_value.sival_int == -1)
        {
            check = true;
            exit(0);
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
    else if (signum == SIGUSR1 && info->si_pid == pid1)
    {
        if (info->si_value.sival_int == -1)
        {
            check = true;
        }
        else {
            cout << "Дочерний процесс: отправленное число " << info->si_value.sival_int << "\n";
            if (info->si_value.sival_int != result) {
                kill(pid1, SIGUSR2);
            } else {
                kill(pid1, SIGUSR1);
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
            if (isParentWish) {
                if (pid1 == getpid() && result == 0) {
                    result = rand() % module + 1;
                    cout << "Родительский процесс: число загадано\nЗагаданное число: " << result << "\n";
                    kill(pid2, SIGUSR2);
                } else if (pid1 == getpid()) {
                    cout << "Родительский процесс: перехватываю сигнал от дочернего процесса\n";
                    sigaction(SIGUSR1, &act, &oldact);
                } else if (pid2 == getpid()) {
                    t = clock() - t;
                    cout << "Дочерний процесс: я отгадал число!\nКоличество попыток: " << count - 1
                         << "\nЗатраченное время: " << (double) t / CLOCKS_PER_SEC << "\n";
                    sigqueue(pid1, SIGUSR1, sigval{-1});

                }
            }
            else {
                if (pid1 == getpid()) {
                    t = clock() - t;
                    cout << "Родительский процесс: я отгадал число!\nКоличество попыток: " << count - 1 << "\nЗатраченное время: " << (double) t / CLOCKS_PER_SEC << "\n";
                    sigqueue(pid2, SIGUSR1, sigval{-1});
                }
            }
            break;


        case SIGUSR2:
            cout << "Процесс обработки сигнала SIGUSR2\n";
            if (isParentWish) {
                if (pid2 == getpid()) {
                    cout << "Дочерний процесс: пробую угадать число!\n";
                    int tmp = rand() % module + 1;
                    cout << "Попытка " << count++ << ": " << tmp << "\n";
                    sigqueue(pid1, SIGUSR1, sigval{tmp});
                }
            }
            else {
                if (pid1 == getpid()) {
                    t = clock();
                    cout << "Родительский процесс: пробую угадать число!\n";
                    int tmp = rand() % module + 1;
                    cout << "Попытка " << count++ << ": " << tmp << "\n";
                    sigqueue(pid2, SIGUSR1, sigval{tmp});
                }
            }
            break;

    }
    return;
}