setup:
    @echo "Setting up project..."
    @echo "Cloning Raylib..."
    # @git clone --depth 1 https://github.com/raysan5/raylib.git external/raylib
    @cd external/raylib && git fetch --tags --prune
    @cd external/raylib && git checkout tags/5.5 # v5.5

    # Create logs directory
    @mkdir -p logs

    # Install Ninja (platform-specific)
    @echo "Installing Ninja..."
    @if [ "$(uname)" = "Darwin" ]; then \
        brew install ninja || true; \
    elif [ "$(uname)" = "Linux" ]; then \
        sudo apt-get update && sudo apt-get install -y ninja-build || true; \
    elif [ "$(uname -s)" = "MINGW64_NT-10.0" ]; then \
        choco install ninja -y || true; \
    else \
        echo "Unsupported platform for Ninja installation."; \
    fi

    # Install uv (if not already installed)
    @echo "Installing uv (if not found)..."
    @command -v uv >/dev/null 2>&1 || (curl -sSf https://astral.sh/uv/install.sh | sh)

install:
    @echo "Installing project..."
    @if [ ! -d ".venv" ]; then \
        uv venv; \
    fi
    @. .venv/bin/activate && uv pip install pip setuptools wheel
    @. .venv/bin/activate && uv pip install .[gpu] --no-build-isolation

build:
    @echo "Building project..."
    @if [ ! -d "build" ]; then \
        mkdir build; \
    fi
    @cd build && cmake .. -GNinja && cmake --build .

generate: build
    @echo "Running 'generate'..."
    @./build/generate/generate

render: build
    @echo "Running 'render'..."
    @./build/render/render < logs/game_log.txt

clean:
    @echo "Cleaning project..."
    @rm -rf build
    @rm -rf external
    @rm -rf logs
    @. .venv/bin/activate && uv pip uninstall chinese_checkers_ext
    @rm -rf *.egg-info