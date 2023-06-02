LIB_BUILD_PATH = ./source
DRV_BUILD_PATH = ./drivers/source

all:
	@$(MAKE) -C $(LIB_BUILD_PATH)
	@$(MAKE) -C $(DRV_BUILD_PATH)

install:
	@$(MAKE) -C $(LIB_BUILD_PATH) install
	@$(MAKE) -C $(DRV_BUILD_PATH) modules_install

clean:
	@$(MAKE) -C $(LIB_BUILD_PATH) clean
	@$(MAKE) -C $(DRV_BUILD_PATH) clean
codesize:
	@$(MAKE) -C $(DRV_BUILD_PATH) codesize
