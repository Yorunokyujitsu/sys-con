.PHONY: all build clean mrproper dist distclean

GIT_TAG := $(shell git describe --tags `git rev-list --tags --max-count=1`)
GIT_TAG_COMMIT_COUNT := +$(shell git rev-list  `git rev-list --tags --no-walk --max-count=1`..HEAD --count)
ifeq ($(GIT_TAG_COMMIT_COUNT),+0)
	GIT_TAG_COMMIT_COUNT := 
endif

ATMOSPHERE_VERSION	?= 1.7.x
SOURCE_DIR			:= source
OUT_DIR				:= out
DIST_DIR			:= dist
OUT_ZIP				:= sys-con-$(GIT_TAG)$(GIT_TAG_COMMIT_COUNT)-ATMOSPHERE-$(ATMOSPHERE_VERSION).zip

all: build
	rm -rf $(OUT_DIR)
	mkdir -p $(OUT_DIR)/atmosphere/contents/690000000000000D/flags
	mkdir -p $(OUT_DIR)/config/sys-con
	mkdir -p $(OUT_DIR)/switch/sys-con
	touch $(OUT_DIR)/atmosphere/contents/690000000000000D/flags/boot2.flag
	cp $(SOURCE_DIR)/Sysmodule/sys-con.nsp $(OUT_DIR)/atmosphere/contents/690000000000000D/exefs.nsp
	cp $(SOURCE_DIR)/AppletCompanion/sys-con.nro $(OUT_DIR)/switch/sys-con/sys-con.nro
	cp -r $(DIST_DIR)/. $(OUT_DIR)/
	@echo [DONE] sys-con compiled successfully. All files have been placed in $(OUT_DIR)/

build:
	$(MAKE) -C $(SOURCE_DIR) ATMOSPHERE_VERSION=$(ATMOSPHERE_VERSION)

clean:
	$(MAKE) -C $(SOURCE_DIR) clean
	rm -rf $(OUT_DIR)
	rm -f $(OUT_ZIP)
	
mrproper: clean
	$(MAKE) -C $(SOURCE_DIR) mrproper
	rm -rf $(OUT_DIR)
	rm -f $(OUT_ZIP)

dist: clean all
	cd $(OUT_DIR)/ && zip -r ../$(OUT_ZIP) .

atmosphere_1.6.x:
	cd lib/Atmosphere-libs && \
	git reset --hard && \
	git checkout a55e74aec3ff24112c981e8e2f677113df045b4c
	
atmosphere_1.7.x:
	cd lib/Atmosphere-libs && \
	git reset --hard && \
	git checkout bb767869105d0eb5c38425f54bf20614639a078d && \
	git revert --no-edit 087f682571631d5d8734dd994b6caa3e96b8e07c
	
distclean: mrproper atmosphere_$(ATMOSPHERE_VERSION) all
	cd $(OUT_DIR)/ && zip -r ../$(OUT_ZIP) .
