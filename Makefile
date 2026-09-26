APP_DIR := $(shell pwd)
BUILD := $(APP_DIR)/build

all:
	cmake -B $(BUILD) -S $(APP_DIR) && cmake --build $(BUILD) --parallel

run: all
	$(BUILD)/roller

install: all
	strip --strip-all $(BUILD)/roller 2>/dev/null || true
	mkdir -p $(HOME)/.local/bin
	install -Dm755 $(BUILD)/roller $(HOME)/.local/bin/roller

lint:
	qmllint $(APP_DIR)/qml/*.qml
	qmllint $(APP_DIR)/qml/*.qml --bare 2>&1 | grep -c Warning || true

clean:
	rm -rf $(BUILD)

clean-cache:
	rm -rf $(HOME)/.cache/roller/thumbs

distclean: clean clean-cache
