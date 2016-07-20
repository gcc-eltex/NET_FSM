/*
 * Модуль хранит в себе функции, вызываемые в случае ошибок, а так же при
 * запуске новых процессов и потоков
 */

#include "./../header/shared.h"

/*
 * Выводит сообщение об ошибке и завершает приложение, убивая все дочерние
 * процессы.
 *
 * @param msg сообщение, выводимое перед описанием ошибки
 */
void handler_error(char *msg)
{
    perror(msg);
    killpg(0, SIGKILL);
}

/*
 * Функция, запускаемая в потоке. Извлекает содержимое из очереди результата 
 * и направляет ответ клиентам.
 */
void handler_reply(void *argv)
{
    int           sock = *((int *)argv);  // Сокет сервера
    struct msgbuf msg_mqd;                // Извлеченные из очереди результатов
					 	                  // данные
    while (1){
        // Ждем пока нам не сигнализируют о наличии ответных данных
        sem_wait(sem_repl);
        if (msgrcv(mqd_repl, &msg_mqd, sizeof(msg_mqd), 0, 0) == -1){
            perror("msgrcv thread");
            continue;
        }

        // Направляем ответ клиенту
        if (sendto(sock, msg_mqd.msg, 10, 0, (struct sockaddr *)&(msg_mqd.addr), sizeof(msg_mqd.addr)) == -1)
            perror("sendto thread ");
        printf("Отправлено\n");
    }
}

/*
 * Функция, запускаемая в новом процессе. Извлекает заявку, формирует ответ, 
 * помещая его в очередь ответов.
 */
void handler_request(void)
{
    time_t          utime;    // Текущее время в микросекундах
    struct tm       *tmp;     // Абсолютное текущее время  
    struct msgbuf   msg_mqd;  // Данные, записываемые в очередь обработанных 
                              // заявок
    while (1){
        // Ждем пока нам не сигнализируют о новых заявках
        sem_wait(sem_recv);

        // Извлекаем новую заявку
        if (msgrcv(mqd_recv, &msg_mqd, sizeof(msg_mqd), 0, 0) == -1){
            perror("msgrcv process");
            continue;
        }

        // Формируем время в нужном формате
        utime = time(NULL);
        tmp = localtime(&utime);
        strftime(msg_mqd.msg, 10, "%H:%M:%S", tmp);

        // Помещаем в очередь с обработанными заявками
        if (msgsnd(mqd_repl, &msg_mqd, sizeof(msg_mqd), 1) == -1)
            perror("msgsnd process");
        sem_post(sem_repl);
    }
}
