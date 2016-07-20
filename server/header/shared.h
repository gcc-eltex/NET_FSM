#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SRV_PORT    2951            // Порт сервера
#define SRV_ADDR    "127.0.0.1"     // IP адрес сервера
#define CNT_THRD    2               // Количество отвечающих потоков
#define CNT_PROC    2               // Количество обрабатывающих процессов
#define SRECV_NAME  "/ts-sem-recv1"  // Имя семафора для сонхронизации доступа к
                                    // очереди заявок 
#define SREPL_NAME  "/ts-sem-repl1"  // Имя семафора для сонхронизации доступа к
                                    // очереди ответов 

/*
 * Перечисление возможных состояний сервера.
 * Возможены следующие переходы:
 *
 *   |------> ERROR <-------|
 *   |          ^           |
 *   |          |           |
 * READY -> RECEIVED -> PROCESSING
 *   ^                      |
 *   |----------------------|
 */
enum scond
{
    COND_READY,      // Готов принять новую заявку
    COND_RECEIVED,   // Заявка получена
    COND_MSGPLACED,  // Заявка размещена в очереди
    COND_ERROR       // Ошибка
};

/*
 * Структура, reply_data хранит результат обработки заявки и данные клиента,
 * которому направить этот ответ 
 */
struct msgbuf
{
    long prio;                     // Приоритет
    struct sockaddr_in  addr;      // Данные клиента
    char                msg[10];   // Результат обработки заявки
};

int mqd_recv;     // Очередь с заявками клиентов
int mqd_repl;     // Очередь с ответами на заявки
sem_t *sem_recv;  // Семафор, синхронизирующий доступ к очереди заявок для 
                  // обрабатывающийх их процессов.
sem_t *sem_repl;  // Семафор, синхронизирующий доступ к очереди с результатами
                  // обработки для отвечающих потоков.

// Модуль handler.c
void handler_error(char *msg);
void handler_reply(void *sock);
void handler_request(void);
