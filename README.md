
# User
Пользовательский процесс иммитирует работу двух читателей, которые могут иметь одновременный доступ к ресурсу и одного писателя, который должен иметь единоличное владение ресурсом. Если я пользуюсь функцией *sleep*, то не удается построить флеймграф - большую часть времени моя программа просто спит в ожидании и в запись не попадает никаких событий, поэтому я взял *spinlock_s* - он попадает в записи перфа.
Чтобы построить флеймграф для user_rw я собрал и запустил программу
```
make check-user
```
Затем нашел её PID и запустил с ним запись perf
```
ps -aux | grep user_rw
kent       54438  0.1  0.0  92636   536 pts/4    Sl+  16:17   0:00 ./user_rw
sudo perf record -F 99 -g -p 54438
```
через 10 минут я прервал программы и с помощью репозитория **https://github.com/brendangregg/FlameGraph** построил флеймграф для программы
`sudo perf script -i ../HW_04_kthread/perf.data | ./stackcollapse-perf.pl | ./flamegraph.pl > ../HW_04_kthread/user.svg`
и получил *user.svg* 
![user.svg](https://github.com/CaptainProfit/HW_04_kthread/blob/master/user.svg)


# Kernel
Чтобы запустить вариант такой же программы в ядре надо выполнить `make check-kernel` и запустить запись перфа, но на этот раз процессы которые я ищу называются `my_reader1/2/3` и `my_writer1`. Вот команда :
```
sudo perf record -F 99 -g -p `ps -aux | grep -E "(my_reader|my_writer)" | grep -v grep | awk '{print $2}' | tr ' ' ','`
```
или в ручном режиме
```
ps -aux
root        1868 51.2  0.0      0     0 ?        D    18:35   0:12 [my_reader1]
root        1869 51.2  0.0      0     0 ?        D    18:35   0:12 [my_reader2]
root        1870 52.4  0.0      0     0 ?        R    18:35   0:12 [my_writer1]
sudo perf record -F 99 -g -p 1868,1869,1870
```
Теперь нужно предобработать результат на виртуальной машине
`perf script > kernel.scr`
 затем копирую в результат на свою машину
`scp otus2:~/hw4/kernel.scr .`
и строю флеймграф для по ней:
`cat ../HW_04_kthread/kernel.scr | ./stackcollapse-perf.pl | ./flamegraph.pl > ../HW_04_kthread/kernel.svg`

![kernel.svg](https://github.com/CaptainProfit/HW_04_kthread/blob/master/kernel.svg)

В результате у меня получилось два графика работы с процессами - в ядерном и пользовательском пространстве.

В итоге в рамках работы я запустил потоки и воспользовался 
