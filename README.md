# linux_practice
練習基於一些 linux 底層的操作

## my-http-server
用 C 語言實現一個小型的接受 http request 的 server

- [x] GET
- [ ] POST
- [ ] HEAD
- [ ] DELETE
- [ ] PATCH
- [ ] OPTIONS
- [ ] PUT

## scheduling-simulation
linux 中有自己的 os 的 scheduler，我使用context switch 的system call implement 自己的 scheduler，這個程式中我模擬的是基於 priority 並同時有兩個 priority queue (low and high) 並以round robin 的規則模擬 os 的 scheduler。

## simple-pstree
linux 中有一個功能是 ps，會印出所有程式的 process 以及其子 process 相關性的訊息，這個一個就是對於 ps 的實現。
