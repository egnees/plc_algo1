import pybind11
from distutils.core import setup, Extension

ext_modules = [
    Extension(
        'placer',
        ['src/module.cpp', 'src/impl.cpp', 'src/TaskSolver.cpp', 'src/IdleTaskSolver.cpp',
         'src/bfTaskSolver.cpp', 'src/zdTaskSolver.cpp', 'algo/ZD_heurist_QAP1.cpp', 'src/LayoutGenerator.cpp',
         'src/newTaskSolver.cpp', 'src/dpTaskSolver.cpp', 'algo/dp.cpp'],
        include_dirs=[pybind11.get_include()],
        language='c++',
        extra_compile_args=['-std=c++20'],
    ),
]

setup(
    name='placer',
    version='1.0.9',
    author='3gnees',
    author_email='3gnees@gmail.com',
    description='Library for place electronic elements',
    ext_modules=ext_modules,
    requires=['pybind11'],
    package_dir = {'': 'lib'}
)