APP_DIR := $(shell pwd)
BUILD := $(APP_DIR)/build

all:
	cmake -B $(BUILD) -S $(APP_DIR) && cmake --build $(BUILD) --parallel

run: all
	$(BUILD)/roller

cache:
	$(APP_DIR)/scripts/cache.sh $(APP_DIR)

install: all
	strip --strip-all $(BUILD)/roller 2>/dev/null || true
	mkdir -p $(HOME)/.local/bin
	install -Dm755 $(BUILD)/roller $(HOME)/.local/bin/roller

lint:
	qmllint $(APP_DIR)/qml/*.qml

clean:
	rm -rf $(BUILD)

clean-cache:
	rm -rf $(HOME)/.cache/roller/thumbs

distclean: clean clean-cache
