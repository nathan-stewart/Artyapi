from setuptools import setup, find_packages

setup(
    name='artyapi',
    version='0.1',
    packages=find_packages(where='src'),
    package_dir={'': 'src'},
    install_requires=[
        'numpy',
        'scipy',
    ],
    test_suite='tests',
)