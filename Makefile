APP_DIR := $(shell pwd)
CONFIG := $(APP_DIR)/config.json

run:
	python3 $(APP_DIR)/main.py

cache:
	$(APP_DIR)/scripts/cache.sh $(APP_DIR)

open:
	hyprroll

install:
	mkdir -p $(HOME)/.local/bin
	install -Dm755 $(HOME)/.local/bin/hyprroll $(HOME)/.local/bin/hyprroll
	@echo "installed to ~/.local/bin/hyprroll — add 'hyprroll' to hyprland.lua SUPER+W"

lint:
	qmllint $(APP_DIR)/qml/*.qml
	python3 -m py_compile $(APP_DIR)/main.py $(APP_DIR)/backend/*.py

clean:
	rm -rf $(HOME)/.cache/hyprroll/thumbs
