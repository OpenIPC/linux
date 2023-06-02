export PATH=/opt/codesourcery/arm-2010q1/bin:./../uboot/tools:$PATH
echo $PATH
arm-none-linux-gnueabi-objdump -d head.o > head.txt
