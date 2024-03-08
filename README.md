# learn-llvm17-practice
practice for the book "Learn LLVM 17" https://www.packtpub.com/en-sk/product/learn-llvm-17-9781837631346?type=print

Original repo: https://github.com/PacktPublishing/Learn-LLVM-17/tree/main

# build

```bash
# change DLLVM_DIR if not found
cmake -GNinja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_DIR=/usr/local/bin ./
ninja
```

# Run

```bash
./tools/driver/tinylang Gcd.mod
```

# format

- use `clang-format`
    - [https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools C/C++ - Visual Studio Marketplace]
    - config: `.clang-format`
