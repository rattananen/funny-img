## Features

- Generate ascii art from some BMP and some PNG.
- Can modify ascii output.
- The code is cross platform (maybe).

## Usages

There are executable in folder `dist` if it's incompactible with running platform then you could compile for your platform see [developement](#developement) section.

### Basic:

```bash
funny_img test.bmp
```

### Output to file:

```bash
funny_img test.bmp 1> output.txt
```

### Modify ascii output:

```bash
funny_img test.bmp 123654987
```

📙 The output character will be calculate by luminance of color. The first character is highest luminance and the last one is lowest.

## Developement

### Requirement
- clang, gcc, MSVC, ...
- C++20

### Optional
- Cmake
- Ninja build
- IDE that could integrated with c++ compiler and cmake e.g. `vscode` (for Windows we recommend `Visual stdio 2022 any edition`).
    
