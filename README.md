Сделана домашняя работа с POSIX threads. 100 потоков-источников генерируют числа от 1 до 100 с задержкой 1 - 7 секунд и кладут в буфер. Монитор следит за буфером, как только есть два числа, создаёт поток-сумматор, который складывает их с задержкой 3 - 6 секунд и возвращает результат в буфер. Так работают несколько сумматоров одновременно.

В консоли видно, кто что положил в буфер, какие числа монитор взял, какие суммы получились. В итоге остаётся одно число - финальный результат. При каждом запуске порядок суммирования разный, но логика выполняется правильно.
 

Пример вывода (конец консоли)

```
[Monitor] picked 80 and 144 from buffer. Buffer size: 1
[Sum thread] 28 + 9 = 37. Buffer size: 2
[Sum thread] 76 + 73 = 149. Buffer size: 3
[Monitor] picked 149 and 37 from buffer. Buffer size: 1
[Sum thread] 95 + 56 = 151. Buffer size: 2
[Monitor] picked 151 and 108 from buffer. Buffer size: 0
[Sum thread] 107 + 69 = 176. Buffer size: 1
[Main] Final result: 176
[Sum thread] 15 + 5 = 20. Buffer size: 2
[Sum thread] 67 + 53 = 120. Buffer size: 3
[Monitor] picked 120 and 0 from buffer. Buffer size: 1
```
