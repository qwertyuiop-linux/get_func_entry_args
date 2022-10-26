# 编译命令

`make -C ../kernel/linux-4.14.7/ M=/home/qc/get_func_entry_args CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm modules`

如果出现没有权限错误，make前面加上sudo。

# 加载命令

```bash
insmod get_func_entry_args.ko symbol_name=__do_fork
```

# 内核配置

`CONFIG_KPROBES=y`需要配置

# 可能遇到问题

- 输入内核符号，register_kprobe注册失败。可以通过`cat /proc/kallsyms | grep 符号名称`查看内核符号表里的符号名称。

例如在某些设备上加载send_signal函数失败。

```bash
insmod get_func_entry_args.ko symbol_name=send_signal
register_kprobe failed, return -2
insmod: can't insert 'get_func_entry_args.ko': unknown symbol in module or invalid parameter
```

查看send_signal相关符号：

```bash
cat /proc/kallsyms | grep send_signal
8012a4b4 t __send_signal.constprop.0
```

加载得到的符号：

```bash
insmod get_func_entry_args.ko symbol_name=__send_signal.constprop.0
```





