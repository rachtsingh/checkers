setup:
    @echo "setting up project..."
    @echo "cloning Raylib..."
    @git clone --depth 1 https://github.com/raysan5/raylib.git external/raylib
    @cd external/raylib && git fetch --tags --prune
    @cd external/raylib && git checkout tags/5.5 # v5.5

    # create logs directory
    @mkdir -p logs

    # install Ninja (platform-specific)
    @echo "installing Ninja..."
    @if [ "$(uname)" = "Darwin" ]; then \
        brew install ninja || true; \
    elif [ "$(uname)" = "Linux" ]; then \
        sudo apt-get update && sudo apt-get install -y ninja-build || true; \
    elif [ "$(uname -s)" = "MINGW64_NT-10.0" ]; then \
        choco install ninja -y || true; \
    else \
        echo "unsupported platform for Ninja installation."; \
    fi

    # install uv (if not already installed)
    @echo "installing uv (if not found)..."
    @command -v uv >/dev/null 2>&1 || (curl -sSf https://astral.sh/uv/install.sh | sh)

install:
    @echo "installing project..."
    @if [ ! -d ".venv" ]; then \
        uv venv; \
    fi
    @. .venv/bin/activate && uv pip install pip setuptools wheel
    @. .venv/bin/activate && uv pip install .[gpu] --no-build-isolation

build:
    @echo "building CLI tools (generate and render)..."
    @if [ ! -d "build" ]; then \
        mkdir build; \
    fi
    cmake --build build
    # @cd build &&\
    #     export CMAKE_PREFIX_PATH=$(python -c "import torch.utils; print(f'{torch.utils.cmake_prefix_path}/Torch')") &&\
    #     cmake -GNinja ../csrc/generate && cmake --build .
    # @cd build_render &&\
    #     export CMAKE_PREFIX_PATH=$(python -c "import torch.utils; print(f'{torch.utils.cmake_prefix_path}/Torch')") &&\
    #     cmake -DCMAKE_BUILD_TYPE=Release -GNinja ../csrc/render && cmake --build .

generate: build 
    @echo "running 'generate'..."
    @./build/generate > logs/game.log

render: build
    @echo "running 'render'..."
    @./build/render < logs/game.log

clean:
    @echo "cleaning project..."
    @rm -rf build
    @rm -rf build_render
    @rm -rf logs
    @. .venv/bin/activate && uv pip uninstall chinese_checkers_ext
    @rm -rf *.egg-info
    @rm -rf **/*.egg-info
    @uv cache clean chinese_checkers_ext