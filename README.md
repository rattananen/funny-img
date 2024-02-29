## Features

- generate ascii art from some BMP and some PNG.
- can modify ascii output.
- the code is cross platform (maybe).

## usage

### basic

```bash
funny_img test.bmp
```

### output to file:

```bash
funny_img test.bmp 1> output.txt
```

### modify ascii output:

```bash
funny_img test.bmp 123654987
```
📙 the output character will calcurate by luminance of color. The first 
