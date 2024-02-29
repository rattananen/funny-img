## Features

- Generate ascii art from some BMP and some PNG.
- Can modify ascii output.
- The code is cross platform (maybe).

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

📙 The output character will be calculate by luminance of color. The first character is higtest luminance and the last one is lowest.
