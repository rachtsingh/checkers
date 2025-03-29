default:
    @just --list

all: setup install build generate render
    @echo "✓ Project built and run successfully"

setup: check-dependencies create-directories setup-raylib setup-ninja setup-uv
    @echo "✓ Setup completed successfully"

# setup: start
check-dependencies:
    @echo "Checking dependencies..."
    @command -v cmake >/dev/null 2>&1 || (echo "⚠️ CMake not found. Please install CMake and run again." && exit 1)
    @echo "✓ Dependencies check completed"

create-directories:
    @echo "Creating project directories..."
    @mkdir -p external
    @mkdir -p logs
    @mkdir -p build

setup-raylib:
    @echo "Setting up Raylib..."
    @if [ ! -d "external/raylib" ]; then \
        echo "Cloning Raylib..."; \
        git clone --depth 1 https://github.com/raysan5/raylib.git external/raylib; \
        cd external/raylib && git fetch --tags --prune; \
        cd external/raylib && git checkout tags/5.5; \
        echo "✓ Raylib setup complete"; \
    else \
        echo "✓ Raylib is already installed"; \
    fi

setup-ninja:
    @echo "Setting up Ninja build system..."
    @if command -v ninja >/dev/null 2>&1; then \
        echo "✓ Ninja is already installed"; \
    else \
        echo "Installing Ninja..."; \
        if [ "$(uname)" = "Darwin" ]; then \
            brew install ninja || true; \
        elif [ "$(uname)" = "Linux" ]; then \
            if command -v apt-get >/dev/null 2>&1; then \
                sudo apt-get update && sudo apt-get install -y ninja-build; \
            elif command -v dnf >/dev/null 2>&1; then \
                sudo dnf install -y ninja-build; \
            elif command -v pacman >/dev/null 2>&1; then \
                sudo pacman -S --noconfirm ninja; \
            else \
                echo "⚠️ Unsupported Linux distribution. Please install ninja manually."; \
            fi; \
        elif [ "$(uname -s)" = "MINGW64_NT-10.0" ]; then \
            choco install ninja -y || true; \
        else \
            echo "⚠️ Unsupported platform for Ninja installation."; \
        fi; \
    fi

setup-uv:
    @echo "Setting up uv package manager..."
    @if command -v uv >/dev/null 2>&1; then \
        echo "✓ uv is already installed"; \
    else \
        echo "Would you like to install uv package manager? [y/N]"; \
        read -r resp; \
        if [ "$$resp" = "y" ] || [ "$$resp" = "Y" ]; then \
            echo "Installing uv..."; \
            curl -sSf https://astral.sh/uv/install.sh | sh; \
            echo "✓ uv installation complete"; \
        else \
            echo "❌ uv is required for this project. Exiting."; \
            exit 1; \
        fi; \
    fi

    @echo "Setting up Python with uv..."
    @read -p "Would you like to install Python 3.11 using uv? [y/N] " resp; \
    if [ "$$resp" = "y" ] || [ "$$resp" = "Y" ]; then \
        echo "Installing Python 3.11 using uv..."; \
        uv python install 3.11; \
        echo "✓ Python 3.11 installed"; \
    else \
        echo "Skipping Python installation."; \
    fi
# setup: end

install:
    @echo "Installing project..."
    @uv sync
    @echo "✓ Project installation complete"

build: configure-cmake build-cpp

# build: begin
TORCH_CMAKE_PATH := `uv run python -c "import torch; print(torch.utils.cmake_prefix_path)"`

configure-cmake:
    @echo "Configuring CMake project..."
    @if [ ! -d "build" ]; then \
        mkdir -p build; \
    fi
    @echo "Checking Python and PyTorch configuration..."
    @echo "TORCH_CMAKE_PATH={{TORCH_CMAKE_PATH}}"
    @cd build && cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH={{TORCH_CMAKE_PATH}} ..
    @echo "✓ CMake configuration complete"

build-cpp:
    @echo "Building C++ executables..."
    @cmake --build build
    @echo "✓ Build complete"

build-debug:
    @echo "Building with debug symbols..."
    @if [ ! -d "build" ]; then \
        mkdir -p build; \
    fi
    @eval "$$(. .venv/bin/activate && echo 'TORCH_CMAKE_PATH=$$(python -c "import torch; print(torch.utils.cmake_prefix_path)")')"
    @cd build && cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$$TORCH_CMAKE_PATH .. && cmake --build .
    @echo "✓ Debug build complete"
# build: end

generate: build
    @echo "running 'generate'..."
    @./build/generate run -n 25 -o logs/game.log

render: build
    @echo "running 'render'..."
    @./build/render < logs/game.log

render-with-indices: build
    @echo "running 'render' with grid indices..."
    @./build/render -g < logs/game.log

clean: clean-build clean-python
    @echo "✓ Project cleaned successfully"

# clean: begin
clean-build:
    @echo "Cleaning build artifacts..."
    @rm -rf build
    @rm -rf logs/*.log
    @echo "✓ Build artifacts cleaned"

clean-python:
    @echo "Cleaning Python artifacts..."
    @if [ -d ".venv" ]; then \
        . .venv/bin/activate && uv pip uninstall chinese_checkers_ext || true; \
    fi
    @rm -rf env/*.egg-info
    @rm -rf env/**/*.egg-info
    @find env -type d -name __pycache__ -exec rm -rf {} +
    @uv cache clean chinese_checkers_ext
    @echo "✓ Python artifacts cleaned"
# clean: end

distclean: clean
    @echo "Performing complete cleanup..."
    @rm -rf .venv
    @rm -rf external
    @echo "✓ Complete cleanup finished"
