from setuptools import setup, find_packages

setup(
    name="artyapi",
    version="0.1",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    entry_points={
        "console_scripts": [
            "rta=src.rta:main",
        ],
    },
    install_requires=["numpy", "scipy", "matplotlib"],
    test_suite="tests",
)
