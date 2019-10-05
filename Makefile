include defines.mk

VERS := (($(MAJOR) * 255 + $(MINOR)) * 255 + $(CHG_H)) * 255 + $(CHG_L)
# quite compile.
Q = @

CC = gcc
MV = mv
#CC = aarch64-linux-gnu-gcc

OUTPUT_EXE = sk_client
OUTPUT_DIR = output

# 头文件所在地址
INCLUDE := -I ./inc/

# 库文件存放地址
LIB := -L ./lib

# CFLAGS
CFLAGS := -g -Wall -DSKHL_DEBUG $(LIB) -lpthread -lqxwz -lm -DVERS $(INCLUDE)

#CFLAGS += -DUSR_APP_KEY=\"$(USER_CONFIG_APP_KEY)\" \
#-DUSR_APP_SECRET=\"$(USER_CONFIG_APP_SECRET)\" \
#-DUSR_DEV_ID=\"$(USER_CONFIG_DEV_ID)\" \
#-DUSR_DEV_TYPE=\"$(USER_CONFIG_DEV_TYPE)\"

# 通过find命令，找到所有的.c文件
CSources := $(shell find src/ -name "*.c") 


Objs := $(CSources:.c=.o)

CLEAN_FILE = $(wildcard output/)

$(OUTPUT_EXE):$(Objs)
	$(Q) $(CC) $^ -o $@ $(CFLAGS)
	$(Q) test -d $(OUTPUT_DIR) || mkdir -p $(OUTPUT_DIR)
	$(Q) $(MV) src/*.o $(OUTPUT_DIR)
	$(Q) echo "Make $(OUTPUT_EXE) done!"

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@ 

.PHONY : clean

test:
	@echo version : $(VERS)

clean:
	$(Q) -rm src/*.o
	$(Q) -rm -rf $(CLEAN_FILE)
	$(Q) echo "Make clean done!"
