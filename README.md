# STM32H750B-DK
 Stratify OS package for the STM32H750B-DK development board.



```
sl os.invokebootloader
sl fs.write:source=host@STM32H750B-DK/build_debug/STM32H750B-DK.bin,dest=device@/dev/ramdrive,pagesize=2048
sl task.signal:id=1,signal=CONT
```
