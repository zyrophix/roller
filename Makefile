APP_DIR := $(shell pwd)
BUILD := $(APP_DIR)/build

all:
	cmake -B $(BUILD) -S $(APP_DIR) && cmake --build $(BUILD)

run: all
	$(BUILD)/hyprroll

cache:
	$(APP_DIR)/scripts/cache.sh $(APP_DIR)

install: all
	mkdir -p $(HOME)/.local/bin
	install -Dm755 $(BUILD)/hyprroll $(HOME)/.local/bin/hyprroll

lint:
	qmllint $(APP_DIR)/qml/*.qml

clean:
	rm -rf $(BUILD) $(HOME)/.cache/hyprroll/thumbs
