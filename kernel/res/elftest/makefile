

#变量在声明时需要给予初值，而在使用时，需要给在变量名前 加上“$”符号，但最好用小括号“（）”或是大括号“{}”把变量给包括起来。如果你要使用真实的“$”字符，那么你需要用“$$”来表示。
# $@ 目标文件，$^ 所有的依赖文件，$< 第一个依赖文件
#命令Tab键开头
#.PHONY表示clean是“伪目标”
#rm前面减号的意思是，也许某些文件出现问题，但不要管，继续做后面的事。
#clean的规则不要放在文件开头，不然就会变成make的默认目标.不成文的规矩是clean从来都是放在文件的最后
#第一条规则中的目标将被确立为最终的目标
#CFLAGS C语言编译器参数
#cc来自于Unix的c语言编译器，是 c compiler 的缩写。gcc来自Linux世界，是GNU compiler collection 的缩写

#extern "C" void _init(void) { } -------》编译生成后在.init段  
#__attribute__((constructor)) void _init(void) { } -------》编译生成后在.init_array段  



objectfiles = function.o work.o main.o 

main:$(objectfiles)
	cc -o main $(objectfiles) -m32 -ldl
	
function.o:function.c
	cc -c function.c -m32
	
main.o:main.cpp
	cc -c main.cpp -m32
	
work.o:work.cpp work.h
	cc -c work.cpp -m32

test.o:test.cpp
	cc -c work.cpp -m32
test.so:test.o
	cc -shared -fPIC -o test.so test.o -m32

clean:
	rm main $(objectfiles)



