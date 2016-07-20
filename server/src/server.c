#include "./../header/shared.h"

int main(int argc, void *argv[])
{
    int                 sock;       	// Сокет сервера для заявок
    struct sockaddr_in  addr_srv;   	// Данные сервера
    socklen_t           addr_size;  	// Размер структуры struct sockaddr_in
    enum scond          cond;       	// Текущее состояние сервера
    pthread_t           tid;        	// Идентификатор созданного потока
    key_t               key;        	// Ключ для создания семафора
    char                msg_err[64];	// Сообщение выводимое при ошибке
    char                msg_tmp[64];	// Буфер полученных данных
    struct msgbuf       msg_mqd;    	// Сформированное сообщение для
                                    	// передачи в очередь 

    // Создание обьектов IPC. 54 55 - Просто случайные ключи
    mqd_recv = msgget(ftok(argv[0], 55), IPC_CREAT|0777);
    if (mqd_recv == -1)
        handler_error("msgget recv");
    mqd_repl = msgget(ftok(argv[0], 55), IPC_CREAT|0777);
    if (mqd_repl == -1)
        handler_error("msgget repl");
    /*
     * Начальное состояние семафоров устанавливаем в заблокированное(0),
     * что говорит об отсутствии заявок и ответов и не дает обрабатывающим
     * потокам и процессам манипулировать очередями.
     */
    sem_recv = sem_open(SRECV_NAME, O_RDWR|O_CREAT, 0777, 0);
	if (sem_recv == SEM_FAILED)
         handler_error("sem_open recv");
    sem_repl = sem_open(SREPL_NAME, O_RDWR|O_CREAT, 0777, 0);
	if (sem_repl == SEM_FAILED)
         handler_error("sem_open recv");
    
    // Создание и именование сокета
    addr_size = sizeof(struct sockaddr_in);
    addr_srv.sin_family = AF_INET;
    addr_srv.sin_port = htons(SRV_PORT);
    addr_srv.sin_addr.s_addr = inet_addr(SRV_ADDR);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        handler_error("socket");
    if (bind(sock, (struct sockaddr *)&addr_srv, addr_size) == -1)
        handler_error("bind");

    // Запуск отвечающих потоков
    for (int i = 0; i < CNT_THRD; i++){
        if (pthread_create(&tid, NULL, (void *)handler_reply, (void *)&sock) == -1)
            handler_error("pthread_create");
    }
    
    /*
     * Создаем новую группу, чтобы при завершении можно было послать сигнал
     * всей группе разом и создаем обрабатывающие процессы
     */
    setpgrp();
    for (int i = 0; i < CNT_PROC; i++){
        switch (fork()){
            case 0:
                handler_request();
            break;

            case -1:
                handler_error("fork");
            break;
        }
    }

    // Основной блок с переходами состояний
    cond = COND_READY;
    msg_mqd.prio = 1l;
    while (1){
            switch (cond){
                case COND_READY:
                    if (recvfrom(sock, msg_tmp, 64, 0, (struct sockaddr *)&(msg_mqd.addr), &addr_size) == -1){
                        strcpy(msg_err, "recvfrom");
                        cond = COND_ERROR;	
                        continue;
                    }
                    printf("get: %s\n", msg_tmp);
                    cond = COND_RECEIVED;
                break;
    
                case COND_RECEIVED:
                    if (msgsnd(mqd_recv, &msg_mqd, sizeof(struct msgbuf), 0) == -1){
                        strcpy(msg_err, "mq_send");
                        cond = COND_ERROR;
                        continue;
                    }
                    cond = COND_MSGPLACED;
                break;

                case COND_MSGPLACED:
                    if (sem_post(sem_recv) == -1){
                        strcpy(msg_err, "sem_post");
                        cond = COND_ERROR;
                        continue;
                    }
                    cond = COND_READY;
                break;

                case COND_ERROR:
                    //Удаляем все созданные обьекты
                    sem_unlink(SRECV_NAME);
                    sem_unlink(SREPL_NAME);
                    close(sock);
                    handler_error(msg_err);
                break;
            }
    }
}
