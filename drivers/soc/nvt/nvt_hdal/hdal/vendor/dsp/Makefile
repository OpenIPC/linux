DRV_BUILD_PATH = ./drivers/source
LIB_BUILD_PATH = ./source

all:
	@$(MAKE) -C $(DRV_BUILD_PATH)
	@$(MAKE) -C $(LIB_BUILD_PATH)

install:
	@$(MAKE) -C $(DRV_BUILD_PATH) modules_install
	@$(MAKE) -C $(LIB_BUILD_PATH) install

clean:
	@$(MAKE) -C $(DRV_BUILD_PATH) clean
	@$(MAKE) -C $(LIB_BUILD_PATH) clean
codesize:
	@$(MAKE) -C $(DRV_BUILD_PATH) codesize
	@$(MAKE) -C $(LIB_BUILD_PATH) codesize