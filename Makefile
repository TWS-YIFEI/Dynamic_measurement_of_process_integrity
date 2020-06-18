CONFIG_MODULE_SIG=n
EXTRA_CFLAGS := -I$(src)

PID=66538
ADDR1=$(shell sudo cat /proc/${PID}/maps | awk 'NR==1' | awk -F '-' '{print $$1}')
ADDR2=$(shell sudo cat /proc/${PID}/maps | awk 'NR==1' | awk -F '-|| ' '{print $$2}')
ELFPATH=$(shell sudo cat /proc/${PID}/maps | awk 'NR==1' | awk -F ' ' '{print $$6}')
OPER=1

obj-m += memory_measurement.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

testload:
	sudo dmesg -C
	sudo insmod memory_measurement.ko pid=$(PID) operator=$(OPER) virstart=0x$(ADDR1) virend=0x$(ADDR2) filepath='$(ELFPATH)'
	dmesg
testunload:
	sudo rmmod memory_measurement.ko
	dmesg

#make testunload > /dev/null
