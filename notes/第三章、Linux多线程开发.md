[toc]

# lesson01-07 线程

## 一、线程概述

与进程类似，线程（thread）是允许应用程序并发执行多个任务的一种机制。一个进程可以包含多个线程。同一个程序中的所有线程均会独立执行相同程序，且共享同一份全局内存区域，包括初始化数据段、未初始化数据段，以及堆内存段。（传统意义上的 UNIX 进程只是多线程程序的一个特例，该进程只包含一个线程）

进程是 CPU 分配资源的最小单位，线程是操作系统调度执行的最小单位。

线程是轻量级的进程（LWP：Light Weight Process），在 Linux 环境下线程的本质仍是进程。查看指定进程的 LWP 号：`ps –Lf pid`

## 二、线程和进程区别

1. 进程间的信息难以共享。由于除去只读代码段外，父子进程并未共享内存，因此必须采用一些进程间通信方式，在进程间进行信息交换。

2. 调用 fork() 来创建进程的代价相对较高，即便利用写时复制技术，仍然需要复制诸如内存页表和文件描述符表之类的多种进程属性，时间开销不菲。

3. 线程之间能够方便、快速地共享信息。只需将数据复制到共享（全局或堆）变量中即可。

4. 创建线程比创建进程通常要快 10 倍甚至更多。线程间是共享虚拟地址空间的，无需采用写时复制来复制内存，也无需复制页表。

## 三、线程之间共享和非共享资源

- ***共享资源***
    - 进程 ID 和父进程 ID
    - 进程组 ID 和会话 ID
    - 用户 ID 和 用户组 ID
    - **文件描述符表**
    - **信号处置**
    - 文件系统的相关信息：文件权限掩码（umask）、当前工作目录
    - **虚拟地址空间（除栈、.text）**
- ***非共享资源***
    - 线程 ID
    - **信号掩码**
    - 线程特有数据
    - error 变量
    - 实时调度策略和优先级
    - **栈，本地变量和函数的调用链接信息**

## 四、线程操作
```c++
#include <pthread.h>

// 创建一个子线程
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg); 
    - 参数：
        - thread：传出参数，线程创建成功后，子线程的线程ID被写到该变量中。
        - attr : 设置线程的属性，一般使用默认值，NULL
        - start_routine : 函数指针，这个函数是子线程需要处理的逻辑代码
        - arg : 给第三个参数使用，传参
    - 返回值：
        成功：0
        失败：返回错误号。这个错误号和之前errno不太一样。
        获取错误号的信息：  char * strerror(int errnum);

    一般情况下,main函数所在的线程我们称之为主线程（main线程），其余创建的线程 称之为子线程

// 终止一个线程，在哪个线程中调用，就表示终止哪个线程
void pthread_exit(void *retval);
    - 参数retval: 需要传递一个指针，作为一个返回值，可以在pthread_join()中获取到。

    注意：在进程主函数main() 中调用pthread_exit()，只会使主函数所在的线程（可以说是进程的主线程）退出；而如果是return，编译器将使其调用进程退出的代码 (如_exit())，从而导致进程及其所有线程结束运行。

    所以main()中调用了pthread_exit后，导致主线程提前退出，其后的exit()无法执行了，所以要到其他线程全部执行完了，整个进程才会退出。

// 获取当前的线程的线程ID
pthread_t pthread_self(void);

// 比较两个线程ID是否相等
int pthread_equal(pthread_t t1, pthread_t t2);
    注意：不同的操作系统，pthread_t类型的实现不一样，有的是无符号的长整型，有的是使用结构体去实现的。

// 和一个已经终止的线程进行连接，回收子线程的资源
// 注意，这个函数是阻塞函数，调用一次只能回收一个子线程(同wait())。一般在主线程中使用，可以回收不相关的线程
int pthread_join(pthread_t thread, void **retval);
    - 参数：
        - thread：需要回收的子线程的ID
        - retval: 接收子线程退出时的返回值，要接受收的是一个指针，为了能传出来，传进去一个双重指针
    - 返回值：
        成功：0；失败：返回错误号

// 分离一个线程，被分离的线程在终止的时候，会自动释放资源返回给系统
int pthread_detach(pthread_t thread);
    - 参数：需要分离的线程的ID
    - 返回值：
        成功：0；失败：返回错误号
    - 注意：
        1. 不能多次分离，会产生不可预料的行为。
        2. 不能去连接一个已经分离的线程，会报错。

// 取消线程（让线程终止）
int pthread_cancel(pthread_t thread);
    取消某个线程，可以终止某个线程的运行，但是并不是立马终止，而是当子线程执行到一个取消点，线程才会终止。
    取消点：系统规定好的一些系统调用，我们可以粗略的理解为从用户区到内核区的切换，这个位置称之为取消点。
```

***总结exit、return、pthread_exit，pthread_cancel各自退出效果和pthread_join，pthread_detach的作用***
- `return`：返回到调用者那里去。注意，在主线程退出时效果与exit,_exit一样
- `pthread_exit()`：只退出当前子线程。注意：在主线程退出时，其它线程不会结束。与return一样，`pthread_exit`退出的线程也需要调用`pthread_join`去回收子线程的资源，否则服务器长时间运行会浪费资源导致无法再创建新线程
- `exit，_exit`： 将进程退出，无论哪个子线程调用整个程序都将结束
- `pthread_cancel()`：可以杀死子线程，但必须需要一个契机，这个契机就是系统调用
- `pthread_join()`：阻塞等待回收子线程
- `pthread_detach()`：注意，所有线程的错误号返回都只能使用`strerror()`这个函数判断；线程可以被置为detach状态，这样的线程一旦终止就立刻回收它占用的所有资源，而不保留终止状态。并且不能对一个已经处于detach状态的线程调用`pthread_join()`，这样的调用将返回`EINVAL`错误


## 五、线程属性
线程属性类型 `pthread_attr_t`，设置了线程属性后可以在创建线程的时候直接指定属性

```c++
// 初始化线程属性变量
int pthread_attr_init(pthread_attr_t *attr);

// 释放线程属性的资源
int pthread_attr_destroy(pthread_attr_t *attr);

// 获取线程分离的状态属性
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);

// 设置线程分离的状态属性
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

// 获取线程栈的大小
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

// 设置线程栈的大小
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

...
```

# lesson08-14 线程同步

## 一、线程同步

线程的主要优势在于，能够**通过全局变量来共享信息，因此必须确保多个线程不会同时修改同一变量，或者某一线程不会读取正在由其他线程修改的变量。**

临界区是指访问某一共享资源的代码片段，并且这段代码的执行应为原子操作，也就是同时访问同一共享资源的其他线程不应中断该片段的执行。

线程同步：即当有一个线程在对内存进行操作时，其他线程都不可以对这个内存地址进行操作，直到该线程完成操作，其他线程应处于等待状态。

## 二、互斥量

为避免线程更新共享变量时出现问题，可以使用互斥量（mutex 是 mutual exclusion的缩写）来确保同时仅有一个线程可以访问某项共享资源。

互斥量有两种状态（已锁定和未锁定）。任何时候，至多只有一个线程可以锁定该互斥量。试图对已经锁定的某一互斥量再次加锁，将可能阻塞线程或者报错失败，具体取决于加锁时使用的方法。
 
一旦线程锁定互斥量，只有所有者才能解锁。一般情况下，不同的共享资源会使用不同的互斥量，每一线程在访问同一资源时将采用如下协议：
1. 针对共享资源锁定互斥量
2. 访问共享资源
3. 对互斥量解锁

### 1、互斥量相关操作函数
```c++
pthread_mutex_t : 互斥量的类型 
restrict : C语言的修饰符，被修饰的指针，不能由另外的一个指针进行操作。
    pthread_mutex_t *restrict mutex = xxx;
    pthread_mutex_t * mutex1 = mutex; //报错，只有mutex 能对 XXX 进行操作

// 初始化互斥量
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
    - 参数 ：
        - mutex ： 需要初始化的互斥量变量
        - attr ： 互斥量相关的属性，NULL

// 释放互斥量的资源
int pthread_mutex_destroy(pthread_mutex_t *mutex);

// 加锁，阻塞的，如果有一个线程加锁了，那么其他的线程只能阻塞等待
int pthread_mutex_lock(pthread_mutex_t *mutex);

// 尝试加锁，如果加锁失败，不会阻塞，会直接返回。
int pthread_mutex_trylock(pthread_mutex_t *mutex);

// 解锁
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```
### 2、生产者消费者模型（粗略版本）

```c++
pthread_mutex_t mutex; // 创建一个互斥量
struct Node{
    int num;
    struct Node *next;
};

struct Node * head = NULL; // 头结点

void * producer(void * arg) {
    while(1) { // 不断的创建新的节点，添加到链表中
        pthread_mutex_lock(&mutex);
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->next = head;
        head = newNode;
        newNode->num = rand() % 1000;
        printf("add node, num : %d, tid : %ld\n", newNode->num, pthread_self());
        pthread_mutex_unlock(&mutex);
        usleep(100);
    }
    return NULL;
}

void * customer(void * arg) {
    while(1) {
        pthread_mutex_lock(&mutex);
        // 保存头结点的指针
        struct Node * tmp = head;
        // 判断是否有数据
        if(head != NULL) {
            head = head->next;
            printf("del node, num : %d, tid : %ld\n", tmp->num, pthread_self());
            free(tmp);
            pthread_mutex_unlock(&mutex);
            usleep(100);
        } 
        else pthread_mutex_unlock(&mutex);
    }
    return  NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_t ptids[5], ctids[5]; // 创建5个生产者线程，和5个消费者线程
    for(int i = 0; i < 5; i++) {
        pthread_create(&ptids[i], NULL, producer, NULL);
        pthread_create(&ctids[i], NULL, customer, NULL);
    }

    for(int i = 0; i < 5; i++) {
        pthread_detach(ptids[i]);
        pthread_detach(ctids[i]);
    }
    while(1) sleep(10); //没有它的话，这个锁就被直接销毁了
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
    return 0;
}
```

## 三、死锁

有时，一个线程需要同时访问两个或更多不同的共享资源，而每个资源又都由不同的互斥量管理。当超过一个线程加锁同一组互斥量时，就有可能发生死锁，即因争夺共享资源而造成的一种互相等待的现象。

死锁的几种场景：
1. 忘记释放锁
2. 重复加锁
3. 多线程多锁，抢占锁资源

## 四、读写锁

当有一个线程已经持有互斥锁时，互斥锁将所有试图进入临界区的线程都阻塞住，但实际上多个线程同时读访问共享资源并不会导致问题。在对数据的读写操作中，更多的是读操作，写操作较少，为了满足当前能够允许多个读出，但只允许一个写入的需求，线程提供了读写锁

读写锁的特点：
1. 如果有其它线程读数据，则允许其它线程执行读操作，但不允许写操作。
2. 如果有其它线程写数据，则其它线程都不允许读、写操作。
3. **写是独占的，写的优先级高**。

### 1、读写锁相关操作函数

同互斥量，只是将mutex改成了rwlock

```c++
读写锁的类型 pthread_rwlock_t

int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock, const pthread_rwlockattr_t *restrict attr);

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
```
### 2、读写锁案例

描述：8个线程操作同一个全局变量，3个线程不定时写这个全局变量，5个线程不定时的读这个全局变量

```c++
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int num = 1; // 创建一个共享数据
pthread_rwlock_t rwlock;

void * writeNum(void * arg) {
    while(1) {
        pthread_rwlock_wrlock(&rwlock);
        num++;
        printf("++write, tid : %ld, num : %d\n", pthread_self(), num);
        pthread_rwlock_unlock(&rwlock);
        usleep(100);
    }
    return NULL;
}

void * readNum(void * arg) {
    while(1) {
        pthread_rwlock_rdlock(&rwlock);
        printf("===read, tid : %ld, num : %d\n", pthread_self(), num);
        pthread_rwlock_unlock(&rwlock);
        usleep(100);
    }
    return NULL;
}

int main() {
    pthread_rwlock_init(&rwlock, NULL);
    // 创建3个写线程，5个读线程
    pthread_t wtids[3], rtids[5];
    for(int i = 0; i < 3; i++) pthread_create(&wtids[i], NULL, writeNum, NULL);
    for(int i = 0; i < 5; i++) pthread_create(&rtids[i], NULL, readNum, NULL);
    // 设置线程分离
    for(int i = 0; i < 3; i++) pthread_detach(wtids[i]);
    for(int i = 0; i < 5; i++) pthread_detach(rtids[i]);
    pthread_exit(NULL);
    pthread_rwlock_destroy(&rwlock);
    return 0;
}
```
## 五、条件变量

条件变量的类型 `pthread_cond_t`，条件变量不是锁，但可以让某个线程在满足一定条件下时阻塞或解除阻塞

### 1、条件变量相关函数

```c++
int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);

int pthread_cond_destroy(pthread_cond_t *cond);

// 线程缺乏所需资源，进入等待状态。调用了该函数，线程会阻塞
// 在函数内部会先解锁，以便其他线程继续其操作，待满足条件后通知该阻塞的线程解除阻塞。解除阻塞时又会重新加上原来的锁
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);

// 调用了这个函数，线程会阻塞，直到指定的时间结束。
int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);

// 唤醒一个或者多个等待的线程
int pthread_cond_signal(pthread_cond_t *cond);

// 唤醒所有的等待的线程
int pthread_cond_broadcast(pthread_cond_t *cond);
```

### 2、生产者消费者条件变量版
```c++
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex; // 创建一个互斥量
pthread_cond_t cond; // 创建条件变量

struct Node{
    int num;
    struct Node *next;
};

struct Node * head = NULL; // 头结点

void * producer(void * arg) {
    while(1) { // 不断的创建新的节点，添加到链表中
        pthread_mutex_lock(&mutex);
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->next = head;
        head = newNode;
        newNode->num = rand() % 1000;
        printf("add node, num : %d, tid : %ld\n", newNode->num, pthread_self());
        // 只要生产了一个，就通知消费者消费
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        usleep(100);
    }
    return NULL;
}

void * customer(void * arg) {
    while(1) {
        pthread_mutex_lock(&mutex);
        struct Node * tmp = head;
        if(head != NULL) {
            head = head->next;
            printf("del node, num : %d, tid : %ld\n", tmp->num, pthread_self());
            free(tmp);
            pthread_mutex_unlock(&mutex);
            usleep(100);
        } else {
            // 没有数据，需要等待
            // 当这个函数调用阻塞的时候，会对互斥锁进行解锁，当不阻塞时，继续向下执行，会重新加锁。
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
        }
    }
    return  NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_t ptids[5], ctids[5]; // 创建5个生产者线程，和5个消费者线程
    for(int i = 0; i < 5; i++) {
        pthread_create(&ptids[i], NULL, producer, NULL);
        pthread_create(&ctids[i], NULL, customer, NULL);
    }
    for(int i = 0; i < 5; i++) {
        pthread_detach(ptids[i]);
        pthread_detach(ctids[i]);
    }
    while(1) sleep(10);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    pthread_exit(NULL);
    return 0;
}
```

## 六、信号量

信号量的类型 `sem_t`，又名信号灯，共有两种状态，灯亮表示资源可用，灯灭表示资源不可用。也是用于阻塞线程的，同样不能保证多线程数据安全问题

### 1、信号量相关函数
```c++
// 初始化信号量
int sem_init(sem_t *sem, int pshared, unsigned int value);
    - 参数：
        - sem : 信号量变量的地址
        - pshared : 0 用在线程间 ，非0 用在进程间
        - value : 信号量中的值 (类似有几盏灯)

// 释放资源
int sem_destroy(sem_t *sem);

// 对信号量加锁，调用一次信号量的值 -1。若值为 0 ，就阻塞 (类似灭一盏灯)
int sem_wait(sem_t *sem);

int sem_trywait(sem_t *sem);

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);

// 对信号量解锁，调用一次信号量的值 +1 (类似亮一盏灯)
int sem_post(sem_t *sem);

int sem_getvalue(sem_t *sem, int *sval);
```

### 2、生产者消费者的信号量版

大体流程为利用两个信号量来限制当前还能生产几个产品(psem，即几个生产线程可以工作)以及当前有几个产品已经生产出来了(csem，即几个消费线程可以工作)

生产者：
1. `sem_wait(&psem);`
2. `sem_post(&csem);`

消费者：
1. `sem_wait(&csem);`
2. `sem_post(&psem);`

```c++
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

pthread_mutex_t mutex;

// 创建两个信号量
sem_t psem;
sem_t csem;

struct Node{
    int num;
    struct Node *next;
};

struct Node * head = NULL;

void * producer(void * arg) {
    while(1) {
        sem_wait(&psem);
        pthread_mutex_lock(&mutex);
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->next = head;
        head = newNode;
        newNode->num = rand() % 1000;
        printf("add node, num : %d, tid : %ld\n", newNode->num, pthread_self());
        pthread_mutex_unlock(&mutex);
        sem_post(&csem);
    }
    return NULL;
}

void * customer(void * arg) {
    while(1) {
        sem_wait(&csem);
        pthread_mutex_lock(&mutex);
        struct Node * tmp = head;
        head = head->next;
        printf("del node, num : %d, tid : %ld\n", tmp->num, pthread_self());
        free(tmp);
        pthread_mutex_unlock(&mutex);
        sem_post(&psem);
    }
    return  NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    sem_init(&psem, 0, 8); //信号量的值表示还可以生产几个产品
    sem_init(&csem, 0, 0); //信号量的值表示有几个产品可以消费
    pthread_t ptids[5], ctids[5];
    for(int i = 0; i < 5; i++) {
        pthread_create(&ptids[i], NULL, producer, NULL);
        pthread_create(&ctids[i], NULL, customer, NULL);
    }
    for(int i = 0; i < 5; i++) {
        pthread_detach(ptids[i]);
        pthread_detach(ctids[i]);
    }
    while(1) sleep(10);
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
    return 0;
}
```