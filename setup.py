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
        ],
        "nvcc": ["-O3" if not debug_mode else "-O0"],
    }
    if debug_mode:
        extra_compile_args["cxx"].append("-g")
        extra_compile_args["nvcc"].append("-g")
        extra_link_args.extend(["-O0", "-g"])

    base_dir = os.path.dirname(os.path.abspath(__file__))
    src_dir = os.path.join(base_dir, "csrc")
    sources = glob.glob(os.path.join(src_dir, "ext", "*.cpp"))

    if use_cuda:
        cuda_sources = glob.glob(os.path.join(src_dir, "ext", "*.cu"))
        sources += cuda_sources

    ext_modules = [
        extension(
            f"{library_name}._C",
            sources,
            include_dirs=[src_dir, os.path.join(src_dir, "shared")],
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
        )
    ]
    return ext_modules

setup(
    name=library_name,
    version="0.1.0",
    packages=find_packages(),
    ext_modules=get_extensions(),
    install_requires=["torch>=2.1.0", "numpy", "pybind11>=2.12"],
    description="Chinese Checkers PyTorch extension",
    long_description=open("README.md").read() if os.path.isfile("README.md") else "",
    long_description_content_type="text/markdown",
    url="https://github.com/youruser/chinese-checkers", # Replace with your URL
    cmdclass={"build_ext": BuildExtension},
)