Project Base
====
## Requirement
1. cmake
2. conan - package manager
```bash
pip3 install conan
```
3. download package
```bash
mkdir build && cd build
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
conan install opencv/4.0.1@conan/stable
```

在conanfile.txt裡面加上
[requires]
opencv/4.0.1@conan/stable

```bash
conan install .. --build glad -sbuild_type=Debug
```

## Configure
```bash
cd build
cmake .. -G "Visual Studio 15 2017 Win64"
```

## Build
```bash
cd build
cmake --build .
```

## Thirdparty
1. GLFW3
2. ImGui
