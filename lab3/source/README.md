#Lab3

##请注意：

如果在函数开头的定义变量部分出现类似`int a; int b = a;`的代码，程序一定会出错。这是因为作为自底向上SDT在对`int b = a;`进行类型检查的时候，`a`尚未插入符号表，所以`b`的类型检查因为找不到`a`而一定会错并导致`b`同样无法插入符号表。解决方法是先`int b;`，然后在声明部分结束后再进行赋值`b=a;`。万望助教发现有这种情况的时候改一下测试用例。谢谢助教！

输入 `make parser` 来编译 **parser**。

然后输入 `./parser 文件名` 以获得中间代码，中间代码会被存储在 **code.ir** 文件中。

输入 `bash irsim.sh` 来启动irsim。

虽然这次的程序吸取了lab2的教训，经过了一些包括快排在内的测试，但仍会有一些问题，如果发生问题请联系mattzhang9@gmail.com。 



Input `make parser` to compile the **parser**.

Then input `./parser filename` to generate the ir code. The result is saved in **code.ir**

Input `bash irsim.sh` to run the irsim.

Although the parser has went through some basic tests and even the quicksort, in some special cases there will also be something wrong. If any problem occurs, please contact mattzhang9@gmail.com. 
