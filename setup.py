import os
import glob
import torch

from setuptools import find_packages, setup
from torch.utils.cpp_extension import (
    CppExtension,
    CUDAExtension,
    BuildExtension,
    CUDA_HOME,
)
from torch.utils import cmake_prefix_path

library_name = "chinese_checkers_ext"

def get_extensions():
    debug_mode = os.getenv("DEBUG", "0") == "1"
    use_cuda = os.getenv("USE_CUDA", "1") == "1"

    if debug_mode:
        print("Compiling in debug mode")

    use_cuda = use_cuda and torch.cuda.is_available() and (CUDA_HOME is not None)
    extension = CUDAExtension if use_cuda else CppExtension

    torch_lib_dir = os.path.join(os.path.dirname(torch.__file__), "lib")
    extra_link_args = [
        f"-Wl,-rpath,{torch_lib_dir}",
        "-Wl,-rpath,@loader_path",
    ]
    extra_compile_args = {
        "cxx": [
            "-O3" if not debug_mode else "-O0",
            "-fdiagnostics-color=always",
            "-g" if debug_mode else "",
        ],
        "nvcc": ["-O3" if not debug_mode else "-O0", "-g" if debug_mode else ""],
    }

    this_dir = os.path.dirname(os.path.abspath(__file__))
    extensions_dir = os.path.join(this_dir, "csrc")

    # Collect all source files in csrc/ext/ and csrc/shared/
    source_files = glob.glob(os.path.join(extensions_dir, "ext", "*.cpp"))
    source_files += glob.glob(os.path.join(extensions_dir, "shared", "*.cpp"))

    # Add CUDA sources if available
    if use_cuda:
        cuda_sources = glob.glob(os.path.join(extensions_dir, "ext", "*.cu"))
        source_files += cuda_sources

    # Convert source file paths to be relative to the setup.py directory
    source_files_relative = [
        os.path.relpath(path, start=this_dir) for path in source_files
    ]
    print(source_files_relative)

    # Include directories for header files
    include_dirs = [
        os.path.join(this_dir, "csrc", "ext"),
        os.path.join(this_dir, "csrc", "shared"),
    ]

    ext_modules = [
        extension(
            f"{library_name}._C",
            source_files_relative,
            include_dirs=include_dirs,
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
        )
    ]

    return ext_modules

setup(
    name=library_name,
    version="0.1.0",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    ext_modules=get_extensions(),
    install_requires=["torch>=2.1.0", "numpy", "pybind11>=2.12"],
    description="Chinese Checkers PyTorch extension",
    long_description=open("README.md").read() if os.path.isfile("README.md") else "",
    long_description_content_type="text/markdown",
    url="https://github.com/youruser/chinese-checkers",  # Replace with your repo URL
    cmdclass={"build_ext": BuildExtension},
)