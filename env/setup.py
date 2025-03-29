import os
import glob
import torch
from setuptools import find_packages, setup
from torch.utils.cpp_extension import CppExtension, BuildExtension

library_name = "chinese_checkers_ext"

def get_extensions():
    debug_mode = os.getenv("DEBUG", "0") == "1"
    use_cuda = os.getenv("USE_CUDA", "1") == "1" and torch.cuda.is_available()
    extension = CppExtension

    torch_lib_dir = os.path.join(os.path.dirname(torch.__file__), "lib")
    extra_link_args = [
        f"-Wl,-rpath,{torch_lib_dir}",
        "-Wl,-rpath,@loader_path",
    ]
    extra_compile_args = {
        "cxx": ["-O3" if not debug_mode else "-O0", "-fdiagnostics-color=always", "-g" if debug_mode else ""],
    }

    this_dir = os.path.dirname(os.path.abspath(__file__))
    extensions_dir = os.path.join(this_dir, "csrc")
    source_files = glob.glob(os.path.join(extensions_dir, "ext", "*.cpp"))
    source_files += glob.glob(os.path.join(extensions_dir, "shared", "*.cpp"))

    # Convert to relative paths
    source_files = [os.path.relpath(path, start=this_dir) for path in source_files]
    include_dirs = [os.path.join(this_dir, "csrc", "ext"),
                    os.path.join(this_dir, "csrc", "shared")]

    return [extension(f"{library_name}._C", source_files, include_dirs=include_dirs,
                      extra_compile_args=extra_compile_args, extra_link_args=extra_link_args)]

setup(
    name=library_name,
    version="0.1.0",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    ext_modules=get_extensions(),
    install_requires=["torch>=2.1.0", "numpy", "pybind11>=2.12"],
    description="Chinese Checkers PyTorch extension",
    cmdclass={"build_ext": BuildExtension},
)
