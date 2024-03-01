## Features

- Generate ascii art from some BMP and some PNG.
- Can modify ascii output.
- The code is cross platform (maybe).

## Usages

#### Basic:

```bash
funny_img test.bmp
```

#### Output to file:

```bash
funny_img test.bmp 1> output.txt
```

#### Modify ascii output:

```bash
funny_img test.bmp 123654987
```

📙 The output character will be calculate by luminance of color. The first character is highest luminance and the last one is lowest.
